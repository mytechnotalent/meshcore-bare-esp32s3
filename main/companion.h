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
// File:    companion.h
// Desc:    Defines the framed MeshCore companion-radio BLE service interface.
// Created: 2026

#ifndef COMPANION_H
#define COMPANION_H

#include "mesh_packet.h"
#include "node_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Process bytes written by a MeshCore client.
 *
 * Dispatches commands including APP_START, SEND_TEXT, SEND_CHANNEL_TEXT,
 * GET_CONTACTS, SET_RADIO_PARAMS, SET_ADVERT_NAME, etc.
 *
 * @param data Pointer to received command buffer bytes.
 * @param length Number of command bytes received.
 * @return void
 */
void companion_receive(const uint8_t *data, size_t length);

/**
 * @brief Return and clear the deferred reboot request.
 *
 * @param void No parameters.
 * @return bool true if a reboot was requested, false otherwise.
 */
bool companion_take_reboot_request(void);

/**
 * @brief Return the next response frame for BLE notification.
 *
 * Dequeues one framed response packet from the companion ring buffer.
 *
 * @param data Destination buffer for response frame.
 * @param capacity Capacity of destination buffer in bytes.
 * @return size_t Length of dequeued response frame in bytes, or 0 if empty.
 */
size_t companion_next_response(uint8_t *data, size_t capacity);

/**
 * @brief Queue a contact push notification for the connected app.
 *
 * @param code Response code identifier.
 * @param contact Pointer to NodeContact structure.
 * @return void
 */
void companion_push_contact(uint8_t code, const NodeContact *contact);

/**
 * @brief Queue a received text message for app synchronization.
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
                            const char *text);

/**
 * @brief Queue a received channel message for app synchronization.
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
                               const char *text);

/**
 * @brief Queue a raw received packet log notification for the connected app.
 *
 * @param snr_x4 Signal-to-noise ratio times four.
 * @param rssi Received signal strength indicator in dBm.
 * @param raw Pointer to raw received packet bytes.
 * @param length Length of raw packet bytes.
 * @return void
 */
void companion_push_rx_log(int8_t snr_x4, int8_t rssi, const uint8_t *raw, size_t length);

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
                               int8_t last_snr);

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
                                    size_t length);

#ifdef __cplusplus
}
#endif

#endif // COMPANION_H