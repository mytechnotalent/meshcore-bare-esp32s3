// MIT License
//
// Copyright (c) 2026 Kevin Thomas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Author:  Kevin Thomas
// Email:   kevin@mytechnotalent.com
// GitHub:  https://github.com/mytechnotalent/bare-meshcore-esp32s3
// File:    ble_companion.c
// Desc:    Implements the MeshCore BLE companion transport.
// Created: 2026

#include "ble_companion.h"
#include "companion.h"
#include "config.h"
#include "node_state.h"
#include "storage.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

/**
 * @brief Initialize ESP-IDF NimBLE persistent storage callbacks.
 *
 * @param void No parameters.
 * @return void
 */
extern void ble_store_config_init(void);

/**
 * @brief BLE transport log tag.
 */
static const char *_ble_log_tag = "ble_companion";

/**
 * @brief Static MeshCore pairing passkey.
 */
#define MESHCORE_BLE_PASSKEY MESHCORE_BLE_PIN

/**
 * @brief Inferred BLE controller address type.
 */
static uint8_t _own_addr_type = BLE_OWN_ADDR_PUBLIC;

/**
 * @brief MeshCore Nordic-UART service UUID.
 */
static const ble_uuid128_t _service_uuid = BLE_UUID128_INIT(0x9e,
                                                            0xca,
                                                            0xdc,
                                                            0x24,
                                                            0x0e,
                                                            0xe5,
                                                            0xa9,
                                                            0xe0,
                                                            0x93,
                                                            0xf3,
                                                            0xa3,
                                                            0xb5,
                                                            0x01,
                                                            0x00,
                                                            0x40,
                                                            0x6e);

/**
 * @brief MeshCore RX characteristic UUID.
 */
static const ble_uuid128_t _rx_uuid = BLE_UUID128_INIT(0x9e,
                                                       0xca,
                                                       0xdc,
                                                       0x24,
                                                       0x0e,
                                                       0xe5,
                                                       0xa9,
                                                       0xe0,
                                                       0x93,
                                                       0xf3,
                                                       0xa3,
                                                       0xb5,
                                                       0x02,
                                                       0x00,
                                                       0x40,
                                                       0x6e);

/**
 * @brief MeshCore TX characteristic UUID.
 */
static const ble_uuid128_t _tx_uuid = BLE_UUID128_INIT(0x9e,
                                                       0xca,
                                                       0xdc,
                                                       0x24,
                                                       0x0e,
                                                       0xe5,
                                                       0xa9,
                                                       0xe0,
                                                       0x93,
                                                       0xf3,
                                                       0xa3,
                                                       0xb5,
                                                       0x03,
                                                       0x00,
                                                       0x40,
                                                       0x6e);

/**
 * @brief Handle MeshCore GATT characteristic access.
 *
 * @param connection Connection handle.
 * @param attribute Attribute handle.
 * @param context Pointer to access context structure.
 * @param argument User argument pointer.
 * @return int 0 on success, or ATT error code.
 */
static int _gatt_access(uint16_t connection,
                        uint16_t attribute,
                        struct ble_gatt_access_ctxt *context,
                        void *argument);

/**
 * @brief TX characteristic value handle.
 */
static uint16_t _tx_handle;

/**
 * @brief MeshCore GATT characteristic definitions.
 */
static const struct ble_gatt_chr_def _characteristics[] = {
    {.uuid = &_tx_uuid.u,
     .access_cb = _gatt_access,
     .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ_ENC |
              BLE_GATT_CHR_F_READ_AUTHEN,
     .val_handle = &_tx_handle},
    {.uuid = &_rx_uuid.u,
     .access_cb = _gatt_access,
     .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE_ENC |
              BLE_GATT_CHR_F_WRITE_AUTHEN},
    {0}};

/**
 * @brief MeshCore GATT service definitions.
 */
static const struct ble_gatt_svc_def _services[] = {{.type = BLE_GATT_SVC_TYPE_PRIMARY,
                                                     .uuid = &_service_uuid.u,
                                                     .characteristics = _characteristics},
                                                    {0}};

/**
 * @brief Active BLE connection handle.
 */
static uint16_t _connection = BLE_HS_CONN_HANDLE_NONE;

/**
 * @brief Maximum companion write accepted from the BLE client in bytes.
 */
#define BLE_COMPANION_MAX_WRITE 172U

/**
 * @brief Deferred companion command buffer structure.
 */
typedef struct {
    /**
     * @brief Command length in bytes.
     */
    uint16_t length;
    /**
     * @brief Command data payload bytes.
     */
    uint8_t data[BLE_COMPANION_MAX_WRITE];
} BleCompanionCommand;

/**
 * @brief Queue of companion commands awaiting application-task processing.
 */
static QueueHandle_t _command_queue;

/**
 * @brief Run the NimBLE host event loop.
 *
 * @param argument FreeRTOS task argument pointer (unused).
 * @return void
 */
