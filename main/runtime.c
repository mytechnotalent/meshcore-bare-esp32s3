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
// File:    runtime.c
// Desc:    Implements the MeshCore radio and persistence runtime task.
// Created: 2026

#include "runtime.h"

#include "advert.h"
#include "ble_companion.h"
#include "companion.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "mesh_crypto.h"
#include "mesh_packet.h"
#include "mesh_transport.h"
#include "node_state.h"
#include "storage.h"
#include "sx1262.h"

#include <string.h>

/**
 * @brief Runtime log tag.
 */
static const char *_runtime_log_tag = "runtime";

/**
 * @brief Broadcast self advert across the mesh.
 *
 * Constructs and signs a local chat advert packet, applies default flood
 * scope, encodes it, and transmits it via the SX1262 LoRa transceiver.
 *
 * @param void No parameters.
 * @return void
 */
static void _runtime_send_boot_advert(void) {
    uint8_t advert_payload[MESH_ADVERT_MAX_PAYLOAD];
    size_t advert_length;
    MeshAdvertOptions advert_options;
    MeshPacket packet;
    uint8_t wire[255U];
    size_t wire_length;
    uint8_t scope_key[16U];
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
        packet.route = MESH_ROUTE_FLOOD;
        packet.version = 0U;
        packet.path_length = 0U;
        memcpy(packet.payload, advert_payload, advert_length);
        packet.payload_length = (uint8_t)advert_length;
        node_state_get_default_scope(NULL, scope_key);
        mesh_transport_apply_scope(&packet, scope_key);
        if (mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length)) {
            sx1262_transmit(wire, wire_length);
            ESP_LOGI(_runtime_log_tag, "broadcast boot advert wire=%u", (unsigned)wire_length);
        }
    }
}

/**
 * @brief Poll radio traffic and persist state.
 *
 * FreeRTOS task function running continuously to poll incoming LoRa frames,
 * parse adverts, decrypt direct/channel messages, deliver them to companion BLE,
 * and handle pending reboot requests.
 *
 * @param parameter FreeRTOS task argument pointer (unused).
 * @return void
 */
