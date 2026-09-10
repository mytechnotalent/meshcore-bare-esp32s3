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
// File:    datagram.c
// Desc:    Builds MeshCore encrypted text datagrams using contact shared secrets.
// Created: 2026

#include "datagram.h"

#include "mesh_crypto.h"

#include <string.h>

/**
 * @brief Maximum MeshCore text length in bytes.
 */
#define MESH_DATAGRAM_MAX_TEXT 127U

/**
 * @brief Build a one-byte identity hash from a public key.
 *
 * Computes SHA-256 over 32-byte public key and returns the first byte.
 *
 * @param public_key Pointer to 32-byte public key.
 * @param hash Destination pointer for 1-byte hash.
 * @return bool true on success, false on invalid input.
 */
static bool _mesh_datagram_hash(const uint8_t *public_key, uint8_t *hash) {
    return mesh_crypto_sha256(public_key, 32U, hash, 1U);
}

/**
 * @brief Build and encrypt a MeshCore text message for one contact.
 *
 * Derives ECDH shared secret, computes destination/source single-byte hashes,
 * encrypts message with AES-128 and 2-byte MAC, and calculates ack verification hash.
 *
 * @param identity Pointer to local valid identity.
 * @param contact Pointer to destination NodeContact.
 * @param timestamp Epoch timestamp in seconds.
 * @param attempt Transmission attempt sequence number.
 * @param text Null-terminated text string to send.
 * @param packet Destination MeshPacket pointer.
 * @param ack_code Destination pointer for calculated ack code.
 * @return bool true on success, false if parameters invalid or encryption fails.
 */
bool mesh_datagram_text(const MeshIdentity *identity,
                        const NodeContact *contact,
                        uint32_t timestamp,
                        uint8_t attempt,
                        const char *text,
                        MeshPacket *packet,
                        uint32_t *ack_code) {
    uint8_t secret[32U];
    uint8_t destination_hash;
    uint8_t source_hash;
    uint8_t plain[5U + MESH_DATAGRAM_MAX_TEXT];
    uint8_t encrypted[184U];
    uint8_t ack_data[5U + MESH_DATAGRAM_MAX_TEXT + 32U];
    size_t text_length;
    size_t encrypted_length;
    size_t plain_length;
    size_t path_bytes;
    if (identity == NULL || contact == NULL || text == NULL || packet == NULL || ack_code == NULL ||
        !identity->valid) {
        return false;
    }
    text_length = strnlen(text, MESH_DATAGRAM_MAX_TEXT);
    if (text_length == 0U || text_length > MESH_DATAGRAM_MAX_TEXT) {
        return false;
    }
    if (!_mesh_datagram_hash(contact->public_key, &destination_hash) ||
        !_mesh_datagram_hash(identity->public_key, &source_hash) ||
        !mesh_identity_shared_secret(identity, contact->public_key, secret)) {
        return false;
    }
    memcpy(plain, &timestamp, 4U);
    plain[4] = attempt & 3U;
    memcpy(&plain[5], text, text_length + 1U);
    plain_length = 6U + text_length;
    if (!mesh_crypto_encrypt_then_mac(
            secret, plain, plain_length, encrypted, sizeof(encrypted), &encrypted_length)) {
        return false;
    }
    memcpy(ack_data, plain, 5U + text_length);
    memcpy(&ack_data[5U + text_length], identity->public_key, 32U);
    if (!mesh_crypto_sha256(ack_data, 37U + text_length, (uint8_t *)ack_code, 4U)) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->type = MESH_PAYLOAD_TEXT;
    packet->version = 0U;
    packet->payload[0] = destination_hash;
    packet->payload[1] = source_hash;
    memcpy(&packet->payload[2], encrypted, encrypted_length);
    packet->payload_length = (uint8_t)(2U + encrypted_length);
    if (contact->out_path_length < 0) {
        packet->route = MESH_ROUTE_FLOOD;
        packet->path_length = 0U;
    } else {
        packet->route = MESH_ROUTE_DIRECT;
        packet->path_length = (uint8_t)contact->out_path_length;
        path_bytes =
            (size_t)(packet->path_length & 63U) * (size_t)((packet->path_length >> 6U) + 1U);
        if (path_bytes > sizeof(packet->path)) {
            return false;
        }
        memcpy(packet->path, contact->out_path, path_bytes);
    }
    return true;
}

