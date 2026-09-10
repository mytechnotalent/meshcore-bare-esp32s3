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
// File:    companion.c
// Desc:    Implements the MeshCore companion frame parser and responses.
// Created: 2026

#include "companion.h"

#include "advert.h"
#include "ble_companion.h"
#include "config.h"
#include "datagram.h"
#include "esp_log.h"
#include "mesh_transport.h"
#include "node_state.h"
#include "storage.h"
#include "sx1262.h"

#include <inttypes.h>
#include <string.h>

/**
 * @brief Companion parser log tag.
 */
static const char *_companion_log_tag = "companion";

/**
 * @brief Maximum companion frame size in bytes.
 */
#define COMPANION_MAX_FRAME 172U

/**
 * @brief App-start command code.
 */
#define COMPANION_CMD_APP_START 1U

/**
 * @brief Send text command code.
 */
#define COMPANION_CMD_SEND_TEXT 2U

/**
 * @brief Send channel text command code.
 */
#define COMPANION_CMD_SEND_CHANNEL_TEXT 3U

/**
 * @brief Send self advertisement command code.
 */
#define COMPANION_CMD_SEND_SELF_ADVERT 7U

/**
 * @brief Get contacts command code.
 */
#define COMPANION_CMD_GET_CONTACTS 4U

/**
 * @brief Synchronize the next queued message command code.
 */
#define COMPANION_CMD_SYNC_NEXT_MESSAGE 10U

/**
 * @brief Get-device-time command code.
 */
#define COMPANION_CMD_GET_DEVICE_TIME 5U

/**
 * @brief Get battery and storage command code.
 */
#define COMPANION_CMD_GET_BATT_STORAGE 20U

/**
 * @brief Set-device-time command code.
 */
#define COMPANION_CMD_SET_DEVICE_TIME 6U

/**
 * @brief Set radio parameters command code.
 */
#define COMPANION_CMD_SET_RADIO_PARAMS 11U

/**
 * @brief Set transmit power command code.
 */
#define COMPANION_CMD_SET_RADIO_TX_POWER 12U

/**
 * @brief Reboot command code.
 */
#define COMPANION_CMD_REBOOT 19U

/**
 * @brief Set default flood scope command code.
 */
#define COMPANION_CMD_SET_DEFAULT_SCOPE 63U

/**
 * @brief Set temporary flood scope command code.
 */
#define COMPANION_CMD_SET_FLOOD_SCOPE_KEY 54U

/**
 * @brief Set path-hash mode command code.
 */
#define COMPANION_CMD_SET_PATH_HASH_MODE 61U

/**
 * @brief Get default flood scope command code.
 */
#define COMPANION_CMD_GET_DEFAULT_SCOPE 64U

/**
 * @brief Set advertisement coordinates command code.
 */
#define COMPANION_CMD_SET_ADVERT_LATLON 14U

/**
 * @brief Set companion behavior parameters command code.
 */
#define COMPANION_CMD_SET_OTHER_PARAMS 38U

/**
 * @brief Get custom variables command code.
 */
#define COMPANION_CMD_GET_CUSTOM_VARS 40U

/**
 * @brief Get tuning parameters command code.
 */
#define COMPANION_CMD_GET_TUNING_PARAMS 43U

/**
 * @brief Device-query command code.
 */
#define COMPANION_CMD_DEVICE_QUERY 22U

/**
 * @brief Remove contact command code.
 */
#define COMPANION_CMD_REMOVE_CONTACT 15U

/**
 * @brief Get contact by key command code.
 */
#define COMPANION_CMD_GET_CONTACT_BY_KEY 30U

/**
 * @brief Get-channel command code.
 */
#define COMPANION_CMD_GET_CHANNEL 31U

/**
 * @brief Set-channel command code.
 */
#define COMPANION_CMD_SET_CHANNEL 32U

/**
 * @brief Add or update contact command code.
 */
#define COMPANION_CMD_ADD_UPDATE_CONTACT 9U

/**
 * @brief Set advert name command code.
 */
#define COMPANION_CMD_SET_ADVERT_NAME 8U

/**
 * @brief Send login command code.
 */
#define COMPANION_CMD_SEND_LOGIN 26U

/**
 * @brief Send status request command code.
 */
#define COMPANION_CMD_SEND_STATUS_REQ 27U

/**
 * @brief Send trace path command code.
 */
#define COMPANION_CMD_SEND_TRACE_PATH 36U

/**
 * @brief Send binary request command code.
 */
#define COMPANION_CMD_SEND_BINARY_REQ 50U

/**
 * @brief OK response code.
 */
#define COMPANION_RESP_OK 0U

/**
 * @brief Current-time response code.
 */
#define COMPANION_RESP_CURRENT_TIME 9U

/**
 * @brief Self-info response code.
 */
#define COMPANION_RESP_SELF_INFO 5U

/**
 * @brief Device-info response code.
 */
#define COMPANION_RESP_DEVICE_INFO 13U

/**
 * @brief Channel-info response code.
 */
#define COMPANION_RESP_CHANNEL_INFO 18U

/**
 * @brief Contact-message response code for app protocol version 3.
 */
#define COMPANION_RESP_CONTACT_MSG_V3 16U

/**
 * @brief Channel-message response code for app protocol version 3.
 */
#define COMPANION_RESP_CHANNEL_MSG_V3 17U

/**
 * @brief No-more-messages response code.
 */
#define COMPANION_RESP_NO_MORE_MESSAGES 10U

/**
 * @brief Error response code.
 */
#define COMPANION_RESP_ERROR 1U

/**
 * @brief Message-waiting push code.
 */
#define COMPANION_PUSH_MESSAGE_WAITING 0x83U

/**
 * @brief Status response push code.
 */
#define COMPANION_PUSH_STATUS_RESPONSE 0x87U

/**
 * @brief Raw RX data log push code.
 */