static void _runtime_task(void *parameter) {
    uint8_t received[255U];
    MeshPacket packet;
    NodeContact contact;
    NodeContact *sender;
    NodeChannel *channel;
    uint8_t local_hash;
    uint8_t shared_secret[32U];
    uint8_t plain[184U];
    size_t plain_length;
    uint32_t message_timestamp;
    size_t received_length;
    size_t my_name_len;
    uint32_t boot_cycles = 0U;
    bool boot_advert_sent = false;
    uint32_t trace_tag;
    uint32_t trace_auth;
    uint8_t trace_flags;
    uint8_t trace_path_sz;
    uint8_t trace_path_len;
    size_t trace_offset;
    (void)parameter;
    while (true) {
        storage_save_if_dirty();
        if (!boot_advert_sent && node_state_get_time() > 0U) {
            boot_cycles++;
            if (boot_cycles > 50U) {
                boot_advert_sent = true;
                _runtime_send_boot_advert();
            }
        }
        received_length = sx1262_receive(received, sizeof(received));
        if (received_length > 0U) {
            ESP_LOGI(_runtime_log_tag,
                     "radio rx wire=%u rssi=%d snr=%d",
                     (unsigned)received_length,
                     (int)sx1262_last_rssi(),
                     (int)(sx1262_last_snr_x4() / 4));
            companion_push_rx_log(
                sx1262_last_snr_x4(), sx1262_last_rssi(), received, received_length);
        }
        if (received_length > 0U && mesh_packet_decode(received, received_length, &packet)) {
            ESP_LOGI(_runtime_log_tag,
                     "decoded type=%u route=%u path_len=%u payload_len=%u",
                     (unsigned)packet.type,
                     (unsigned)packet.route,
                     (unsigned)packet.path_length,
                     (unsigned)packet.payload_length);
            if (packet.type == MESH_PAYLOAD_ADVERT) {
                if (mesh_advert_parse(
                        storage_identity(), packet.payload, packet.payload_length, &contact)) {
                    contact.out_path_length = packet.path_length;
                    memcpy(contact.out_path, packet.path, sizeof(contact.out_path));
                    if (node_state_upsert_contact(&contact)) {
                        ESP_LOGI(_runtime_log_tag,
                                 "contact stored: name=%s type=%u",
                                 contact.name,
                                 (unsigned)contact.type);
                        companion_push_contact(0x8AU, &contact);
                        storage_save_if_dirty();
                    }
                } else {
                    ESP_LOGW(_runtime_log_tag,
                             "advert parse failed payload_len=%u",
                             (unsigned)packet.payload_length);
                }
            }
            if (packet.type == MESH_PAYLOAD_TEXT && packet.payload_length > 4U &&
                mesh_crypto_sha256(storage_identity()->public_key, 32U, &local_hash, 1U) &&
                packet.payload[0] == local_hash) {
                sender = node_state_find_contact(&packet.payload[1], 1U);
                if (sender != NULL &&
                    mesh_identity_shared_secret(
                        storage_identity(), sender->public_key, shared_secret) &&
                    mesh_crypto_mac_then_decrypt(shared_secret,
                                                 &packet.payload[2],
                                                 packet.payload_length - 2U,
                                                 plain,
                                                 sizeof(plain),
                                                 &plain_length) &&
                    plain_length > 5U) {
                    memcpy(&message_timestamp, plain, sizeof(message_timestamp));
                    plain[plain_length - 1U] = 0U;
                    companion_deliver_text(
                        sender, &packet, message_timestamp, (const char *)&plain[5]);
                }
            }
            if ((packet.type == MESH_PAYLOAD_GROUP_TEXT ||
                 packet.type == MESH_PAYLOAD_GROUP_DATA) &&
                packet.payload_length > 3U &&
                (channel = node_state_find_channel_hash(packet.payload[0])) != NULL &&
                mesh_crypto_mac_then_decrypt_key(channel->secret,
                                                 sizeof(channel->secret),
                                                 &packet.payload[1],
                                                 packet.payload_length - 1U,
                                                 plain,
                                                 sizeof(plain),
                                                 &plain_length) &&
                plain_length > 5U) {
                memcpy(&message_timestamp, plain, sizeof(message_timestamp));
                plain[plain_length - 1U] = 0U;
                my_name_len = strlen(node_state_get_name());
                if (strncmp((const char *)&plain[5], node_state_get_name(), my_name_len) == 0 &&
                    plain[5U + my_name_len] == ':') {
                    ESP_LOGI(_runtime_log_tag,
                             "suppressing self echo wire=%u hops=%u",
                             (unsigned)received_length,
                             (unsigned)packet.path_length);
                } else {
                    companion_deliver_channel(node_state_channel_index(channel),
                                              &packet,
                                              message_timestamp,
                                              (const char *)&plain[5]);
                }
            }
            if (packet.type == MESH_PAYLOAD_TRACE && packet.payload_length >= 9U) {
                memcpy(&trace_tag, &packet.payload[0], 4U);
                memcpy(&trace_auth, &packet.payload[4], 4U);
                trace_flags = packet.payload[8];
                trace_path_sz = trace_flags & 3U;
                trace_path_len = packet.payload_length - 9U;
                trace_offset = (size_t)packet.path_length << trace_path_sz;
                if (trace_offset >= trace_path_len) {
                    companion_push_trace_data(trace_tag,
                                              trace_auth,
                                              trace_flags,
                                              &packet.payload[9],
                                              trace_path_len,
                                              packet.path,
                                              sx1262_last_snr_x4());
                }
            }
            if (packet.type == MESH_PAYLOAD_RESPONSE && packet.payload_length > 4U &&
                mesh_crypto_sha256(storage_identity()->public_key, 32U, &local_hash, 1U) &&
                packet.payload[0] == local_hash) {
                sender = node_state_find_contact(&packet.payload[1], 1U);
                if (sender != NULL &&
                    mesh_identity_shared_secret(
                        storage_identity(), sender->public_key, shared_secret) &&
                    mesh_crypto_mac_then_decrypt(shared_secret,
                                                 &packet.payload[2],
                                                 packet.payload_length - 2U,
                                                 plain,
                                                 sizeof(plain),
                                                 &plain_length) &&
                    plain_length > 4U) {
                    companion_push_status_response(
                        sender->public_key, &plain[4], plain_length - 4U);
                }
            }
        }
        ble_companion_flush_pending();
        if (companion_take_reboot_request()) {
            storage_save_node_name(node_state_get_name());
            storage_save_if_dirty();
            vTaskDelay(pdMS_TO_TICKS(250U));
            esp_restart();
        }
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

/**
 * @brief Start the MeshCore radio and persistence runtime task.
 *
 * Creates the FreeRTOS background task "mesh_runtime" with 8KB stack and priority 5.
 *
 * @param void No parameters.
 * @return void
 */
void runtime_start(void) {
    xTaskCreate(_runtime_task, "mesh_runtime", 8192U, NULL, 5U, NULL);
}
