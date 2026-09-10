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
// File:    identity.h
// Desc:    Defines the portable MeshCore Ed25519 identity interface.
// Created: 2026

#ifndef IDENTITY_H
#define IDENTITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Ed25519 seed length in bytes.
 */
#define MESH_IDENTITY_SEED_SIZE 32U

/**
 * @brief Ed25519 public-key length in bytes.
 */
#define MESH_IDENTITY_PUBLIC_KEY_SIZE 32U

/**
 * @brief Ed25519 private-key length in bytes.
 */
#define MESH_IDENTITY_PRIVATE_KEY_SIZE 64U

/**
 * @brief Ed25519 signature length in bytes.
 */
#define MESH_IDENTITY_SIGNATURE_SIZE 64U

/**
 * @brief MeshCore local identity structure.
 */
typedef struct {
    /**
     * @brief Ed25519 public key.
     */
    uint8_t public_key[MESH_IDENTITY_PUBLIC_KEY_SIZE];
    /**
     * @brief Ed25519 private key and expanded hash.
     */
    uint8_t private_key[MESH_IDENTITY_PRIVATE_KEY_SIZE];
    /**
     * @brief Ed25519 seed.
     */
    uint8_t seed[MESH_IDENTITY_SEED_SIZE];
    /**
     * @brief Whether a valid identity is loaded.
     */
    bool valid;
} MeshIdentity;

/**
 * @brief Clear and zero-initialize the identity structure.
 *
 * @param identity Pointer to MeshIdentity structure to reset.
 * @return void
 */
void mesh_identity_init(MeshIdentity *identity);

/**
 * @brief Create an identity keypair from a 32-byte seed.
 *
 * @param identity Pointer to target MeshIdentity structure.
 * @param seed Pointer to 32-byte seed buffer.
 * @return bool true on success, false if pointers are NULL.
 */
bool mesh_identity_from_seed(MeshIdentity *identity, const uint8_t *seed);

/**
 * @brief Sign a message using the identity private key.
 *
 * @param identity Pointer to valid MeshIdentity.
 * @param message Pointer to message buffer to sign.
 * @param length Length of message in bytes.
 * @param signature Destination buffer for 64-byte signature.
 * @return bool true if signing succeeded, false otherwise.
 */
bool mesh_identity_sign(const MeshIdentity *identity,
                        const uint8_t *message,
                        size_t length,
                        uint8_t *signature);

/**
 * @brief Verify an Ed25519 signature against a public key and message.
 *
 * @param public_key Pointer to 32-byte Ed25519 public key.
 * @param message Pointer to message buffer.
 * @param length Length of message in bytes.
 * @param signature Pointer to 64-byte signature.
 * @return bool true if signature is valid, false otherwise.
 */
bool mesh_identity_verify(const uint8_t *public_key,
                          const uint8_t *message,
                          size_t length,
                          const uint8_t *signature);

/**
 * @brief Derive a shared secret using an Ed25519/X25519 key exchange.
 *
 * @param identity Pointer to local valid MeshIdentity.
 * @param public_key Pointer to peer 32-byte public key.
 * @param secret Destination buffer for 32-byte derived secret.
 * @return bool true on success, false otherwise.
 */
bool mesh_identity_shared_secret(const MeshIdentity *identity,
                                 const uint8_t *public_key,
                                 uint8_t *secret);

/**
 * @brief Serialize an identity into a fixed binary record.
 *
 * @param identity Pointer to MeshIdentity to export.
 * @param data Destination buffer for serialized binary record.
 * @param capacity Capacity of destination buffer in bytes.
 * @param length Pointer to store output byte count.
 * @return bool true on success, false if buffer too small or invalid identity.
 */
bool mesh_identity_export(const MeshIdentity *identity,
                          uint8_t *data,
                          size_t capacity,
                          size_t *length);

/**
 * @brief Restore an identity from a fixed binary record.
 *
 * @param identity Pointer to target MeshIdentity structure.
 * @param data Pointer to source binary record.
 * @param length Length of binary record in bytes.
 * @return bool true on success, false on invalid record or corrupted data.
 */
bool mesh_identity_import(MeshIdentity *identity, const uint8_t *data, size_t length);

#endif // IDENTITY_H