#define COMPANION_PUSH_LOG_RX_DATA 0x88U

/**
 * @brief Trace data push code.
 */
#define COMPANION_PUSH_TRACE_DATA 0x89U

/**
 * @brief Sent response code.
 */
#define COMPANION_RESP_SENT 6U

/**
 * @brief Contacts-start response code.
 */
#define COMPANION_RESP_CONTACTS_START 2U

/**
 * @brief Contact response code.
 */
#define COMPANION_RESP_CONTACT 3U

/**
 * @brief End-of-contacts response code.
 */
#define COMPANION_RESP_END_OF_CONTACTS 4U

/**
 * @brief Default flood-scope response code.
 */
#define COMPANION_RESP_DEFAULT_SCOPE 28U

/**
 * @brief Battery/storage response code.
 */
#define COMPANION_RESP_BATT_STORAGE 12U

/**
 * @brief Tuning parameters response code.
 */
#define COMPANION_RESP_TUNING 23U

/**
 * @brief Custom variables response code.
 */
#define COMPANION_RESP_CUSTOM_VARS 21U

/**
 * @brief Outgoing frame slot structure.
 */
typedef struct {
    /**
     * @brief Raw frame payload bytes.
     */
    uint8_t data[COMPANION_MAX_FRAME];
    /**
     * @brief Frame length in bytes.
     */
    size_t length;
} CompanionTxSlot;

/**
 * @brief Outgoing frame queue capacity.
 */
#define COMPANION_TX_QUEUE_CAPACITY 8U

/**
 * @brief Incoming framed command bytes.
 */
static uint8_t _rx_frame[COMPANION_MAX_FRAME];

/**
 * @brief Current incoming frame length.
 */
static size_t _rx_length;

/**
 * @brief Circular buffer of outgoing response frames.
 */
static CompanionTxSlot _tx_queue[COMPANION_TX_QUEUE_CAPACITY];

/**
 * @brief Queue read index.
 */
static size_t _tx_head;

/**
 * @brief Queue write index.
 */
static size_t _tx_tail;

/**
 * @brief Number of queued response frames.
 */
static size_t _tx_count;

/**
 * @brief Whether the active companion transport uses raw BLE packets.
 */
static bool _raw_ble_mode = true;

/**
 * @brief Pending synchronized message arguments.
 */
static uint8_t _message_args[168U];

/**
 * @brief Pending synchronized message argument length.
 */
static size_t _message_length;

/**
 * @brief Whether a message is waiting for synchronization.
 */
static bool _message_waiting;

/**
 * @brief Whether the pending message is a channel message.
 */
static bool _message_is_channel;

/**
 * @brief Temporary flood scope override key.
 */
static uint8_t _send_scope_key[16U];

/**
 * @brief Whether a temporary scope key is active.
 */
static bool _send_scope_active;

/**
 * @brief Whether outgoing floods are explicitly unscoped.
 */
static bool _send_unscoped;

/**
 * @brief Request a reboot after the pending OK response is sent.
 */
static volatile bool _reboot_pending;

/**
 * @brief Whether a contact synchronization is active.
 */
static bool _contact_sync_active;

/**
 * @brief Next contact ordinal to emit.
 */
static size_t _contact_sync_index;

/**
 * @brief Highest contact modification timestamp seen during sync.
 */
static uint32_t _contact_sync_last_modified;

/**
 * @brief Queue a framed companion response.
 *
 * @param code Response type code.
 * @param args Pointer to response argument bytes.
 * @param args_length Number of argument bytes.
 * @return void
 */
static void _companion_response(uint8_t code, const uint8_t *args, size_t args_length);

/**
 * @brief Serialize one contact into the companion contact response layout.
 *
 * @param contact Pointer to contact structure to serialize.
 * @param data Destination output buffer.
 * @param capacity Capacity of destination buffer in bytes (must be >= 147).
 * @return size_t Serialized length in bytes, or zero on error.
 */
static size_t _companion_contact_bytes(const NodeContact *contact, uint8_t *data, size_t capacity) {
    size_t position = 0U;
    if (contact == NULL || data == NULL || capacity < 147U) {
        return 0U;
    }
    memcpy(&data[position], contact->public_key, 32U);
    position += 32U;
    data[position++] = contact->type;
    data[position++] = contact->flags;
    data[position++] = (uint8_t)contact->out_path_length;
    memcpy(&data[position], contact->out_path, 64U);
    position += 64U;
    memcpy(&data[position], contact->name, 32U);
    position += 32U;
    memcpy(&data[position], &contact->last_advert_timestamp, 4U);
    position += 4U;
    memcpy(&data[position], &contact->latitude, 4U);
    position += 4U;
    memcpy(&data[position], &contact->longitude, 4U);
    position += 4U;
    memcpy(&data[position], &contact->last_modified, 4U);
    position += 4U;
    return position;
}

/**
 * @brief Queue the next contact synchronization response.
 *
 * @param void No parameters.
 * @return void
 */
static void _companion_contact_sync_next(void) {
    const NodeContact *contact;
    uint8_t data[147U];
    size_t length;
    contact = node_state_contact_at(_contact_sync_index++);
    if (contact == NULL) {
        _contact_sync_active = false;
        _companion_response(COMPANION_RESP_END_OF_CONTACTS,
                            (const uint8_t *)&_contact_sync_last_modified,
                            sizeof(_contact_sync_last_modified));
        return;
    }
    if (contact->last_modified > _contact_sync_last_modified) {
        _contact_sync_last_modified = contact->last_modified;
    }
    length = _companion_contact_bytes(contact, data, sizeof(data));
    _companion_response(COMPANION_RESP_CONTACT, data, length);
}

/**
 * @brief Parse one contact from an add/update command.
 *
 * @param data Pointer to raw received contact data bytes.
 * @param length Length of data in bytes (must be >= 131).
 * @param contact Destination NodeContact pointer.
 * @return bool true if parsed successfully, false on malformed data.
 */
