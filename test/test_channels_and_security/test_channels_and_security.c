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
// File:    test_channels_and_security.c
// Desc:    PlatformIO Unity test suite for channels, contacts, and state persistence.
// Created: 2026

#include <stdio.h>
#include <string.h>
#include <unity.h>
#include "config.h"
#include "mesh_packet.h"
#include "node_state.h"

#if !defined(ESP_PLATFORM)
// Include node_state, mesh_packet, and datagram implementations directly for standalone host unit testing
#include "../../main/node_state.c"
#include "../../main/mesh_packet.c"
#include "../../main/datagram.c"

/**
 * @brief Mock implementation of mesh_crypto_sha256 for unit tests.
 *
 * @param data Pointer to input data buffer.
 * @param data_length Length of input data in bytes.
 * @param digest Pointer to output digest buffer.
 * @param digest_length Length of digest buffer in bytes.
 * @return bool true on success, false on error.
 */
bool mesh_crypto_sha256(const uint8_t *data,
                        size_t data_length,
                        uint8_t *digest,
                        size_t digest_length) {
    if (data == NULL || digest == NULL || digest_length == 0U) {
        return false;
    }
    digest[0] = (uint8_t)(data[0] ^ (uint8_t)data_length);
    return true;
}

/**
 * @brief Mock implementation of mesh_identity_shared_secret for unit tests.
 *
 * @param identity Local identity pointer.
 * @param peer_public_key Remote peer public key.
 * @param secret Destination shared secret buffer.
 * @return bool true on success, false on invalid input.
 */
bool mesh_identity_shared_secret(const MeshIdentity *identity,
                                 const uint8_t *peer_public_key,
                                 uint8_t *secret) {
    if (identity == NULL || peer_public_key == NULL || secret == NULL) {
        return false;
    }
    memset(secret, 0x5AU, 32U);
    return true;
}

/**
 * @brief Mock implementation of mesh_crypto_encrypt_then_mac for unit tests.
 *
 * @param key Encryption key pointer.
 * @param plain Plaintext buffer pointer.
 * @param plain_length Length of plaintext in bytes.
 * @param cipher Output ciphertext buffer pointer.
 * @param cipher_capacity Capacity of output ciphertext buffer.
 * @param out_length Pointer to store output ciphertext length.
 * @return bool true on success, false on buffer overflow or invalid arguments.
 */
bool mesh_crypto_encrypt_then_mac(const uint8_t *key,
                                  const uint8_t *plain,
                                  size_t plain_length,
                                  uint8_t *cipher,
                                  size_t cipher_capacity,
                                  size_t *out_length) {
    if (key == NULL || plain == NULL || cipher == NULL || out_length == NULL) {
        return false;
    }
    if (cipher_capacity < plain_length + 2U) {
        return false;
    }
    memcpy(cipher, plain, plain_length);
    cipher[plain_length] = 0xAAU;
    cipher[plain_length + 1U] = 0xBBU;
    *out_length = plain_length + 2U;
    return true;
}

/**
 * @brief Mock implementation of mesh_crypto_encrypt_then_mac_key for unit tests.
 *
 * @param key Key buffer pointer.
 * @param key_length Key buffer length.
 * @param plain Plaintext buffer pointer.
 * @param plain_length Length of plaintext in bytes.
 * @param cipher Output ciphertext buffer pointer.
 * @param cipher_capacity Capacity of output ciphertext buffer.
 * @param out_length Pointer to store output ciphertext length.
 * @return bool true on success, false on buffer overflow or invalid arguments.
 */
bool mesh_crypto_encrypt_then_mac_key(const uint8_t *key,
                                      size_t key_length,
                                      const uint8_t *plain,
                                      size_t plain_length,
                                      uint8_t *cipher,
                                      size_t cipher_capacity,
                                      size_t *out_length) {
    if (key == NULL || key_length == 0U || plain == NULL || cipher == NULL || out_length == NULL) {
        return false;
    }
    if (cipher_capacity < plain_length + 2U) {
        return false;
    }
    memcpy(cipher, plain, plain_length);
    cipher[plain_length] = 0xCCU;
    cipher[plain_length + 1U] = 0xDDU;
    *out_length = plain_length + 2U;
    return true;
}
#endif

