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
// File:    datagram.h
// Desc:    Defines encrypted MeshCore direct and flood datagram construction.
// Created: 2026

#ifndef DATAGRAM_H
#define DATAGRAM_H

#include "identity.h"
#include "mesh_packet.h"
#include "node_state.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Build one encrypted MeshCore direct text packet.
 *
 * Derives ECDH shared secret with destination contact, encrypts plaintext
 * with AES-128 and 2-byte MAC, and prepares routing and ack hash.
 *
 * @param identity Pointer to local valid identity.
 * @param contact Pointer to destination NodeContact.
 * @param timestamp Epoch timestamp in seconds.
 * @param attempt Transmission attempt sequence number.
 * @param text Null-terminated text string to send.
 * @param packet Destination MeshPacket pointer.
 * @param ack_code Destination pointer for calculated ack verification code.
 * @return bool true on success, false if parameters invalid or encryption fails.
 */
bool mesh_datagram_text(const MeshIdentity *identity,
                        const NodeContact *contact,
                        uint32_t timestamp,
                        uint8_t attempt,
                        const char *text,
                        MeshPacket *packet,
                        uint32_t *ack_code);

/**
 * @brief Build one encrypted MeshCore group-text flood packet.
 *
 * Formats "sender_name: text", encrypts payload with channel secret key,
 * and sets packet envelope to MESH_PAYLOAD_GROUP_TEXT and MESH_ROUTE_FLOOD.
 *
 * @param channel Pointer to target NodeChannel.
 * @param timestamp Epoch timestamp in seconds.
 * @param sender_name Null-terminated sender display name.
 * @param text Null-terminated message text string.
 * @param packet Destination MeshPacket pointer.
 * @return bool true on success, false if parameters invalid or buffer exceeded.
 */
bool mesh_datagram_group_text(const NodeChannel *channel,
                              uint32_t timestamp,
                              const char *sender_name,
                              const char *text,
                              MeshPacket *packet);

/**
 * @brief Build one encrypted MeshCore request packet.
 *
 * Derives ECDH shared secret with destination contact, encrypts request payload
 * prefixed with 4-byte tag, and sets packet envelope to MESH_PAYLOAD_REQUEST.
 *
 * @param identity Pointer to local valid identity.
 * @param contact Pointer to destination NodeContact.
 * @param tag 32-bit request identification tag.
 * @param req_data Pointer to request payload bytes.
 * @param req_data_len Number of request bytes.
 * @param packet Destination MeshPacket pointer.
 * @return bool true on success, false if parameters invalid or encryption fails.
 */
bool mesh_datagram_request(const MeshIdentity *identity,
                           const NodeContact *contact,
                           uint32_t tag,
                           const uint8_t *req_data,
                           size_t req_data_len,
                           MeshPacket *packet);

#endif // DATAGRAM_H