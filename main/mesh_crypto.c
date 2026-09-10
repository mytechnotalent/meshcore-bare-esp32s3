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
// File:    mesh_crypto.c
// Desc:    Implements MeshCore cipher and MAC framing using ESP-IDF C crypto APIs.
// Created: 2026

#include "mesh_crypto.h"

#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"

#include <string.h>

/**
 * @brief Compute a truncated HMAC-SHA256 over ciphertext.
 *
 * Hashes ciphertext using HMAC-SHA256 with the secret key and copies
 * the first 2 bytes as the truncated MAC.
 *
 * @param secret Pointer to secret key buffer.
 * @param secret_length Length of secret key in bytes.
 * @param cipher Pointer to ciphertext buffer.
 * @param length Length of ciphertext in bytes.
 * @param mac Destination buffer for 2-byte truncated MAC.
 * @return bool true on success, false on crypto failure or NULL inputs.
 */
static bool _mesh_crypto_mac(const uint8_t *secret,
                             size_t secret_length,
                             const uint8_t *cipher,
                             size_t length,
                             uint8_t *mac) {
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    uint8_t digest[32U];
    if (info == NULL || secret == NULL || secret_length == 0U ||
        mbedtls_md_hmac(info, secret, secret_length, cipher, length, digest) != 0) {
        return false;
    }
    memcpy(mac, digest, MESH_CRYPTO_MAC_SIZE);
    return true;
}

/**
 * @brief Hash bytes with SHA-256 and return the requested prefix.
 *
 * Computes SHA-256 over input data and copies hash_length bytes into hash.
 *
 * @param data Pointer to input data buffer.
 * @param length Length of data in bytes.
 * @param hash Destination buffer for hash bytes.
 * @param hash_length Number of bytes to copy (up to 32).
 * @return bool true on success, false on error or if hash_length > 32.
 */
bool mesh_crypto_sha256(const uint8_t *data, size_t length, uint8_t *hash, size_t hash_length) {
    uint8_t digest[32U];
    if (data == NULL || hash == NULL || hash_length > sizeof(digest)) {
        return false;
    }
    if (mbedtls_sha256(data, length, digest, 0) != 0) {
        return false;
    }
    memcpy(hash, digest, hash_length);
    return true;
}

/**
 * @brief Encrypt bytes in AES blocks and append MeshCore's two-byte MAC.
 *
 * Convenience wrapper using default 32-byte shared secret length.
 *
 * @param secret Pointer to 32-byte shared secret.
 * @param plain Pointer to plaintext buffer.
 * @param plain_length Length of plaintext in bytes.
 * @param out Destination buffer for MAC and ciphertext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param out_length Pointer to store total output byte count.
 * @return bool true on success, false on error.
 */
bool mesh_crypto_encrypt_then_mac(const uint8_t *secret,
                                  const uint8_t *plain,
                                  size_t plain_length,
                                  uint8_t *out,
                                  size_t capacity,
                                  size_t *out_length) {
    return mesh_crypto_encrypt_then_mac_key(
        secret, MESH_CRYPTO_SHARED_SECRET_SIZE, plain, plain_length, out, capacity, out_length);
}

/**
 * @brief Encrypt bytes with AES-128 and MAC using a specified key length.
 *
 * Padds plaintext to 16-byte blocks, encrypts using AES-128-ECB, computes
 * 2-byte HMAC-SHA256, and prepends MAC before the ciphertext.
 *
 * @param secret Pointer to secret key buffer.
 * @param secret_length Length of secret key in bytes (16 or 32).
 * @param plain Pointer to plaintext buffer.
 * @param plain_length Length of plaintext in bytes.
 * @param out Destination buffer for MAC and ciphertext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param out_length Pointer to store output byte count.
 * @return bool true on success, false on encryption error or insufficient capacity.
 */