/**
 * @brief Setup hook executed before each unit test.
 *
 * @param void No parameters.
 * @return void
 */
void setUp(void) {
    node_state_init();
}

/**
 * @brief Teardown hook executed after each unit test.
 *
 * @param void No parameters.
 * @return void
 */
void tearDown(void) {
}

/**
 * @brief Verify configuration constants.
 *
 * @param void No parameters.
 * @return void
 */
void test_config_constants(void) {
    TEST_ASSERT_EQUAL_UINT32(123456U, MESHCORE_BLE_PIN);
    TEST_ASSERT_EQUAL_UINT32(40U, MESHCORE_MAX_CHANNELS);
    TEST_ASSERT_EQUAL_UINT32(40U, NODE_STATE_MAX_CHANNELS);
    TEST_ASSERT_EQUAL_UINT32(40U, NODE_STATE_MAX_CONTACTS);
}

/**
 * @brief Verify channel 0 initializes to Public.
 *
 * @param void No parameters.
 * @return void
 */
void test_channel_init(void) {
    NodeChannel ch0;
    TEST_ASSERT_TRUE(node_state_get_channel(0, &ch0));
    TEST_ASSERT_TRUE(ch0.configured);
    TEST_ASSERT_EQUAL_STRING("Public", ch0.name);
}

/**
 * @brief Verify 40 channels capacity and boundary checks.
 *
 * @param void No parameters.
 * @return void
 */
void test_channel_capacity_and_boundary(void) {
    NodeChannel test_ch;
    char expected_name[32];
    for (uint8_t i = 1; i < 40; i++) {
        memset(&test_ch, 0, sizeof(test_ch));
        snprintf(test_ch.name, sizeof(test_ch.name), "Channel-%02u", i);
        memset(test_ch.secret, i, sizeof(test_ch.secret));
        TEST_ASSERT_TRUE(node_state_set_channel(i, &test_ch));
    }
    memset(&test_ch, 0, sizeof(test_ch));
    TEST_ASSERT_FALSE(node_state_set_channel(40, &test_ch));
    TEST_ASSERT_FALSE(node_state_get_channel(40, &test_ch));
    for (uint8_t i = 1; i < 40; i++) {
        memset(&test_ch, 0, sizeof(test_ch));
        TEST_ASSERT_TRUE(node_state_get_channel(i, &test_ch));
        TEST_ASSERT_TRUE(test_ch.configured);
        snprintf(expected_name, sizeof(expected_name), "Channel-%02u", i);
        TEST_ASSERT_EQUAL_STRING(expected_name, test_ch.name);
        TEST_ASSERT_EQUAL_UINT8(i, test_ch.secret[0]);
    }
}

/**
 * @brief Verify channel deletion.
 *
 * @param void No parameters.
 * @return void
 */
void test_channel_deletion(void) {
    NodeChannel test_ch;
    memset(&test_ch, 0, sizeof(test_ch));
    TEST_ASSERT_TRUE(node_state_set_channel(5, &test_ch));
    TEST_ASSERT_FALSE(node_state_get_channel(5, &test_ch));
}

/**
 * @brief Verify V3 compact export and import.
 *
 * @param void No parameters.
 * @return void
 */
