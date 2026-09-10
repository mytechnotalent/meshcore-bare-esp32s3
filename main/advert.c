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
// File:    advert.c
// Desc:    Builds MeshCore signed advertisement payloads without heap allocation.
// Created: 2026

#include "advert.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Advertisement location flag.
 */
#define MESH_ADVERT_FLAG_LOCATION 0x10U

/**
 * @brief Advertisement feature 1 flag.
 */
#define MESH_ADVERT_FLAG_FEAT1 0x20U

/**
 * @brief Advertisement feature 2 flag.
 */
#define MESH_ADVERT_FLAG_FEAT2 0x40U

/**
 * @brief Advertisement name flag.
 */
#define MESH_ADVERT_FLAG_NAME 0x80U

/**
 * @brief Build the bounded advert application-data section.
 *
 * Encodes role flags, optional coordinates, and optional UTF-8 node name into
 * the caller-provided buffer.
 *
 * @param options Pointer to advert options structure.
 * @param data Destination buffer for application data.
 * @param capacity Capacity of destination buffer in bytes.
 * @return size_t Encoded application-data length, or zero on error.
 */
static size_t
_mesh_advert_app_data(const MeshAdvertOptions *options, uint8_t *data, size_t capacity) {
    size_t position = 1U;
    size_t name_length = 0U;
    uint8_t flags;
    if (options == NULL || data == NULL || capacity < 1U) {
        return 0U;
    }
    flags = options->role & 0x0FU;
    if (options->has_location) {
        flags |= MESH_ADVERT_FLAG_LOCATION;
    }
    if (options->name != NULL) {
        name_length = strnlen(options->name, MESH_ADVERT_MAX_APP_DATA);
        if (name_length > 0U) {
            flags |= MESH_ADVERT_FLAG_NAME;
        }
    }
    data[0] = flags;
    if (options->has_location) {
        if (capacity < position + 8U) {
            return 0U;
        }
        memcpy(&data[position], &options->latitude, 4U);
        position += 4U;
        memcpy(&data[position], &options->longitude, 4U);
        position += 4U;
    }
    if (name_length > capacity - position) {
        name_length = capacity - position;
    }
    if (name_length > 0U) {
        memcpy(&data[position], options->name, name_length);
        position += name_length;
    }
    return position;
}

/**
 * @brief Build the signed advert payload from identity and options.
 *
 * Concatenates the public key, timestamp, and application data, computes
 * the Ed25519 signature over that content, and produces the complete advert packet.
 *
 * @param identity Pointer to local identity.
 * @param timestamp Epoch timestamp in seconds.
 * @param options Pointer to advert options structure.
 * @param payload Destination buffer for signed payload.
 * @param capacity Capacity of destination buffer in bytes.
 * @param length Pointer to store output payload length.
 * @return bool true on success, false on validation failure or insufficient buffer.
 */
bool mesh_advert_build(const MeshIdentity *identity,
                       uint32_t timestamp,
                       const MeshAdvertOptions *options,
                       uint8_t *payload,
                       size_t capacity,
                       size_t *length) {
    uint8_t app_data[MESH_ADVERT_MAX_APP_DATA];
    uint8_t
        signed_data[MESH_IDENTITY_PUBLIC_KEY_SIZE + sizeof(uint32_t) + MESH_ADVERT_MAX_APP_DATA];
    size_t app_length;
    size_t signed_length;
    size_t position = 0U;
    if (identity == NULL || !identity->valid || options == NULL || payload == NULL ||
        length == NULL || capacity < MESH_ADVERT_MAX_PAYLOAD) {
        return false;
    }
    app_length = _mesh_advert_app_data(options, app_data, sizeof(app_data));
    if (app_length == 0U) {
        return false;
    }
    memcpy(&payload[position], identity->public_key, 32U);
    position += 32U;
    memcpy(&payload[position], &timestamp, 4U);
    position += 4U;
    memcpy(&signed_data[0], identity->public_key, 32U);
    memcpy(&signed_data[32], &timestamp, 4U);
    memcpy(&signed_data[36], app_data, app_length);
    signed_length = 36U + app_length;
    if (!mesh_identity_sign(identity, signed_data, signed_length, &payload[position])) {
        return false;
    }
    position += 64U;
    memcpy(&payload[position], app_data, app_length);
    position += app_length;
    *length = position;
    return true;
}

/**
 * @brief Verify and decode a signed advert into a bounded contact.
 *
 * Validates the Ed25519 signature of the received advert packet and populates
 * the target contact structure with public key, timestamp, role, location, and name.
 *
 * @param identity Pointer to local identity.
 * @param payload Pointer to received payload bytes.
 * @param length Payload length in bytes.
 * @param contact Destination pointer for parsed contact.
 * @return bool true if signature and payload are valid, false otherwise.
 */
bool mesh_advert_parse(const MeshIdentity *identity,
                       const uint8_t *payload,
                       size_t length,
                       NodeContact *contact) {
    uint8_t app_data[32U];
    uint8_t signed_data[68U];
    size_t app_length;
    size_t position = 0U;
    size_t name_len;
    uint8_t flags;
    if (identity == NULL || payload == NULL || contact == NULL || length < 101U || length > 132U) {
        return false;
    }
    memset(contact, 0, sizeof(*contact));
    memcpy(contact->public_key, payload, 32U);
    memcpy(&contact->last_advert_timestamp, &payload[32], 4U);
    app_length = length - 100U;
    memcpy(app_data, &payload[100], app_length);
    memcpy(signed_data, payload, 36U);
    memcpy(&signed_data[36], app_data, app_length);
    if (!mesh_identity_verify(payload, signed_data, 36U + app_length, &payload[36])) {
        return false;
    }
    flags = app_data[position++];
    contact->type = flags & 0x0FU;
    if ((flags & MESH_ADVERT_FLAG_LOCATION) != 0U) {
        if (app_length < position + 8U) {
            return false;
        }
        memcpy(&contact->latitude, &app_data[position], 4U);
        position += 4U;
        memcpy(&contact->longitude, &app_data[position], 4U);
        position += 4U;
    }
    if ((flags & MESH_ADVERT_FLAG_FEAT1) != 0U) {
        if (app_length < position + 2U) {
            return false;
        }
        position += 2U;
    }
    if ((flags & MESH_ADVERT_FLAG_FEAT2) != 0U) {
        if (app_length < position + 2U) {
            return false;
        }
        position += 2U;
    }
    if ((flags & MESH_ADVERT_FLAG_NAME) != 0U && position < app_length) {
        name_len = app_length - position;
        if (name_len >= sizeof(contact->name)) {
            name_len = sizeof(contact->name) - 1U;
        }
        memcpy(contact->name, &app_data[position], name_len);
        contact->name[name_len] = '\0';
    } else {
        if (contact->type == 2U) {
            snprintf(
                contact->name, sizeof(contact->name), "Repeater-%02X%02X", payload[0], payload[1]);
        } else {
            snprintf(contact->name, sizeof(contact->name), "Node-%02X%02X", payload[0], payload[1]);
        }
    }
    contact->out_path_length = -1;
    contact->last_modified = contact->last_advert_timestamp;
    return true;
}