/**
 * @brief Build one encrypted MeshCore group-text flood packet.
 *
 * Formats "[sender_name]: [text]", computes 1-byte channel secret hash,
 * encrypts message with channel secret key, and configures group packet envelope.
 *
 * @param channel Pointer to target NodeChannel.
 * @param timestamp Epoch timestamp in seconds.
 * @param sender_name Null-terminated sender display name.
 * @param text Null-terminated message text string.
 * @param packet Destination MeshPacket pointer.
 * @return bool true on success, false if parameters invalid or capacity exceeded.
 */
bool mesh_datagram_group_text(const NodeChannel *channel,
                              uint32_t timestamp,
                              const char *sender_name,
                              const char *text,
                              MeshPacket *packet) {
    uint8_t plain[184U];
    uint8_t encrypted[184U];
    uint8_t channel_hash;
    size_t name_length;
    size_t text_length;
    size_t plain_length;
    size_t encrypted_length;
    if (channel == NULL || !channel->configured || sender_name == NULL || text == NULL ||
        packet == NULL) {
        return false;
    }
    name_length = strnlen(sender_name, 32U);
    text_length = strnlen(text, MESH_DATAGRAM_MAX_TEXT);
    if (text_length == 0U || name_length + text_length + 3U > sizeof(plain)) {
        return false;
    }
    if (!mesh_crypto_sha256(channel->secret, sizeof(channel->secret), &channel_hash, 1U)) {
        return false;
    }
    memcpy(plain, &timestamp, 4U);
    plain[4] = 0U;
    memcpy(&plain[5], sender_name, name_length);
    plain[5U + name_length] = ':';
    plain[6U + name_length] = ' ';
    memcpy(&plain[7U + name_length], text, text_length + 1U);
    plain_length = 8U + name_length + text_length;
    if (!mesh_crypto_encrypt_then_mac_key(channel->secret,
                                          sizeof(channel->secret),
                                          plain,
                                          plain_length,
                                          encrypted,
                                          sizeof(encrypted),
                                          &encrypted_length)) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->type = MESH_PAYLOAD_GROUP_TEXT;
    packet->route = MESH_ROUTE_FLOOD;
    packet->payload[0] = channel_hash;
    memcpy(&packet->payload[1], encrypted, encrypted_length);
    packet->payload_length = (uint8_t)(encrypted_length + 1U);
    return true;
}

/**
 * @brief Build and encrypt a MeshCore request datagram for one contact.
 *
 * Derives ECDH shared secret, encrypts request payload prefixed with 4-byte tag,
 * and sets packet envelope to MESH_PAYLOAD_REQUEST with appropriate direct or flood route.
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
                           MeshPacket *packet) {
    uint8_t secret[32U];
    uint8_t destination_hash;
    uint8_t source_hash;
    uint8_t plain[4U + MESH_PACKET_MAX_PAYLOAD];
    uint8_t encrypted[184U];
    size_t encrypted_length;
    size_t path_bytes;
    if (identity == NULL || contact == NULL || req_data == NULL || packet == NULL ||
        !identity->valid || req_data_len == 0U ||
        req_data_len > MESH_PACKET_MAX_PAYLOAD - 16U) {
        return false;
    }
    if (!_mesh_datagram_hash(contact->public_key, &destination_hash) ||
        !_mesh_datagram_hash(identity->public_key, &source_hash) ||
        !mesh_identity_shared_secret(identity, contact->public_key, secret)) {
        return false;
    }
    memcpy(plain, &tag, 4U);
    memcpy(&plain[4], req_data, req_data_len);
    if (!mesh_crypto_encrypt_then_mac(
            secret, plain, 4U + req_data_len, encrypted, sizeof(encrypted), &encrypted_length)) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->type = MESH_PAYLOAD_REQUEST;
    packet->version = 0U;
    packet->payload[0] = destination_hash;
    packet->payload[1] = source_hash;
    memcpy(&packet->payload[2], encrypted, encrypted_length);
    packet->payload_length = (uint8_t)(2U + encrypted_length);
    if (contact->out_path_length < 0) {
        packet->route = MESH_ROUTE_FLOOD;
        packet->path_length = 0U;
    } else {
        packet->route = MESH_ROUTE_DIRECT;
        packet->path_length = (uint8_t)contact->out_path_length;
        path_bytes =
            (size_t)(packet->path_length & 63U) * (size_t)((packet->path_length >> 6U) + 1U);
        if (path_bytes > sizeof(packet->path)) {
            path_bytes = sizeof(packet->path);
        }
        memcpy(packet->path, contact->out_path, path_bytes);
    }
    return true;
}