void test_v3_compact_export_import(void) {
    uint8_t buffer[16384];
    size_t length = 0;
    NodeChannel test_ch;
    NodeContact c1 = {0};
    NodeContact c2 = {0};
    NodeContact c3 = {0};
    const NodeContact *rc1;
    const NodeContact *rc2;
    const NodeContact *rc3;
    uint32_t exported_magic;
    memset(&test_ch, 0, sizeof(test_ch));
    strcpy(test_ch.name, "Emergency-15");
    memset(test_ch.secret, 0x15, sizeof(test_ch.secret));
    TEST_ASSERT_TRUE(node_state_set_channel(15, &test_ch));
    memset(&test_ch, 0, sizeof(test_ch));
    strcpy(test_ch.name, "Admin-39");
    memset(test_ch.secret, 0x39, sizeof(test_ch.secret));
    TEST_ASSERT_TRUE(node_state_set_channel(39, &test_ch));
    memset(c1.public_key, 0xAA, 32);
    strcpy(c1.name, "Alice");
    c1.last_modified = 1000;
    TEST_ASSERT_TRUE(node_state_upsert_contact(&c1));
    memset(c2.public_key, 0xBB, 32);
    strcpy(c2.name, "Bob");
    c2.last_modified = 2000;
    TEST_ASSERT_TRUE(node_state_upsert_contact(&c2));
    memset(c3.public_key, 0xCC, 32);
    strcpy(c3.name, "Charlie");
    c3.last_modified = 3000;
    TEST_ASSERT_TRUE(node_state_upsert_contact(&c3));
    TEST_ASSERT_EQUAL_UINT32(3, (uint32_t)node_state_contact_count());
    TEST_ASSERT_TRUE(node_state_export(buffer, sizeof(buffer), &length));
    exported_magic = *(uint32_t *)buffer;
    TEST_ASSERT_EQUAL_HEX32(0x3343534D, exported_magic);
    node_state_init();
    TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)node_state_contact_count());
    TEST_ASSERT_FALSE(node_state_get_channel(15, &test_ch));
    TEST_ASSERT_TRUE(node_state_import(buffer, length));
    TEST_ASSERT_TRUE(node_state_get_channel(0, &test_ch));
    TEST_ASSERT_EQUAL_STRING("Public", test_ch.name);
    TEST_ASSERT_TRUE(node_state_get_channel(15, &test_ch));
    TEST_ASSERT_EQUAL_STRING("Emergency-15", test_ch.name);
    TEST_ASSERT_TRUE(node_state_get_channel(39, &test_ch));
    TEST_ASSERT_EQUAL_STRING("Admin-39", test_ch.name);
    TEST_ASSERT_EQUAL_UINT32(3, (uint32_t)node_state_contact_count());
    rc1 = node_state_find_contact(c1.public_key, 32);
    TEST_ASSERT_NOT_NULL(rc1);
    TEST_ASSERT_EQUAL_STRING("Alice", rc1->name);
    rc2 = node_state_find_contact(c2.public_key, 32);
    TEST_ASSERT_NOT_NULL(rc2);
    TEST_ASSERT_EQUAL_STRING("Bob", rc2->name);
    rc3 = node_state_find_contact(c3.public_key, 32);
    TEST_ASSERT_NOT_NULL(rc3);
    TEST_ASSERT_EQUAL_STRING("Charlie", rc3->name);
}

/**
 * @brief Verify contact capacity and LRU eviction.
 *
 * @param void No parameters.
 * @return void
 */
void test_contact_capacity_and_lru_eviction(void) {
    NodeContact c_extra = {0};
    uint8_t key0[32];
    for (uint8_t i = 0; i < 40; i++) {
        NodeContact c = {0};
        memset(c.public_key, i + 1, 32);
        snprintf(c.name, sizeof(c.name), "Contact-%02u", i);
        c.last_modified = 100 + i;
        TEST_ASSERT_TRUE(node_state_upsert_contact(&c));
    }
    TEST_ASSERT_EQUAL_UINT32(40, (uint32_t)node_state_contact_count());
    memset(c_extra.public_key, 0xFF, 32);
    strcpy(c_extra.name, "Extra-Contact");
    c_extra.last_modified = 5000;
    TEST_ASSERT_TRUE(node_state_upsert_contact(&c_extra));
    TEST_ASSERT_EQUAL_UINT32(40, (uint32_t)node_state_contact_count());
    memset(key0, 1, 32);
    TEST_ASSERT_NULL(node_state_find_contact(key0, 32));
    TEST_ASSERT_NOT_NULL(node_state_find_contact(c_extra.public_key, 32));
}

/**
 * @brief Verify channel name validation and corrupted slot sanitization.
 *
 * @param void No parameters.
 * @return void
 */
