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
// File:    advert.h
// Desc:    Defines MeshCore signed advertisement construction and parsing.
// Created: 2026

#ifndef ADVERT_H
#define ADVERT_H

#include "identity.h"
#include "node_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum MeshCore advert application data length.
 */
#define MESH_ADVERT_MAX_APP_DATA 32U

/**
 * @brief Maximum MeshCore advert payload length.
 */
#define MESH_ADVERT_MAX_PAYLOAD 132U

/**
 * @brief MeshCore chat-node advert role.
 */
#define MESH_ADVERT_ROLE_CHAT 1U

/**
 * @brief MeshCore advert options structure.
 */
typedef struct {
    /**
     * @brief Advert role in the low four flag bits.
     */
    uint8_t role;
    /**
     * @brief Whether location is included.
     */
    bool has_location;
    /**
     * @brief Latitude in millionths of a degree.
     */
    int32_t latitude;
    /**
     * @brief Longitude in millionths of a degree.
     */
    int32_t longitude;
    /**
     * @brief Advertised UTF-8 node name.
     */
    const char *name;
} MeshAdvertOptions;

/**
 * @brief Build a signed MeshCore advert payload.
 *
 * Encodes the public key, timestamp, flags, location, optional application data,
 * and signs the payload with Ed25519.
 *
 * @param identity Pointer to local identity containing keypair.
 * @param timestamp Epoch timestamp in seconds.
 * @param options Pointer to advert options structure.
 * @param payload Destination buffer for the signed advert payload.
 * @param capacity Capacity of the destination buffer in bytes.
 * @param length Pointer to store the resulting payload length.
 * @return bool true if advert was built successfully, false otherwise.
 */
bool mesh_advert_build(const MeshIdentity *identity,
                       uint32_t timestamp,
                       const MeshAdvertOptions *options,
                       uint8_t *payload,
                       size_t capacity,
                       size_t *length);

/**
 * @brief Verify and parse a signed MeshCore advert payload.
 *
 * Verifies the Ed25519 signature and unpacks the contact information into
 * a NodeContact structure.
 *
 * @param identity Pointer to local identity.
 * @param payload Pointer to received advert payload bytes.
 * @param length Length of received advert payload in bytes.
 * @param contact Destination pointer for the parsed node contact.
 * @return bool true if signature verifies and parsing succeeds, false otherwise.
 */
bool mesh_advert_parse(const MeshIdentity *identity,
                       const uint8_t *payload,
                       size_t length,
                       NodeContact *contact);

#endif // ADVERT_H