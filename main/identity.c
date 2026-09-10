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
// File:    identity.c
// Desc:    Implements the portable MeshCore Ed25519 identity wrapper.
// Created: 2026

#include "identity.h"

#include "ed_25519.h"

#include <string.h>

/**
 * @brief Clear an identity structure and its secret material.
 *
 * Zeros out the entire MeshIdentity memory buffer.
 *
 * @param identity Pointer to MeshIdentity structure to clear.
 * @return void
 */
void mesh_identity_init(MeshIdentity *identity) {
    if (identity != NULL) {
        memset(identity, 0, sizeof(*identity));
    }
}

/**
 * @brief Create an Ed25519 keypair from a caller-provided seed.
 *
 * Derives the public and private keys using ed25519_create_keypair and
 * marks the identity as valid.
 *
 * @param identity Pointer to target MeshIdentity structure.
 * @param seed Pointer to 32-byte seed buffer.
 * @return bool true on success, false if input pointers are NULL.
 */
bool mesh_identity_from_seed(MeshIdentity *identity, const uint8_t *seed) {
    if (identity == NULL || seed == NULL) {
        return false;
    }
    memcpy(identity->seed, seed, MESH_IDENTITY_SEED_SIZE);
    ed25519_create_keypair(identity->public_key, identity->private_key, identity->seed);
    identity->valid = true;
    return true;
}

/**
 * @brief Sign a bounded message with a valid identity.
 *
 * Uses the private key to produce a 64-byte Ed25519 signature.
 *
 * @param identity Pointer to valid MeshIdentity.
 * @param message Pointer to message buffer to sign.
 * @param length Length of message in bytes.
 * @param signature Destination buffer for 64-byte signature.
 * @return bool true if signing succeeded, false if invalid identity or NULL parameters.
 */
bool mesh_identity_sign(const MeshIdentity *identity,
                        const uint8_t *message,
                        size_t length,
                        uint8_t *signature) {
    if (identity == NULL || !identity->valid || message == NULL || signature == NULL) {
        return false;
    }
    ed25519_sign(signature, message, length, identity->public_key, identity->private_key);
    return true;
}

/**
 * @brief Verify a message signature against an Ed25519 public key.
 *
 * Checks the authenticity of the message signature using ed25519_verify.
 *
 * @param public_key Pointer to 32-byte Ed25519 public key.
 * @param message Pointer to message buffer.
 * @param length Length of message in bytes.
 * @param signature Pointer to 64-byte signature.
 * @return bool true if signature verifies successfully, false otherwise.
 */
bool mesh_identity_verify(const uint8_t *public_key,
                          const uint8_t *message,
                          size_t length,
                          const uint8_t *signature) {
    if (public_key == NULL || message == NULL || signature == NULL) {
        return false;
    }
    return ed25519_verify(signature, message, length, public_key) == 1;
}

/**
 * @brief Derive a shared secret from an identity and peer public key.
 *
 * Computes a Diffie-Hellman shared secret using ed25519_key_exchange.
 *
 * @param identity Pointer to local valid MeshIdentity.
 * @param public_key Pointer to peer 32-byte public key.
 * @param secret Destination buffer for 32-byte derived secret.
 * @return bool true on success, false if invalid identity or NULL parameters.
 */
bool mesh_identity_shared_secret(const MeshIdentity *identity,
                                 const uint8_t *public_key,
                                 uint8_t *secret) {
    if (identity == NULL || !identity->valid || public_key == NULL || secret == NULL) {
        return false;
    }
    ed25519_key_exchange(secret, public_key, identity->private_key);
    return true;
}

/**
 * @brief Serialize seed, private key, and public key material.
 *
 * Copies the seed (32 bytes), private key (64 bytes), and public key (32 bytes)
 * into a contiguous 128-byte export buffer.
 *
 * @param identity Pointer to valid MeshIdentity to export.
 * @param data Destination buffer for serialized binary record.
 * @param capacity Capacity of destination buffer in bytes (must be >= 128).
 * @param length Pointer to store output length (set to 128 on success).
 * @return bool true on success, false if capacity too small or invalid identity.
 */
bool mesh_identity_export(const MeshIdentity *identity,
                          uint8_t *data,
                          size_t capacity,
                          size_t *length) {
    if (identity == NULL || data == NULL || length == NULL || capacity < 128U || !identity->valid) {
        return false;
    }
    memcpy(&data[0], identity->seed, 32U);
    memcpy(&data[32], identity->private_key, 64U);
    memcpy(&data[96], identity->public_key, 32U);
    *length = 128U;
    return true;
}

/**
 * @brief Restore and validate a serialized identity record.
 *
 * Unpacks a 128-byte binary record into the target MeshIdentity and sets valid to true.
 *
 * @param identity Pointer to target MeshIdentity structure.
 * @param data Pointer to 128-byte source binary record.
 * @param length Length of binary record in bytes (must be exactly 128).
 * @return bool true on success, false if NULL or length != 128.
 */
bool mesh_identity_import(MeshIdentity *identity, const uint8_t *data, size_t length) {
    if (identity == NULL || data == NULL || length != 128U) {
        return false;
    }
    memcpy(identity->seed, &data[0], 32U);
    memcpy(identity->private_key, &data[32], 64U);
    memcpy(identity->public_key, &data[96], 32U);
    identity->valid = true;
    return true;
}