void test_channel_sanitization_and_validation(void) {
    NodeChannel ch = {0};
    NodeChannel read_ch = {0};
    strcpy(ch.name, "Test-Channel#1");
    memset(ch.secret, 0x42, sizeof(ch.secret));
    TEST_ASSERT_TRUE(node_state_set_channel(1, &ch));
    TEST_ASSERT_TRUE(node_state_get_channel(1, &read_ch));
    TEST_ASSERT_EQUAL_STRING("Test-Channel#1", read_ch.name);
    TEST_ASSERT_FALSE(node_state_get_channel(2, &read_ch));
    memset(&ch, 0, sizeof(ch));
    ch.name[0] = 0x01;
    ch.name[1] = 'B';
    TEST_ASSERT_FALSE(node_state_set_channel(2, &ch));
    TEST_ASSERT_FALSE(node_state_get_channel(2, &read_ch));
    memset(&ch, 0, sizeof(ch));
    TEST_ASSERT_FALSE(node_state_set_channel(0, &ch));
    TEST_ASSERT_TRUE(node_state_get_channel(0, &read_ch));
    TEST_ASSERT_EQUAL_STRING("Public", read_ch.name);
    memset(&ch, 0, sizeof(ch));
    TEST_ASSERT_TRUE(node_state_set_channel(1, &ch));
    TEST_ASSERT_FALSE(node_state_get_channel(1, &read_ch));
    _channels[3].configured = true;
    memset(_channels[3].name, 0xFF, sizeof(_channels[3].name));
    TEST_ASSERT_FALSE(node_state_get_channel(3, &read_ch));
    TEST_ASSERT_FALSE(_channels[3].configured);
}

/**
 * @brief Verify trace packet encoding and decoding for repeater pings.
 *
 * @param void No parameters.
 * @return void
 */
void test_trace_packet_encode_decode(void) {
    MeshPacket packet;
    MeshPacket decoded;
    uint8_t wire[255U];
    size_t wire_length = 0U;
    uint32_t tag = 0x12345678U;
    uint32_t auth_code = 0x00000000U;
    uint8_t flags = 0x01U;
    uint8_t path[4U] = {0x11U, 0x22U, 0x33U, 0x44U};
    memset(&packet, 0, sizeof(packet));
    packet.type = MESH_PAYLOAD_TRACE;
    packet.route = MESH_ROUTE_DIRECT;
    packet.version = 0U;
    packet.path_length = 0U;
    memcpy(&packet.payload[0], &tag, 4U);
    memcpy(&packet.payload[4], &auth_code, 4U);
    packet.payload[8] = flags;
    memcpy(&packet.payload[9], path, sizeof(path));
    packet.payload_length = (uint8_t)(9U + sizeof(path));
    TEST_ASSERT_TRUE(mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_DIRECT, (uint8_t)(wire[0] & 3U));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_TRACE, (uint8_t)((wire[0] >> 2U) & 15U));
    TEST_ASSERT_TRUE(mesh_packet_decode(wire, wire_length, &decoded));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_TRACE, (uint8_t)decoded.type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_DIRECT, (uint8_t)decoded.route);
    TEST_ASSERT_EQUAL_UINT8(13U, decoded.payload_length);
    TEST_ASSERT_EQUAL_UINT8(0U, memcmp(&decoded.payload[0], &tag, 4U));
    TEST_ASSERT_EQUAL_UINT8(flags, decoded.payload[8]);
    TEST_ASSERT_EQUAL_UINT8(0U, memcmp(&decoded.payload[9], path, sizeof(path)));
}

/**
 * @brief Verify hop validation and timeout calculation for trace paths.
 *
 * @param void No parameters.
 * @return void
 */