static void _host_task(void *argument) {
    (void)argument;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * @brief Start BLE advertising after host synchronization.
 *
 * @param void No parameters.
 * @return void
 */
static void _ble_start(void);

/**
 * @brief Handle BLE GAP lifecycle and security events.
 *
 * @param event Pointer to GAP event structure.
 * @param argument User argument pointer.
 * @return int 0 on success, or GAP error code.
 */
static int _gap_event(struct ble_gap_event *event, void *argument) {
    (void)argument;
    if (event->type == BLE_GAP_EVENT_CONNECT) {
        _connection =
            event->connect.status == 0 ? event->connect.conn_handle : BLE_HS_CONN_HANDLE_NONE;
        ESP_LOGI(
            _ble_log_tag, "BLE connect status=%d handle=%u", event->connect.status, _connection);
        if (event->connect.status == 0) {
            int sec_rc = ble_gap_security_initiate(event->connect.conn_handle);
            ESP_LOGI(_ble_log_tag,
                     "BLE security initiate handle=%u rc=%d",
                     event->connect.conn_handle,
                     sec_rc);
        }
    } else if (event->type == BLE_GAP_EVENT_ENC_CHANGE) {
        ESP_LOGI(_ble_log_tag,
                 "BLE encryption status=%d handle=%u",
                 event->enc_change.status,
                 event->enc_change.conn_handle);
    } else if (event->type == BLE_GAP_EVENT_MTU) {
        ESP_LOGI(
            _ble_log_tag, "BLE MTU handle=%u mtu=%u", event->mtu.conn_handle, event->mtu.value);
    } else if (event->type == BLE_GAP_EVENT_REPEAT_PAIRING) {
        struct ble_gap_conn_desc description;
        int status = ble_gap_conn_find(event->repeat_pairing.conn_handle, &description);
        if (status == 0) {
            ble_store_util_delete_peer(&description.peer_id_addr);
        }
        ESP_LOGI(_ble_log_tag,
                 "BLE repeat pairing handle=%u status=%d",
                 event->repeat_pairing.conn_handle,
                 status);
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    } else if (event->type == BLE_GAP_EVENT_PASSKEY_ACTION) {
        struct ble_sm_io io = {0};
        io.action = event->passkey.params.action;
        if (io.action == BLE_SM_IOACT_DISP) {
            io.passkey = MESHCORE_BLE_PASSKEY;
        } else if (io.action == BLE_SM_IOACT_NUMCMP) {
            io.numcmp_accept = 1U;
        } else if (io.action == BLE_SM_IOACT_INPUT) {
            io.passkey = MESHCORE_BLE_PASSKEY;
        }
        ble_sm_inject_io(event->passkey.conn_handle, &io);
        ESP_LOGI(_ble_log_tag,
                 "BLE pairing action=%u passkey=%u",
                 event->passkey.params.action,
                 (unsigned int)MESHCORE_BLE_PASSKEY);
    } else if (event->type == BLE_GAP_EVENT_DISCONNECT) {
        _connection = BLE_HS_CONN_HANDLE_NONE;
        ESP_LOGI(_ble_log_tag, "BLE disconnect reason=%d", event->disconnect.reason);
        _ble_start();
    }
    return 0;
}

/**
 * @brief Handle one GATT write by deferring work outside the NimBLE host task.
 *
 * @param connection Connection handle.
 * @param attribute Attribute handle.
 * @param context Pointer to access context structure.
 * @param argument User argument pointer.
 * @return int 0 on success, or ATT error code.
 */
static int _gatt_access(uint16_t connection,
                        uint16_t attribute,
                        struct ble_gatt_access_ctxt *context,
                        void *argument) {
    BleCompanionCommand command = {0};
    uint16_t length;
    (void)attribute;
    (void)argument;
    (void)connection;
    if (context->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return 0;
    }
    length = OS_MBUF_PKTLEN(context->om);
    if (length > sizeof(command.data) ||
        os_mbuf_copydata(context->om, 0, length, command.data) != 0) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    command.length = length;
    if (_command_queue == NULL || xQueueSend(_command_queue, &command, 0U) != pdTRUE) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return 0;
}

/**
 * @brief Process deferred companion commands outside the NimBLE host task.
 *
 * @param argument FreeRTOS task argument pointer (unused).
 * @return void
 */
static void _command_task(void *argument) {
    BleCompanionCommand command;
    (void)argument;
    while (xQueueReceive(_command_queue, &command, portMAX_DELAY) == pdTRUE) {
        companion_receive(command.data, command.length);
        ESP_LOGI(_ble_log_tag,
                 "BLE RX length=%u command=%u",
                 command.length,
                 command.length > 0U ? command.data[0] : 0U);
        ble_companion_flush_pending();
    }
}

/**
 * @brief Set the primary advertisement and scan response payloads.
 *
 * @param void No parameters.
 * @return void
 */
static void _ble_start(void) {
    struct ble_hs_adv_fields fields = {0};
    struct ble_hs_adv_fields response_fields = {0};
    struct ble_gap_adv_params parameters = {0};
    const char *name = node_state_get_name();
    size_t name_len = strlen(name);
    int rc;
    if (ble_gap_adv_active()) {
        ble_gap_adv_stop();
    }
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = (ble_uuid128_t *)&_service_uuid;
    fields.num_uuids128 = 1U;
    fields.uuids128_is_complete = 1U;
    if (name_len <= 8U) {
        fields.name = (uint8_t *)name;
        fields.name_len = (uint8_t)name_len;
        fields.name_is_complete = 1U;
    } else {
        fields.name = (uint8_t *)"MeshCore";
        fields.name_len = 8U;
        fields.name_is_complete = 0U;
    }
    if (name_len > 29U) {
        name_len = 29U;
    }
    response_fields.name = (uint8_t *)name;
    response_fields.name_len = (uint8_t)name_len;
    response_fields.name_is_complete = 1U;
    if (name_len <= 26U) {
        response_fields.tx_pwr_lvl_is_present = 1U;
        response_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    }
    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(_ble_log_tag, "ble_gap_adv_set_fields failed rc=%d", rc);
    }
    rc = ble_gap_adv_rsp_set_fields(&response_fields);
    if (rc != 0) {
        ESP_LOGE(_ble_log_tag, "ble_gap_adv_rsp_set_fields failed rc=%d", rc);
    }
    parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    parameters.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    parameters.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;
    rc = ble_gap_adv_start(_own_addr_type, NULL, BLE_HS_FOREVER, &parameters, _gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(_ble_log_tag, "ble_gap_adv_start failed rc=%d", rc);
    } else {
        ESP_LOGI(_ble_log_tag, "BLE advertising started: %s (addr_type=%u)", name, _own_addr_type);
    }
}