bool mesh_crypto_encrypt_then_mac_key(const uint8_t *secret,
                                      size_t secret_length,
                                      const uint8_t *plain,
                                      size_t plain_length,
                                      uint8_t *out,
                                      size_t capacity,
                                      size_t *out_length) {
    mbedtls_aes_context aes;
    size_t cipher_length = ((plain_length + 15U) / 16U) * 16U;
    uint8_t block[16U] = {0};
    if (secret == NULL ||
        (secret_length != MESH_CRYPTO_AES_KEY_SIZE &&
         secret_length != MESH_CRYPTO_SHARED_SECRET_SIZE) ||
        plain == NULL || out == NULL || out_length == NULL || cipher_length == 0U ||
        capacity < MESH_CRYPTO_MAC_SIZE + cipher_length) {
        return false;
    }
    mbedtls_aes_init(&aes);
    if (mbedtls_aes_setkey_enc(&aes, secret, 128U) != 0) {
        mbedtls_aes_free(&aes);
        return false;
    }
    for (size_t offset = 0U; offset < cipher_length; offset += 16U) {
        size_t remaining = plain_length > offset ? plain_length - offset : 0U;
        size_t copy_length = remaining > 16U ? 16U : remaining;
        memset(block, 0, sizeof(block));
        if (copy_length > 0U) {
            memcpy(block, &plain[offset], copy_length);
        }
        mbedtls_aes_crypt_ecb(
            &aes, MBEDTLS_AES_ENCRYPT, block, &out[MESH_CRYPTO_MAC_SIZE + offset]);
    }
    mbedtls_aes_free(&aes);
    if (!_mesh_crypto_mac(secret, secret_length, &out[MESH_CRYPTO_MAC_SIZE], cipher_length, out)) {
        return false;
    }
    *out_length = MESH_CRYPTO_MAC_SIZE + cipher_length;
    return true;
}

/**
 * @brief Verify MeshCore's MAC and decrypt AES blocks with zero padding.
 *
 * Convenience wrapper using default 32-byte shared secret length.
 *
 * @param secret Pointer to 32-byte shared secret.
 * @param cipher Pointer to ciphertext buffer with prepended MAC.
 * @param cipher_length Total ciphertext length in bytes.
 * @param plain Destination buffer for decrypted plaintext.
 * @param capacity Capacity of destination buffer in bytes.
 * @param plain_length Pointer to store decrypted plaintext length.
 * @return bool true if MAC matches and decryption succeeds, false otherwise.
 */
bool mesh_crypto_mac_then_decrypt(const uint8_t *secret,
                                  const uint8_t *cipher,
                                  size_t cipher_length,
                                  uint8_t *plain,
                                  size_t capacity,
                                  size_t *plain_length) {
    return mesh_crypto_mac_then_decrypt_key(secret,
                                            MESH_CRYPTO_SHARED_SECRET_SIZE,
                                            cipher,
                                            cipher_length,
                                            plain,
                                            capacity,
                                            plain_length);
}

/**
 * @brief Verify and decrypt using a specified AES/HMAC key length.
 *
 * Verifies the 2-byte HMAC-SHA256 against the ciphertext blocks, and
 * decrypts AES-128-ECB blocks into the plain buffer.
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
                                      size_t *plain_length) {
    mbedtls_aes_context aes;
    uint8_t mac[MESH_CRYPTO_MAC_SIZE];
    size_t encrypted_length;
    if (secret == NULL ||
        (secret_length != MESH_CRYPTO_AES_KEY_SIZE &&
         secret_length != MESH_CRYPTO_SHARED_SECRET_SIZE) ||
        cipher == NULL || plain == NULL || plain_length == NULL ||
        cipher_length <= MESH_CRYPTO_MAC_SIZE) {
        return false;
    }
    encrypted_length = cipher_length - MESH_CRYPTO_MAC_SIZE;
    if ((encrypted_length % MESH_CRYPTO_BLOCK_SIZE) != 0U || capacity < encrypted_length ||
        !_mesh_crypto_mac(
            secret, secret_length, &cipher[MESH_CRYPTO_MAC_SIZE], encrypted_length, mac) ||
        memcmp(mac, cipher, MESH_CRYPTO_MAC_SIZE) != 0) {
        return false;
    }
    mbedtls_aes_init(&aes);
    if (mbedtls_aes_setkey_dec(&aes, secret, 128U) != 0) {
        mbedtls_aes_free(&aes);
        return false;
    }
    for (size_t offset = 0U; offset < encrypted_length; offset += MESH_CRYPTO_BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(
            &aes, MBEDTLS_AES_DECRYPT, &cipher[MESH_CRYPTO_MAC_SIZE + offset], &plain[offset]);
    }
    mbedtls_aes_free(&aes);
    *plain_length = encrypted_length;
    return true;
}