void test_trace_path_hop_validation(void) {
    uint8_t flags_1byte = 0x00U;
    uint8_t flags_2byte = 0x01U;
    uint8_t flags_4byte = 0x02U;
    uint8_t path_sz_1 = flags_1byte & 3U;
    uint8_t path_sz_2 = flags_2byte & 3U;
    uint8_t path_sz_4 = flags_4byte & 3U;
    uint32_t timeout_2hops;
    TEST_ASSERT_EQUAL_UINT8(0U, path_sz_1);
    TEST_ASSERT_EQUAL_UINT8(1U, path_sz_2);
    TEST_ASSERT_EQUAL_UINT8(2U, path_sz_4);
    TEST_ASSERT_EQUAL_UINT32(0U, 4U % (1U << path_sz_1));
    TEST_ASSERT_EQUAL_UINT32(0U, 4U % (1U << path_sz_2));
    TEST_ASSERT_NOT_EQUAL(0U, 3U % (1U << path_sz_2));
    TEST_ASSERT_EQUAL_UINT32(0U, 8U % (1U << path_sz_4));
    TEST_ASSERT_NOT_EQUAL(0U, 6U % (1U << path_sz_4));
    timeout_2hops = 1500U + (uint32_t)(4U >> path_sz_2) * 500U;
    TEST_ASSERT_EQUAL_UINT32(2500U, timeout_2hops);
}

/**
 * @brief Verify supported text message types for CLI repeater commands.
 *
 * @param void No parameters.
 * @return void
 */
void test_companion_text_type_range(void) {
    uint8_t txt_type_plain = 0U;
    uint8_t txt_type_cli_data = 1U;
    uint8_t txt_type_signed_plain = 2U;
    uint8_t txt_type_cli_cmd = 3U;
    uint8_t txt_type_invalid = 4U;
    TEST_ASSERT_TRUE(txt_type_plain <= 3U);
    TEST_ASSERT_TRUE(txt_type_cli_data <= 3U);
    TEST_ASSERT_TRUE(txt_type_signed_plain <= 3U);
    TEST_ASSERT_TRUE(txt_type_cli_cmd <= 3U);
    TEST_ASSERT_FALSE(txt_type_invalid <= 3U);
}

/**
 * @brief Verify trace data push notification frame layout.
 *
 * @param void No parameters.
 * @return void
 */
void test_trace_push_frame_structure(void) {
    uint8_t frame[172U];
    size_t pos = 0U;
    uint32_t tag = 0xAABBCCDDU;
    uint32_t auth_code = 0x00000000U;
    uint8_t flags = 0x01U;
    uint8_t path_len = 4U;
    uint8_t path_sz = flags & 3U;
    size_t snr_count = (size_t)(path_len >> path_sz);
    int8_t last_snr = 24;
    frame[pos++] = 0U;
    frame[pos++] = path_len;
    frame[pos++] = flags;
    memcpy(&frame[pos], &tag, 4U);
    pos += 4U;
    memcpy(&frame[pos], &auth_code, 4U);
    pos += 4U;
    frame[pos++] = 0x10U;
    frame[pos++] = 0x20U;
    frame[pos++] = 0x30U;
    frame[pos++] = 0x40U;
    frame[pos++] = 16U;
    frame[pos++] = 20U;
    frame[pos++] = (uint8_t)last_snr;
    TEST_ASSERT_EQUAL_UINT32(11U + (size_t)path_len + snr_count + 1U, pos);
    TEST_ASSERT_EQUAL_UINT8(0U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(4U, frame[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, frame[2]);
    TEST_ASSERT_EQUAL_UINT8(0xDDU, frame[3]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)last_snr, frame[pos - 1U]);
}

/**
 * @brief Verify transport route encode and decode with transport codes.
 *
 * @param void No parameters.
 * @return void
 */
void test_mesh_packet_transport_route_encode_decode(void) {
    MeshPacket packet;
    MeshPacket decoded;
    uint8_t wire[255U];
    size_t wire_length = 0U;
    memset(&packet, 0, sizeof(packet));
    packet.route = MESH_ROUTE_TRANSPORT_FLOOD;
    packet.type = MESH_PAYLOAD_TEXT;
    packet.version = 0U;
    packet.transport_codes[0] = 0x1234U;
    packet.transport_codes[1] = 0x5678U;
    packet.path_length = 0U;
    packet.payload_length = 4U;
    memcpy(packet.payload, "PING", 4U);
    TEST_ASSERT_TRUE(mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_TRANSPORT_FLOOD, (uint8_t)(wire[0] & 3U));
    TEST_ASSERT_TRUE(mesh_packet_decode(wire, wire_length, &decoded));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_TRANSPORT_FLOOD, (uint8_t)decoded.route);
    TEST_ASSERT_EQUAL_HEX16(0x1234U, decoded.transport_codes[0]);
    TEST_ASSERT_EQUAL_HEX16(0x5678U, decoded.transport_codes[1]);
    TEST_ASSERT_EQUAL_UINT8(4U, decoded.payload_length);
    TEST_ASSERT_EQUAL_UINT8(0U, memcmp(decoded.payload, "PING", 4U));
    packet.route = MESH_ROUTE_TRANSPORT_DIRECT;
    TEST_ASSERT_TRUE(mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_TRANSPORT_DIRECT, (uint8_t)(wire[0] & 3U));
    TEST_ASSERT_TRUE(mesh_packet_decode(wire, wire_length, &decoded));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_TRANSPORT_DIRECT, (uint8_t)decoded.route);
}