/**
 * @brief NimBLE host synchronization callback.
 *
 * @param void No parameters.
 * @return void
 */
static void _on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(_ble_log_tag, "ble_hs_util_ensure_addr failed rc=%d", rc);
    }
    rc = ble_hs_id_infer_auto(0U, &_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(_ble_log_tag, "ble_hs_id_infer_auto failed rc=%d", rc);
        _own_addr_type = BLE_OWN_ADDR_PUBLIC;
    }
    _ble_start();
}

/**
 * @brief Flush queued companion responses to the connected BLE client.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_flush_pending(void) {
    uint8_t response[172U];
    size_t length;
    int rc;
    struct os_mbuf *buffer;
    if (_connection == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }
    while (true) {
        length = companion_next_response(response, sizeof(response));
        if (length == 0U) {
            break;
        }
        buffer = ble_hs_mbuf_from_flat(response, length);
        if (buffer == NULL) {
            break;
        }
        rc = ble_gatts_notify_custom(_connection, _tx_handle, buffer);
        ESP_LOGI(_ble_log_tag,
                 "BLE TX length=%u code=0x%02X handle=%u rc=%d",
                 (unsigned int)length,
                 response[0],
                 _tx_handle,
                 rc);
        if (rc != 0) {
            break;
        }
    }
}

/**
 * @brief Initialize BLE services, security, and persistent bond storage.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_init(void) {
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
    ble_hs_cfg.sm_bonding = 1U;
    ble_hs_cfg.sm_mitm = 1U;
    ble_hs_cfg.sm_sc = 0U;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sync_cb = _on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(_services);
    ble_gatts_add_svcs(_services);
    ble_svc_gap_device_name_set(node_state_get_name());
    ble_store_config_init();
    if (storage_claim_ble_bond_reset()) {
        ble_store_clear();
        ESP_LOGI(_ble_log_tag, "Cleared stale BLE bonds for pairing reset");
    }
}

/**
 * @brief Start the NimBLE host task.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_start(void) {
    _command_queue = xQueueCreate(8U, sizeof(BleCompanionCommand));
    xTaskCreate(_command_task, "companion_rx", 6144U, NULL, 5U, NULL);
    nimble_port_freertos_init(_host_task);
}

/**
 * @brief Update the advertised and GAP BLE device name.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_update_device_name(void) {
    const char *name = node_state_get_name();
    size_t name_len = strlen(name);
    struct ble_hs_adv_fields response_fields = {0};
    ble_svc_gap_device_name_set(name);
    if (name_len > 29U) {
        name_len = 29U;
    }
    response_fields.name = (uint8_t *)name;
    response_fields.name_len = (uint8_t)name_len;
    response_fields.name_is_complete = 1U;
    ble_gap_adv_rsp_set_fields(&response_fields);
    ESP_LOGI(_ble_log_tag, "BLE updated device name=%s", name);
}