static bool _companion_parse_contact(const uint8_t *data, size_t length, NodeContact *contact) {
    size_t position = 0U;
    if (data == NULL || contact == NULL || length < 131U) {
        return false;
    }
    memset(contact, 0, sizeof(*contact));
    memcpy(contact->public_key, &data[position], 32U);
    position += 32U;
    contact->type = data[position++];
    contact->flags = data[position++];
    contact->out_path_length = (int8_t)data[position++];
    if (contact->out_path_length < -1 || contact->out_path_length > 64) {
        return false;
    }
    memcpy(contact->out_path, &data[position], 64U);
    position += 64U;
    memcpy(contact->name, &data[position], 32U);
    contact->name[31] = '\0';
    position += 32U;
    if (position + 4U <= length) {
        memcpy(&contact->last_advert_timestamp, &data[position], 4U);
        position += 4U;
    }
    if (position + 4U <= length) {
        memcpy(&contact->latitude, &data[position], 4U);
        position += 4U;
    }
    if (position + 4U <= length) {
        memcpy(&contact->longitude, &data[position], 4U);
        position += 4U;
    }
    if (position + 4U <= length) {
        memcpy(&contact->last_modified, &data[position], 4U);
        position += 4U;
    }
    if (contact->last_modified == 0U) {
        contact->last_modified = node_state_get_time();
    }
    return true;
}

/**
 * @brief Queue a framed companion response with bounded arguments.
 *
 * Enqueues a response into the circular TX buffer for delivery over BLE.
 *
 * @param code Response opcode.
 * @param args Pointer to response argument bytes.
 * @param args_length Number of argument bytes.
 * @return void
 */
static void _companion_response(uint8_t code, const uint8_t *args, size_t args_length) {
    CompanionTxSlot *slot;
    size_t packet_length = args_length + 1U;
    if ((_raw_ble_mode && packet_length > COMPANION_MAX_FRAME) ||
        (!_raw_ble_mode && packet_length + 3U > COMPANION_MAX_FRAME)) {
        return;
    }
    if (_tx_count >= COMPANION_TX_QUEUE_CAPACITY) {
        _tx_head = (_tx_head + 1U) % COMPANION_TX_QUEUE_CAPACITY;
        _tx_count--;
    }
    slot = &_tx_queue[_tx_tail];
    if (_raw_ble_mode) {
        slot->data[0] = code;
        if (args_length > 0U && args != NULL) {
            memcpy(&slot->data[1], args, args_length);
        }
        slot->length = packet_length;
    } else {
        slot->data[0] = '>';
        slot->data[1] = (uint8_t)(packet_length & 255U);
        slot->data[2] = (uint8_t)(packet_length >> 8U);
        slot->data[3] = code;
        if (args_length > 0U && args != NULL) {
            memcpy(&slot->data[4], args, args_length);
        }
        slot->length = packet_length + 3U;
    }
    _tx_tail = (_tx_tail + 1U) % COMPANION_TX_QUEUE_CAPACITY;
    _tx_count++;
}

/**
 * @brief Generate the fixed self-info reply used during app startup.
 *
 * @param void No parameters.
 * @return void
 */
static void _companion_self_info(void) {
    uint8_t args[89U] = {0};
    int32_t latitude;
    int32_t longitude;
    NodeRadioSettings radio;
    size_t name_length = strnlen(node_state_get_name(), 32U);
    node_state_get_location(&latitude, &longitude);
    node_state_get_radio(&radio);
    args[0] = 1U;
    args[1] = (uint8_t)radio.transmit_power;
    args[2] = 22U;
    memcpy(&args[3], storage_identity()->public_key, 32U);
    memcpy(&args[35], &latitude, sizeof(latitude));
    memcpy(&args[39], &longitude, sizeof(longitude));
    {
        uint32_t frequency_khz = radio.frequency / 1000U;
        memcpy(&args[47], &frequency_khz, sizeof(frequency_khz));
    }
    memcpy(&args[51], &radio.bandwidth, sizeof(radio.bandwidth));
    args[55] = radio.spreading_factor;
    args[56] = radio.coding_rate;
    memcpy(&args[57], node_state_get_name(), name_length);
    _companion_response(COMPANION_RESP_SELF_INFO, args, 57U + name_length);
}

/**
 * @brief Generate the firmware identity reply used by app negotiation.
 *
 * @param void No parameters.
 * @return void
 */
static void _companion_device_info(void) {
    uint8_t args[81U] = {0};
    args[0] = 12U;
    args[1] = (uint8_t)(NODE_STATE_MAX_CONTACTS / 2U);
    args[2] = (uint8_t)NODE_STATE_MAX_CHANNELS;
    memcpy(&args[3], &(const uint32_t){MESHCORE_BLE_PIN}, sizeof(uint32_t));
    memcpy(&args[7], "Sep 2026", 8U);
    memcpy(&args[19], "bare-meshcore", 13U);
    memcpy(&args[59], "0.1.0", 5U);
    args[79] = 0U;
    args[80] = node_state_get_path_hash_mode();
    _companion_response(COMPANION_RESP_DEVICE_INFO, args, sizeof(args));
}

/**
 * @brief Dispatch one complete companion command.
 *
 * Parses opcode, extracts arguments, dispatches radio or storage operations,
 * and queues appropriate responses.
 *
 * @param command Pointer to received command buffer.
 * @param length Total length of command in bytes.
 * @return void
 */