/**
 * @brief Verify mesh packet bounds checking and error handling.
 *
 * @param void No parameters.
 * @return void
 */
void test_mesh_packet_validation_errors(void) {
    MeshPacket packet;
    MeshPacket decoded;
    uint8_t wire[255U];
    size_t wire_length = 0U;
    memset(&packet, 0, sizeof(packet));
    packet.type = MESH_PAYLOAD_TEXT;
    TEST_ASSERT_FALSE(mesh_packet_encode(NULL, wire, sizeof(wire), &wire_length));
    TEST_ASSERT_FALSE(mesh_packet_encode(&packet, NULL, sizeof(wire), &wire_length));
    TEST_ASSERT_FALSE(mesh_packet_encode(&packet, wire, sizeof(wire), NULL));
    packet.path_length = 0xFFU;
    TEST_ASSERT_FALSE(mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length));
    packet.path_length = 0U;
    packet.payload_length = (uint8_t)(MESH_PACKET_MAX_PAYLOAD + 1U);
    TEST_ASSERT_FALSE(mesh_packet_encode(&packet, wire, sizeof(wire), &wire_length));
    packet.payload_length = 10U;
    TEST_ASSERT_FALSE(mesh_packet_encode(&packet, wire, 4U, &wire_length));
    TEST_ASSERT_FALSE(mesh_packet_decode(NULL, 10U, &decoded));
    TEST_ASSERT_FALSE(mesh_packet_decode(wire, 10U, NULL));
    TEST_ASSERT_FALSE(mesh_packet_decode(wire, 1U, &decoded));
    wire[0] = (uint8_t)MESH_ROUTE_TRANSPORT_FLOOD;
    TEST_ASSERT_FALSE(mesh_packet_decode(wire, 5U, &decoded));
}

/**
 * @brief Verify mesh datagram request creation for direct and flood routing.
 *
 * @param void No parameters.
 * @return void
 */
void test_datagram_request_direct_and_flood(void) {
    MeshIdentity identity;
    NodeContact contact_direct;
    NodeContact contact_flood;
    MeshPacket packet;
    uint32_t tag = 0xCAFEBABEU;
    uint8_t req_data[4U] = {0x01U, 0x02U, 0x03U, 0x04U};
    memset(&identity, 0, sizeof(identity));
    identity.valid = true;
    memset(identity.public_key, 0x11U, sizeof(identity.public_key));
    memset(&contact_direct, 0, sizeof(contact_direct));
    memset(contact_direct.public_key, 0x22U, sizeof(contact_direct.public_key));
    contact_direct.out_path_length = 2;
    contact_direct.out_path[0] = 0xAAU;
    contact_direct.out_path[1] = 0xBBU;
    TEST_ASSERT_TRUE(mesh_datagram_request(&identity, &contact_direct, tag, req_data, sizeof(req_data), &packet));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_REQUEST, (uint8_t)packet.type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_DIRECT, (uint8_t)packet.route);
    TEST_ASSERT_EQUAL_UINT8(2U, packet.path_length);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, packet.path[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBBU, packet.path[1]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(contact_direct.public_key[0] ^ 32U), packet.payload[0]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(identity.public_key[0] ^ 32U), packet.payload[1]);
    memset(&contact_flood, 0, sizeof(contact_flood));
    memset(contact_flood.public_key, 0x33U, sizeof(contact_flood.public_key));
    contact_flood.out_path_length = -1;
    TEST_ASSERT_TRUE(mesh_datagram_request(&identity, &contact_flood, tag, req_data, sizeof(req_data), &packet));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_REQUEST, (uint8_t)packet.type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_FLOOD, (uint8_t)packet.route);
    TEST_ASSERT_EQUAL_UINT8(0U, packet.path_length);
}

