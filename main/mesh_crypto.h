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
// File:    mesh_crypto.h
// Desc:    Defines MeshCore-compatible hashing and encrypted datagram operations.
// Created: 2026

#ifndef MESH_CRYPTO_H
#define MESH_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief MeshCore AES key length in bytes.
 */
#define MESH_CRYPTO_AES_KEY_SIZE 16U

/**
 * @brief MeshCore shared-secret length in bytes.
 */
#define MESH_CRYPTO_SHARED_SECRET_SIZE 32U

/**
 * @brief MeshCore truncated MAC length in bytes.
 */
#define MESH_CRYPTO_MAC_SIZE 2U

/**
 * @brief AES block length in bytes.
 */
#define MESH_CRYPTO_BLOCK_SIZE 16U

/**
 * @brief Hash arbitrary bytes with SHA-256 and truncate to desired length.
 *
 * Computes standard SHA-256 over data and copies up to hash_length bytes
 * into the destination buffer.
 *
 * @param data Pointer to input data to hash.
 * @param length Length of input data in bytes.
 * @param hash Destination buffer for hash bytes.
 * @param hash_length Number of hash bytes requested (up to 32).
 * @return bool true on success, false if parameters invalid.
 */
bool mesh_crypto_sha256(const uint8_t *data, size_t length, uint8_t *hash, size_t hash_length);

/**
 * @brief Encrypt and prepend a MeshCore truncated MAC using a 32-byte shared secret.
 *
 * @param secret Pointer to 32-byte shared secret.
 * @param plain Pointer to plaintext buffer.
 * @param plain_length Length of plaintext in bytes.
 * @param out Destination buffer for encrypted ciphertext with prepended MAC.
 * @param capacity Capacity of destination buffer in bytes.
 * @param out_length Pointer to store total output byte count.
 * @return bool true on success, false on encryption error or insufficient capacity.
 */
bool mesh_crypto_encrypt_then_mac(const uint8_t *secret,
                                  const uint8_t *plain,
                                  size_t plain_length,
                                  uint8_t *out,
                                  size_t capacity,
                                  size_t *out_length);

/**
 * @brief Encrypt and MAC using a specified AES/HMAC key length.
 *
 * @param secret Pointer to secret key buffer.
 * @param secret_length Length of secret key in bytes (16 or 32).
 * @param plain Pointer to plaintext buffer.
 * @param plain_length Length of plaintext in bytes.
 * @param out Destination buffer for MAC and ciphertext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param out_length Pointer to store output byte count.
 * @return bool true on success, false on error.
 */
bool mesh_crypto_encrypt_then_mac_key(const uint8_t *secret,
                                      size_t secret_length,
                                      const uint8_t *plain,
                                      size_t plain_length,
                                      uint8_t *out,
                                      size_t capacity,
                                      size_t *out_length);

/**
 * @brief Verify and decrypt a MeshCore encrypted datagram using a 32-byte shared secret.
 *
 * Verifies truncated MAC and decrypts AES-CTR ciphertext into destination buffer.
 *
 * @param secret Pointer to 32-byte shared secret.
 * @param cipher Pointer to ciphertext buffer including prepended MAC.
 * @param cipher_length Total ciphertext length in bytes.
 * @param plain Destination buffer for decrypted plaintext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param plain_length Pointer to store decrypted plaintext length.
 * @return bool true if MAC verifies and decryption succeeds, false otherwise.
 */
bool mesh_crypto_mac_then_decrypt(const uint8_t *secret,
                                  const uint8_t *cipher,
                                  size_t cipher_length,
                                  uint8_t *plain,
                                  size_t capacity,
                                  size_t *plain_length);

/**
 * @brief Verify and decrypt using a specified AES/HMAC key length.
 *
 * @param secret Pointer to secret key buffer.
 * @param secret_length Length of secret key in bytes (16 or 32).
 * @param cipher Pointer to ciphertext buffer with prepended MAC.
 * @param cipher_length Total ciphertext length in bytes.
 * @param plain Destination buffer for decrypted plaintext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param plain_length Pointer to store decrypted plaintext length.
 * @return bool true if MAC matches and decryption succeeds, false otherwise.
 */
bool mesh_crypto_mac_then_decrypt_key(const uint8_t *secret,
                                      size_t secret_length,
                                      const uint8_t *cipher,
                                      size_t cipher_length,
                                      uint8_t *plain,
                                      size_t capacity,
                                      size_t *plain_length);

#endif // MESH_CRYPTO_H