static void _companion_command(const uint8_t *command, size_t length) {
    uint8_t args[4] = {0};
    uint8_t channel[49U] = {0};
    uint8_t contact_data[147U];
    NodeContact contact;
    NodeChannel node_channel;
    NodeRadioSettings radio;
    bool state_ok;
    const NodeContact *stored_contact;
    MeshPacket packet;
    uint8_t wire[255U];
    uint8_t sent_args[9U];
    size_t wire_length;
    uint32_t ack_code;
    uint32_t message_timestamp;
    uint8_t advert_payload[MESH_ADVERT_MAX_PAYLOAD];
    size_t advert_length;
    MeshAdvertOptions advert_options;
    if (length == 0U) {
        return;
    }
    switch (command[0]) {
    case COMPANION_CMD_SET_FLOOD_SCOPE_KEY:
        if (length >= 2U && command[1] == 0U) {
            memset(_send_scope_key, 0, sizeof(_send_scope_key));
            if (length >= 18U) {
                memcpy(_send_scope_key, &command[2], sizeof(_send_scope_key));
                _send_scope_active = true;
            } else {
                _send_scope_active = false;
            }
            _send_unscoped = false;
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else if (length >= 2U && command[1] == 1U) {
            _send_scope_active = false;
            _send_unscoped = true;
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_APP_START:
        _companion_self_info();
        break;
    case COMPANION_CMD_DEVICE_QUERY:
        _companion_device_info();
        break;
    case COMPANION_CMD_SEND_TEXT:
        stored_contact = length >= 13U ? node_state_find_contact(&command[7], 6U) : NULL;
        message_timestamp = 0U;
        if (length >= 7U) {
            memcpy(&message_timestamp, &command[3], sizeof(message_timestamp));
        }
        state_ok = stored_contact != NULL && length > 13U && command[1] <= 3U &&
                   mesh_datagram_text(storage_identity(),
                                      stored_contact,
                                      message_timestamp,
                                      command[2],
                                      (const char *)&command[13],
                                      &packet,
                                      &ack_code);
        if (state_ok) {
            if (packet.route == MESH_ROUTE_FLOOD) {
                uint8_t scope_key[16U];
                node_state_get_default_scope(NULL, scope_key);
                mesh_transport_apply_scope(&packet, scope_key);
            }
        }
        if (state_ok && mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
            sx1262_transmit(wire, wire_length)) {
            memset(sent_args, 0, sizeof(sent_args));
            sent_args[0] = packet.route == MESH_ROUTE_FLOOD ? 1U : 0U;
            memcpy(&sent_args[1], &ack_code, 4U);
            {
                uint32_t timeout = packet.route == MESH_ROUTE_FLOOD ? 5000U : 1500U;
                memcpy(&sent_args[5], &timeout, 4U);
            }
            _companion_response(COMPANION_RESP_SENT, sent_args, sizeof(sent_args));
        } else {
            args[0] = stored_contact == NULL ? 2U : 3U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_CHANNEL_TEXT:
        if (length >= 8U && command[1] == 0U && node_state_get_channel(command[2], &node_channel)) {
            char text_buf[128U];
            size_t text_bytes = length - 7U;
            if (text_bytes >= sizeof(text_buf)) {
                text_bytes = sizeof(text_buf) - 1U;
            }
            memcpy(text_buf, &command[7], text_bytes);
            text_buf[text_bytes] = '\0';
            memcpy(&message_timestamp, &command[3], sizeof(message_timestamp));
            uint8_t scope_key[16U];
            node_state_get_default_scope(NULL, scope_key);
            if (_send_scope_active) {
                memcpy(scope_key, _send_scope_key, sizeof(scope_key));
            }
            if (mesh_datagram_group_text(
                    &node_channel, message_timestamp, node_state_get_name(), text_buf, &packet) &&
                (_send_unscoped || mesh_transport_apply_scope(&packet, scope_key)) &&
                mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                sx1262_transmit(wire, wire_length)) {
                ESP_LOGI(_companion_log_tag,
                         "group tx channel=%u route=%u code0=%04X code1=%04X hash=%02X payload=%u "
                         "wire=%u",
                         command[2],
                         packet.route,
                         packet.transport_codes[0],
                         packet.transport_codes[1],
                         packet.payload[0],
                         packet.payload_length,
                         (unsigned)wire_length);
                _companion_response(COMPANION_RESP_OK, NULL, 0U);
            } else {
                args[0] = 3U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_SELF_ADVERT:
        memset(&advert_options, 0, sizeof(advert_options));
        advert_options.role = MESH_ADVERT_ROLE_CHAT;
        advert_options.name = node_state_get_name();
        if (mesh_advert_build(storage_identity(),
                              node_state_get_time(),
                              &advert_options,
                              advert_payload,
                              sizeof(advert_payload),
                              &advert_length)) {
            memset(&packet, 0, sizeof(packet));
            packet.type = MESH_PAYLOAD_ADVERT;
            packet.route = length >= 2U && command[1] != 0U ? MESH_ROUTE_FLOOD : MESH_ROUTE_DIRECT;
            packet.version = 0U;
            packet.path_length = 0U;
            memcpy(packet.payload, advert_payload, advert_length);
            packet.payload_length = (uint8_t)advert_length;
            if (packet.route == MESH_ROUTE_FLOOD) {
                uint8_t scope_key[16U];
                node_state_get_default_scope(NULL, scope_key);
                mesh_transport_apply_scope(&packet, scope_key);
            }
            if (mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                sx1262_transmit(wire, wire_length)) {
                _companion_response(COMPANION_RESP_OK, NULL, 0U);
            } else {
                args[0] = 3U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = 3U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_GET_CONTACTS:
        memset(args, 0, sizeof(args));
        {
            uint32_t count = (uint32_t)node_state_contact_count();
            memcpy(args, &count, sizeof(count));
        }
        _contact_sync_last_modified = 0U;
        _companion_response(COMPANION_RESP_CONTACTS_START, args, sizeof(args));
        _contact_sync_index = 0U;
        _contact_sync_active = true;
        break;
    case COMPANION_CMD_SYNC_NEXT_MESSAGE:
        if (_message_waiting) {
            _companion_response(_message_is_channel ? COMPANION_RESP_CHANNEL_MSG_V3
                                                    : COMPANION_RESP_CONTACT_MSG_V3,
                                _message_args,
                                _message_length);
            _message_waiting = false;
        } else {
            _companion_response(COMPANION_RESP_NO_MORE_MESSAGES, NULL, 0U);
        }
        break;
    case COMPANION_CMD_GET_DEVICE_TIME: {
        uint32_t timestamp = node_state_get_time();
        memcpy(args, &timestamp, sizeof(timestamp));
    }
        _companion_response(COMPANION_RESP_CURRENT_TIME, args, sizeof(args));
        break;
    case COMPANION_CMD_GET_BATT_STORAGE: {
        uint8_t battery[10U] = {0};
        _companion_response(COMPANION_RESP_BATT_STORAGE, battery, sizeof(battery));
    } break;
    case COMPANION_CMD_GET_TUNING_PARAMS:
        memset(args, 0, sizeof(args));
        _companion_response(COMPANION_RESP_TUNING, (const uint8_t[8U]){0}, 8U);
        break;
    case COMPANION_CMD_GET_CUSTOM_VARS:
        _companion_response(COMPANION_RESP_CUSTOM_VARS, NULL, 0U);
        break;
    case COMPANION_CMD_SET_DEVICE_TIME:
        if (length >= 5U) {
            uint32_t timestamp;
            memcpy(&timestamp, &command[1], sizeof(timestamp));
            node_state_set_time(timestamp);
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            args[0] = 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_REBOOT:
        if (length >= 7U && memcmp(&command[1], "reboot", 6U) == 0) {
            _reboot_pending = true;
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_SET_RADIO_PARAMS:
        if (length >= 11U) {
            memset(&radio, 0, sizeof(radio));
            memcpy(&radio.frequency, &command[1], 4U);
            memcpy(&radio.bandwidth, &command[5], 4U);
            radio.spreading_factor = command[9];
            radio.coding_rate = command[10];
            radio.transmit_power = length >= 12U ? (int8_t)command[11] : 17;
            if (radio.frequency < 10000000U) {
                radio.frequency *= 1000U;
            }
            if (radio.bandwidth < 1000U) {
                radio.bandwidth *= 1000U;
            }
            ESP_LOGI(_companion_log_tag,
                     "radio params f=%" PRIu32 " bw=%" PRIu32 " sf=%u cr=%u len=%u",
                     radio.frequency,
                     radio.bandwidth,
                     radio.spreading_factor,
                     radio.coding_rate,
                     (unsigned)length);
            state_ok = node_state_set_radio(&radio);
            if (state_ok) {
                state_ok = sx1262_set_params(
                    radio.frequency, radio.bandwidth, radio.spreading_factor, radio.coding_rate);
            }
            _companion_response(state_ok ? COMPANION_RESP_OK : COMPANION_RESP_ERROR,
                                state_ok ? NULL : (const uint8_t[]){6U},
                                state_ok ? 0U : 1U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_SET_RADIO_TX_POWER:
        if (length >= 2U) {
            node_state_get_radio(&radio);
            radio.transmit_power = (int8_t)command[1];
            state_ok = radio.transmit_power >= -9 && radio.transmit_power <= 22 &&
                       node_state_set_radio(&radio);
            _companion_response(state_ok ? COMPANION_RESP_OK : COMPANION_RESP_ERROR,
                                state_ok ? NULL : (const uint8_t[]){6U},
                                state_ok ? 0U : 1U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_SET_PATH_HASH_MODE:
        if (length >= 3U && command[1] == 0U && node_state_set_path_hash_mode(command[2])) {
            storage_save_if_dirty();
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_SET_DEFAULT_SCOPE:
        if (length < 48U) {
            node_state_clear_default_scope();
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            size_t name_length = 0U;
            while (name_length < 31U && command[1U + name_length] != 0U) {
                name_length++;
            }
            if (name_length > 0U && name_length < 31U &&
                node_state_set_default_scope(&command[1], name_length, &command[32])) {
                ESP_LOGI(_companion_log_tag,
                         "scope set name=%.*s length=%u",
                         (int)name_length,
                         &command[1],
                         (unsigned)name_length);
                storage_save_if_dirty();
                _companion_response(COMPANION_RESP_OK, NULL, 0U);
            } else {
                _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
            }
        }
        break;
    case COMPANION_CMD_GET_DEFAULT_SCOPE: {
        uint8_t scope[47U] = {0};
        node_state_get_default_scope(scope, &scope[31]);
        _companion_response(
            COMPANION_RESP_DEFAULT_SCOPE, scope, scope[0] == 0U ? 0U : sizeof(scope));
    } break;
    case COMPANION_CMD_SET_ADVERT_LATLON:
        if (length >= 9U) {
            int32_t latitude;
            int32_t longitude;
            memcpy(&latitude, &command[1], 4U);
            memcpy(&longitude, &command[5], 4U);
            state_ok = node_state_set_location(latitude, longitude);
            _companion_response(state_ok ? COMPANION_RESP_OK : COMPANION_RESP_ERROR,
                                state_ok ? NULL : (const uint8_t[]){6U},
                                state_ok ? 0U : 1U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_SET_OTHER_PARAMS:
        if (length >= 2U) {
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            _companion_response(COMPANION_RESP_ERROR, (const uint8_t[]){6U}, 1U);
        }
        break;
    case COMPANION_CMD_GET_CONTACT_BY_KEY:
        stored_contact = length >= 33U ? node_state_find_contact(&command[1], 32U) : NULL;
        if (stored_contact != NULL) {
            size_t contact_length =
                _companion_contact_bytes(stored_contact, contact_data, sizeof(contact_data));
            _companion_response(COMPANION_RESP_CONTACT, contact_data, contact_length);
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_ADD_UPDATE_CONTACT:
        if (length >= 132U && _companion_parse_contact(&command[1], length - 1U, &contact) &&
            node_state_upsert_contact(&contact)) {
            storage_save_if_dirty();
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            args[0] = 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_REMOVE_CONTACT:
        if (length >= 33U && node_state_remove_contact(&command[1], 32U)) {
            storage_save_if_dirty();
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SET_ADVERT_NAME:
        if (length >= 2U && node_state_set_name(&command[1], length - 1U)) {
            storage_save_node_name(node_state_get_name());
            storage_save_if_dirty();
            ble_companion_update_device_name();
            _companion_response(COMPANION_RESP_OK, NULL, 0U);
        } else {
            args[0] = 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_GET_CHANNEL:
        if (length >= 2U && command[1] < NODE_STATE_MAX_CHANNELS) {
            channel[0] = command[1];
            if (node_state_get_channel(command[1], &node_channel)) {
                memcpy(&channel[1], node_channel.name, 32U);
                memcpy(&channel[33], node_channel.secret, 16U);
            } else {
                memset(&channel[1], 0, 48U);
            }
            _companion_response(COMPANION_RESP_CHANNEL_INFO, channel, sizeof(channel));
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SET_CHANNEL:
        if (length >= 18U && command[1] < NODE_STATE_MAX_CHANNELS) {
            size_t name_len = length > 50U ? 32U : (length - 18U > 32U ? 32U : length - 18U);
            memset(&node_channel, 0, sizeof(node_channel));
            if (name_len > 0U) {
                memcpy(node_channel.name, &command[2], name_len > 31U ? 31U : name_len);
            }
            memcpy(node_channel.secret, &command[length - 16U], 16U);
            if (node_state_set_channel(command[1], &node_channel)) {
                storage_save_if_dirty();
                _companion_response(COMPANION_RESP_OK, NULL, 0U);
            } else {
                args[0] = 6U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_TRACE_PATH:
        if (length >= 10U && length - 10U <= MESH_PACKET_MAX_PATH) {
            uint8_t path_len = (uint8_t)(length - 10U);
            uint8_t flags = command[9];
            uint8_t path_sz = flags & 3U;
            if ((path_len >> path_sz) > MESH_PACKET_MAX_PATH ||
                (path_len % (1U << path_sz)) != 0U) {
                args[0] = 6U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            } else {
                uint32_t tag;
                uint32_t auth_code;
                uint32_t est_timeout;
                memcpy(&tag, &command[1], 4U);
                memcpy(&auth_code, &command[5], 4U);
                memset(&packet, 0, sizeof(packet));
                packet.type = MESH_PAYLOAD_TRACE;
                packet.route = MESH_ROUTE_DIRECT;
                packet.version = 0U;
                packet.path_length = 0U;
                memcpy(&packet.payload[0], &tag, 4U);
                memcpy(&packet.payload[4], &auth_code, 4U);
                packet.payload[8] = flags;
                if (path_len > 0U) {
                    memcpy(&packet.payload[9], &command[10], path_len);
                }
                packet.payload_length = (uint8_t)(9U + path_len);
                if (mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                    sx1262_transmit(wire, wire_length)) {
                    est_timeout = 1500U + (uint32_t)(path_len >> path_sz) * 500U;
                    sent_args[0] = 0U;
                    memcpy(&sent_args[1], &tag, 4U);
                    memcpy(&sent_args[5], &est_timeout, 4U);
                    _companion_response(COMPANION_RESP_SENT, sent_args, sizeof(sent_args));
                } else {
                    args[0] = 3U;
                    _companion_response(COMPANION_RESP_ERROR, args, 1U);
                }
            }
        } else {
            args[0] = 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_STATUS_REQ:
        stored_contact = length >= 33U ? node_state_find_contact(&command[1], 32U) : NULL;
        if (stored_contact == NULL && length >= 7U) {
            stored_contact = node_state_find_contact(&command[1], 6U);
        }
        if (stored_contact != NULL) {
            uint8_t req_data[9U] = {1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
            uint32_t tag = node_state_get_time();
            uint32_t est_timeout;
            state_ok = mesh_datagram_request(
                storage_identity(), stored_contact, tag, req_data, sizeof(req_data), &packet);
            if (state_ok && packet.route == MESH_ROUTE_FLOOD) {
                uint8_t scope_key[16U];
                node_state_get_default_scope(NULL, scope_key);
                mesh_transport_apply_scope(&packet, scope_key);
            }
            if (state_ok && mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                sx1262_transmit(wire, wire_length)) {
                est_timeout = packet.route == MESH_ROUTE_FLOOD ? 5000U : 2000U;
                sent_args[0] = packet.route == MESH_ROUTE_FLOOD ? 1U : 0U;
                memcpy(&sent_args[1], &tag, 4U);
                memcpy(&sent_args[5], &est_timeout, 4U);
                _companion_response(COMPANION_RESP_SENT, sent_args, sizeof(sent_args));
            } else {
                args[0] = 3U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_BINARY_REQ:
        stored_contact = length >= 34U ? node_state_find_contact(&command[1], 32U) : NULL;
        if (stored_contact == NULL && length >= 7U) {
            stored_contact = node_state_find_contact(&command[1], 6U);
        }
        if (stored_contact != NULL && length > 33U) {
            uint32_t tag = node_state_get_time();
            uint32_t est_timeout;
            state_ok = mesh_datagram_request(storage_identity(),
                                             stored_contact,
                                             tag,
                                             &command[33],
                                             length - 33U,
                                             &packet);
            if (state_ok && packet.route == MESH_ROUTE_FLOOD) {
                uint8_t scope_key[16U];
                node_state_get_default_scope(NULL, scope_key);
                mesh_transport_apply_scope(&packet, scope_key);
            }
            if (state_ok && mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                sx1262_transmit(wire, wire_length)) {
                est_timeout = packet.route == MESH_ROUTE_FLOOD ? 5000U : 2000U;
                sent_args[0] = packet.route == MESH_ROUTE_FLOOD ? 1U : 0U;
                memcpy(&sent_args[1], &tag, 4U);
                memcpy(&sent_args[5], &est_timeout, 4U);
                _companion_response(COMPANION_RESP_SENT, sent_args, sizeof(sent_args));
            } else {
                args[0] = 3U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = stored_contact == NULL ? 2U : 6U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    case COMPANION_CMD_SEND_LOGIN:
        stored_contact = length >= 33U ? node_state_find_contact(&command[1], 32U) : NULL;
        if (stored_contact == NULL && length >= 7U) {
            stored_contact = node_state_find_contact(&command[1], 6U);
        }
        if (stored_contact != NULL) {
            uint8_t login_buf[20U] = {0};
            size_t pass_len = length > 33U ? length - 33U : 0U;
            uint32_t tag = node_state_get_time();
            uint32_t est_timeout;
            if (pass_len > 15U) {
                pass_len = 15U;
            }
            if (pass_len > 0U) {
                memcpy(login_buf, &command[33], pass_len);
            }
            state_ok = mesh_datagram_request(
                storage_identity(), stored_contact, tag, login_buf, pass_len, &packet);
            if (state_ok && packet.route == MESH_ROUTE_FLOOD) {
                uint8_t scope_key[16U];
                node_state_get_default_scope(NULL, scope_key);
                mesh_transport_apply_scope(&packet, scope_key);
            }
            if (state_ok && mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length) &&
                sx1262_transmit(wire, wire_length)) {
                est_timeout = packet.route == MESH_ROUTE_FLOOD ? 5000U : 2000U;
                sent_args[0] = packet.route == MESH_ROUTE_FLOOD ? 1U : 0U;
                memcpy(&sent_args[1], stored_contact->public_key, 4U);
                memcpy(&sent_args[5], &est_timeout, 4U);
                _companion_response(COMPANION_RESP_SENT, sent_args, sizeof(sent_args));
            } else {
                args[0] = 3U;
                _companion_response(COMPANION_RESP_ERROR, args, 1U);
            }
        } else {
            args[0] = 2U;
            _companion_response(COMPANION_RESP_ERROR, args, 1U);
        }
        break;
    default:
        args[0] = 1U;
        _companion_response(COMPANION_RESP_ERROR, args, 1U);
        break;
    }
}

/**
 * @brief Consume arbitrary BLE chunks and dispatch complete companion frames.
 *
 * Checks for binary framing '<' or raw packet mode and delivers extracted commands.
 *
 * @param data Pointer to input data bytes.
 * @param length Number of input data bytes.
 * @return void
 */
void companion_receive(const uint8_t *data, size_t length) {
    size_t index = 0U;
    uint16_t declared;
    size_t frame_length;
    if (data == NULL) {
        return;
    }
    if (length > 0U && data[0] != '<') {
        _raw_ble_mode = true;
        _companion_command(data, length);
        return;
    }
    while (index < length) {
        if (_rx_length == 0U) {
            if (data[index++] != '<') {
                continue;
            }
            _rx_frame[_rx_length++] = '<';
        }
        while (index < length && _rx_length < 3U) {
            _rx_frame[_rx_length++] = data[index++];
        }
        if (_rx_length < 3U) {
            return;
        }
        declared = (uint16_t)_rx_frame[1] | ((uint16_t)_rx_frame[2] << 8U);
        frame_length = declared + 3U;
        if (frame_length > sizeof(_rx_frame) || frame_length < 4U) {
            _rx_length = 0U;
            continue;
        }
        while (index < length && _rx_length < frame_length) {
            _rx_frame[_rx_length++] = data[index++];
        }
        if (_rx_length == frame_length) {
            _companion_command(&_rx_frame[3], declared);
            _rx_length = 0U;
        }
    }
}

/**
 * @brief Return and clear the deferred reboot request.
 *
 * @param void No parameters.
 * @return bool true if reboot was flagged, false otherwise.
 */
bool companion_take_reboot_request(void) {
    bool pending = _reboot_pending;
    _reboot_pending = false;
    return pending;
}

/**
 * @brief Return and clear the next complete response frame.
 *
 * @param data Destination buffer for response frame bytes.
 * @param capacity Capacity of destination buffer in bytes.
 * @return size_t Length of copied frame in bytes, or 0 if queue empty or buffer too small.
 */
size_t companion_next_response(uint8_t *data, size_t capacity) {
    CompanionTxSlot *slot;
    size_t length;
    if (data == NULL || _tx_count == 0U) {
        return 0U;
    }
    slot = &_tx_queue[_tx_head];
    length = slot->length;
    if (capacity < length) {
        return 0U;
    }
    memcpy(data, slot->data, length);
    _tx_head = (_tx_head + 1U) % COMPANION_TX_QUEUE_CAPACITY;
    _tx_count--;
    if (_contact_sync_active) {
        _companion_contact_sync_next();
    }
    return length;
}

/**
 * @brief Queue a raw received packet log notification for the connected app.
 *
 * @param snr_x4 SNR times four as a signed 8-bit integer.
 * @param rssi RSSI in dBm as a signed 8-bit integer.
 * @param raw Pointer to raw packet wire bytes.
 * @param length Number of raw bytes.
 * @return void
 */
void companion_push_rx_log(int8_t snr_x4, int8_t rssi, const uint8_t *raw, size_t length) {
    uint8_t payload[COMPANION_MAX_FRAME];
    if (raw == NULL || length == 0U || length + 2U > sizeof(payload)) {
        return;
    }
    payload[0] = (uint8_t)snr_x4;
    payload[1] = (uint8_t)rssi;
    memcpy(&payload[2], raw, length);
    _companion_response(COMPANION_PUSH_LOG_RX_DATA, payload, length + 2U);
}

/**
 * @brief Queue a MeshCore contact push notification.
 *
 * @param code Response opcode (must be >= 0x80).
 * @param contact Pointer to NodeContact structure.
 * @return void
 */
void companion_push_contact(uint8_t code, const NodeContact *contact) {
    uint8_t data[147U];
    if (contact == NULL || code < 0x80U ||
        _companion_contact_bytes(contact, data, sizeof(data)) == 0U) {
        return;
    }
    _companion_response(code, data, sizeof(data));
}

/**
 * @brief Queue a received text message and notify a connected app.
 *
 * @param contact Pointer to sender NodeContact.
 * @param packet Pointer to received MeshPacket.
 * @param timestamp Message timestamp.
 * @param text Null-terminated text string.
 * @return void
 */
void companion_deliver_text(const NodeContact *contact,
                            const MeshPacket *packet,
                            uint32_t timestamp,
                            const char *text) {
    size_t text_length;
    size_t position = 0U;
    if (contact == NULL || packet == NULL || text == NULL) {
        return;
    }
    text_length = strnlen(text, sizeof(_message_args) - 16U);
    if (text_length == 0U || text_length > sizeof(_message_args) - 16U) {
        return;
    }
    _message_args[position++] = 0;
    _message_args[position++] = 0;
    _message_args[position++] = 0;
    memcpy(&_message_args[position], contact->public_key, 6U);
    position += 6U;
    _message_args[position++] = packet->route == MESH_ROUTE_FLOOD ? packet->path_length : 0xFFU;
    _message_args[position++] = 0U;
    memcpy(&_message_args[position], &timestamp, 4U);
    position += 4U;
    memcpy(&_message_args[position], text, text_length);
    position += text_length;
    _message_length = position;
    _message_is_channel = false;
    _message_waiting = true;
    _companion_response(COMPANION_PUSH_MESSAGE_WAITING, NULL, 0U);
}

/**
 * @brief Queue a received channel message in the v3 companion format.
 *
 * @param channel_index Channel slot index.
 * @param packet Pointer to received MeshPacket.
 * @param timestamp Message timestamp.
 * @param text Null-terminated text string.
 * @return void
 */
void companion_deliver_channel(uint8_t channel_index,
                               const MeshPacket *packet,
                               uint32_t timestamp,
                               const char *text) {
    size_t text_length;
    size_t position = 0U;
    if (packet == NULL || text == NULL) {
        return;
    }
    text_length = strnlen(text, sizeof(_message_args) - 12U);
    if (text_length == 0U) {
        return;
    }
    _message_args[position++] = 0U;
    _message_args[position++] = 0U;
    _message_args[position++] = 0U;
    _message_args[position++] = channel_index;
    _message_args[position++] =
        (packet->route == MESH_ROUTE_FLOOD || packet->route == MESH_ROUTE_TRANSPORT_FLOOD)
            ? packet->path_length
            : 0xFFU;
    _message_args[position++] = 0U;
    memcpy(&_message_args[position], &timestamp, 4U);
    position += 4U;
    memcpy(&_message_args[position], text, text_length);
    position += text_length;
    _message_length = position;
    _message_is_channel = true;
    _message_waiting = true;
    _companion_response(COMPANION_PUSH_MESSAGE_WAITING, NULL, 0U);
}

/**
 * @brief Queue a trace data push notification for the connected app.
 *
 * @param tag 32-bit trace identifier tag.
 * @param auth_code 32-bit trace authentication code.
 * @param flags Trace flags including hop hash size.
 * @param path_hashes Pointer to path hash bytes.
 * @param path_len Length of path hash bytes.
 * @param path_snrs Pointer to SNR bytes recorded per hop.
 * @param last_snr SNR of final hop to this node (snr * 4).
 * @return void
 */
void companion_push_trace_data(uint32_t tag,
                               uint32_t auth_code,
                               uint8_t flags,
                               const uint8_t *path_hashes,
                               uint8_t path_len,
                               const uint8_t *path_snrs,
                               int8_t last_snr) {
    uint8_t frame[COMPANION_MAX_FRAME];
    uint8_t path_sz = flags & 3U;
    size_t snr_count = (size_t)(path_len >> path_sz);
    size_t total_payload = 11U + (size_t)path_len + snr_count + 1U;
    size_t pos = 0U;
    if (total_payload > sizeof(frame)) {
        return;
    }
    frame[pos++] = 0U;
    frame[pos++] = path_len;
    frame[pos++] = flags;
    memcpy(&frame[pos], &tag, 4U);
    pos += 4U;
    memcpy(&frame[pos], &auth_code, 4U);
    pos += 4U;
    if (path_len > 0U && path_hashes != NULL) {
        memcpy(&frame[pos], path_hashes, path_len);
        pos += path_len;
    }
    if (snr_count > 0U && path_snrs != NULL) {
        memcpy(&frame[pos], path_snrs, snr_count);
        pos += snr_count;
    }
    frame[pos++] = (uint8_t)last_snr;
    _companion_response(COMPANION_PUSH_TRACE_DATA, frame, pos);
}

/**
 * @brief Queue a status response push notification for the connected app.
 *
 * @param pub_key Pointer to 32-byte public key of the responding contact.
 * @param data Pointer to status response data payload.
 * @param length Length of status response data.
 * @return void
 */
void companion_push_status_response(const uint8_t *pub_key,
                                    const uint8_t *data,
                                    size_t length) {
    uint8_t frame[COMPANION_MAX_FRAME];
    size_t total_payload;
    size_t pos = 0U;
    if (pub_key == NULL || data == NULL || length == 0U) {
        return;
    }
    total_payload = 7U + length;
    if (total_payload > sizeof(frame)) {
        length = sizeof(frame) - 7U;
    }
    frame[pos++] = 0U;
    memcpy(&frame[pos], pub_key, 6U);
    pos += 6U;
    memcpy(&frame[pos], data, length);
    pos += length;
    _companion_response(COMPANION_PUSH_STATUS_RESPONSE, frame, pos);
}