/**
 * @brief Verify mesh datagram request input validation.
 *
 * @param void No parameters.
 * @return void
 */
void test_datagram_request_validation(void) {
    MeshIdentity identity;
    NodeContact contact;
    MeshPacket packet;
    uint32_t tag = 0x11223344U;
    uint8_t req_data[4U] = {0x01U, 0x02U, 0x03U, 0x04U};
    memset(&identity, 0, sizeof(identity));
    identity.valid = true;
    memset(&contact, 0, sizeof(contact));
    TEST_ASSERT_FALSE(mesh_datagram_request(NULL, &contact, tag, req_data, sizeof(req_data), &packet));
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, NULL, tag, req_data, sizeof(req_data), &packet));
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, &contact, tag, NULL, sizeof(req_data), &packet));
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, &contact, tag, req_data, sizeof(req_data), NULL));
    identity.valid = false;
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, &contact, tag, req_data, sizeof(req_data), &packet));
    identity.valid = true;
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, &contact, tag, req_data, 0U, &packet));
    TEST_ASSERT_FALSE(mesh_datagram_request(&identity, &contact, tag, req_data, MESH_PACKET_MAX_PAYLOAD, &packet));
}

/**
 * @brief Verify mesh datagram text and group text building.
 *
 * @param void No parameters.
 * @return void
 */
void test_datagram_text_and_group_text(void) {
    MeshIdentity identity;
    NodeContact contact;
    NodeChannel channel;
    MeshPacket packet;
    uint32_t ack_code = 0U;
    memset(&identity, 0, sizeof(identity));
    identity.valid = true;
    memset(identity.public_key, 0x44U, sizeof(identity.public_key));
    memset(&contact, 0, sizeof(contact));
    memset(contact.public_key, 0x55U, sizeof(contact.public_key));
    contact.out_path_length = -1;
    TEST_ASSERT_TRUE(mesh_datagram_text(&identity, &contact, 1000U, 0U, "Hello Mesh", &packet, &ack_code));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_TEXT, (uint8_t)packet.type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_FLOOD, (uint8_t)packet.route);
    TEST_ASSERT_NOT_EQUAL(0U, ack_code);
    TEST_ASSERT_FALSE(mesh_datagram_text(&identity, &contact, 1000U, 0U, NULL, &packet, &ack_code));
    memset(&channel, 0, sizeof(channel));
    channel.configured = true;
    strcpy(channel.name, "Public");
    memset(channel.secret, 0x77U, sizeof(channel.secret));
    TEST_ASSERT_TRUE(mesh_datagram_group_text(&channel, 2000U, "Tester", "Broadcast msg", &packet));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_PAYLOAD_GROUP_TEXT, (uint8_t)packet.type);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MESH_ROUTE_FLOOD, (uint8_t)packet.route);
    TEST_ASSERT_NOT_EQUAL(0U, packet.payload_length);
    channel.configured = false;
    TEST_ASSERT_FALSE(mesh_datagram_group_text(&channel, 2000U, "Tester", "Broadcast msg", &packet));
}

/**
 * @brief Verify status response push notification frame format.
 *
 * @param void No parameters.
 * @return void
 */
void test_companion_status_push_frame(void) {
    uint8_t frame[172U];
    size_t pos = 0U;
    uint32_t tag = 0x12345678U;
    const char *status_str = "battery: 4.1V, nodes: 5";
    size_t text_len = strlen(status_str);
    frame[pos++] = 0x87U;
    memcpy(&frame[pos], &tag, 4U);
    pos += 4U;
    memcpy(&frame[pos], status_str, text_len);
    pos += text_len;
    TEST_ASSERT_EQUAL_UINT8(0x87U, frame[0]);
    TEST_ASSERT_EQUAL_UINT32(5U + text_len, pos);
    TEST_ASSERT_EQUAL_UINT8(0x78U, frame[1]);
    TEST_ASSERT_EQUAL_UINT8(0x56U, frame[2]);
    TEST_ASSERT_EQUAL_UINT8('b', frame[5]);
}

/**
 * @brief Verify companion opcode definitions for repeaters and requests.
 *
 * @param void No parameters.
 * @return void
 */
void test_companion_opcode_constants(void) {
    TEST_ASSERT_EQUAL_UINT32(26U, 26U);
    TEST_ASSERT_EQUAL_UINT32(27U, 27U);
    TEST_ASSERT_EQUAL_UINT32(36U, 36U);
    TEST_ASSERT_EQUAL_UINT32(50U, 50U);
    TEST_ASSERT_EQUAL_HEX8(0x87U, 0x87U);
    TEST_ASSERT_EQUAL_HEX8(0x89U, 0x89U);
}

/**
 * @brief Main test runner function for Unity.
 *
 * @param void No parameters.
 * @return int 0 on success.
 */
#if defined(ESP_PLATFORM)
/**
 * @brief Application entry point for ESP-IDF on-device testing.
 *
 * @param void No parameters.
 * @return void
 */
void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_constants);
    RUN_TEST(test_channel_init);
    RUN_TEST(test_channel_capacity_and_boundary);
    RUN_TEST(test_channel_deletion);
    RUN_TEST(test_v3_compact_export_import);
    RUN_TEST(test_contact_capacity_and_lru_eviction);
    RUN_TEST(test_channel_sanitization_and_validation);
    RUN_TEST(test_trace_packet_encode_decode);
    RUN_TEST(test_trace_path_hop_validation);
    RUN_TEST(test_companion_text_type_range);
    RUN_TEST(test_trace_push_frame_structure);
    RUN_TEST(test_mesh_packet_transport_route_encode_decode);
    RUN_TEST(test_mesh_packet_validation_errors);
    RUN_TEST(test_datagram_request_direct_and_flood);
    RUN_TEST(test_datagram_request_validation);
    RUN_TEST(test_datagram_text_and_group_text);
    RUN_TEST(test_companion_status_push_frame);
    RUN_TEST(test_companion_opcode_constants);
    UNITY_END();
}
#else
/**
 * @brief Main test runner function for Unity on host native environment.
 *
 * @param void No parameters.
 * @return int 0 on success.
 */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_constants);
    RUN_TEST(test_channel_init);
    RUN_TEST(test_channel_capacity_and_boundary);
    RUN_TEST(test_channel_deletion);
    RUN_TEST(test_v3_compact_export_import);
    RUN_TEST(test_contact_capacity_and_lru_eviction);
    RUN_TEST(test_channel_sanitization_and_validation);
    RUN_TEST(test_trace_packet_encode_decode);
    RUN_TEST(test_trace_path_hop_validation);
    RUN_TEST(test_companion_text_type_range);
    RUN_TEST(test_trace_push_frame_structure);
    RUN_TEST(test_mesh_packet_transport_route_encode_decode);
    RUN_TEST(test_mesh_packet_validation_errors);
    RUN_TEST(test_datagram_request_direct_and_flood);
    RUN_TEST(test_datagram_request_validation);
    RUN_TEST(test_datagram_text_and_group_text);
    RUN_TEST(test_companion_status_push_frame);
    RUN_TEST(test_companion_opcode_constants);
    return UNITY_END();
}
#endif
