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
// File:    storage.c
// Desc:    Persists portable MeshCore node state in ESP-IDF NVS.
// Created: 2026

#include "storage.h"

#include "esp_log.h"
#include "esp_random.h"
#include "mesh_crypto.h"
#include "node_state.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

/**
 * @brief Storage log tag.
 */
static const char *_storage_log_tag = "storage";

/**
 * @brief NVS namespace used by the node.
 */
#define STORAGE_NAMESPACE "meshcore"

/**
 * @brief NVS key containing serialized state.
 */
#define STORAGE_STATE_KEY "state"

/**
 * @brief NVS key containing the dedicated node name.
 */
#define STORAGE_NODE_NAME_KEY "node_name"

/**
 * @brief NVS key containing the default flood-scope name.
 */
#define STORAGE_SCOPE_NAME_KEY "scope_name"

/**
 * @brief NVS key containing the default flood-scope key.
 */
#define STORAGE_SCOPE_KEY "scope_key"

/**
 * @brief NVS key containing the local identity.
 */
#define STORAGE_IDENTITY_KEY "identity"

/**
 * @brief NVS marker preventing repeated BLE bond resets.
 */
#define STORAGE_BLE_RESET_KEY "ble_reset_v12"

/**
 * @brief Maximum serialized node state size in bytes.
 */
#define STORAGE_STATE_SIZE 12288U

/**
 * @brief Opened NVS handle.
 */
static nvs_handle_t _storage_handle;

/**
 * @brief Whether NVS storage is ready.
 */
static bool _storage_ready;

/**
 * @brief Persistent local identity.
 */
static MeshIdentity _identity;

/**
 * @brief Persistent state serialization buffer.
 */
static uint8_t _storage_state_data[STORAGE_STATE_SIZE];

/**
 * @brief Persistent identity record buffer.
 */
static uint8_t _storage_identity_data[128U];

/**
 * @brief First-boot identity seed buffer.
 */
static uint8_t _storage_seed[32U];

/**
 * @brief Initialize NVS and restore the saved state blob.
 *
 * Mounts NVS partition, initializes persistent Ed25519 identity, imports
 * state blob, and applies the dedicated node name and channel scope.
 *
 * @param void No parameters.
 * @return bool true when NVS is ready, false on initialization failure.
 */
bool storage_init(void) {
    size_t length = sizeof(_storage_state_data);
    size_t identity_length = sizeof(_storage_identity_data);
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        result = nvs_flash_init();
    }
    if (result != ESP_OK ||
        nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &_storage_handle) != ESP_OK) {
        return false;
    }
    _storage_ready = true;
    mesh_identity_init(&_identity);
    if (nvs_get_blob(_storage_handle, STORAGE_IDENTITY_KEY, NULL, &identity_length) == ESP_OK &&
        identity_length == sizeof(_storage_identity_data) &&
        nvs_get_blob(
            _storage_handle, STORAGE_IDENTITY_KEY, _storage_identity_data, &identity_length) ==
            ESP_OK) {
        mesh_identity_import(&_identity, _storage_identity_data, identity_length);
    }
    if (!_identity.valid) {
        esp_fill_random(_storage_seed, sizeof(_storage_seed));
        mesh_identity_from_seed(&_identity, _storage_seed);
        if (mesh_identity_export(&_identity,
                                 _storage_identity_data,
                                 sizeof(_storage_identity_data),
                                 &identity_length) &&
            nvs_set_blob(
                _storage_handle, STORAGE_IDENTITY_KEY, _storage_identity_data, identity_length) ==
                ESP_OK) {
            nvs_commit(_storage_handle);
        }
    }
    if (nvs_get_blob(_storage_handle, STORAGE_STATE_KEY, NULL, &length) == ESP_OK &&
        length <= sizeof(_storage_state_data) &&
        nvs_get_blob(_storage_handle, STORAGE_STATE_KEY, _storage_state_data, &length) == ESP_OK) {
        node_state_import(_storage_state_data, length);
    }
    {
        char saved_node_name[32U] = {0};
        size_t saved_node_name_len = sizeof(saved_node_name);
        if (nvs_get_str(
                _storage_handle, STORAGE_NODE_NAME_KEY, saved_node_name, &saved_node_name_len) ==
                ESP_OK &&
            saved_node_name_len > 0U) {
            node_state_set_name((const uint8_t *)saved_node_name, saved_node_name_len);
            node_state_clear_dirty();
            ESP_LOGI(_storage_log_tag, "Restored device name: %s", saved_node_name);
        }
    }
    {
        uint8_t scope_name[31U] = {0};
        uint8_t scope_key[16U] = {0};
        uint8_t old_region_key[16U] = {0};
        size_t scope_name_length = sizeof(scope_name);
        size_t scope_key_length = sizeof(scope_key);
        mesh_crypto_sha256((const uint8_t *)"#Region", 7U, old_region_key, sizeof(old_region_key));
        if (nvs_get_blob(_storage_handle, STORAGE_SCOPE_NAME_KEY, scope_name, &scope_name_length) ==
                ESP_OK &&
            nvs_get_blob(_storage_handle, STORAGE_SCOPE_KEY, scope_key, &scope_key_length) ==
                ESP_OK &&
            scope_name_length > 0U && scope_name_length < sizeof(scope_name) &&
            scope_key_length == sizeof(scope_key)) {
            if (scope_name_length == 6U && memcmp(scope_name, "Region", 6U) == 0 &&
                memcmp(scope_key, old_region_key, sizeof(scope_key)) == 0) {
                memset(scope_name, 0, sizeof(scope_name));
                memcpy(scope_name, "USA/Canada", 10U);
                mesh_crypto_sha256(
                    (const uint8_t *)"#USA/Canada", 11U, scope_key, sizeof(scope_key));
                nvs_set_blob(_storage_handle, STORAGE_SCOPE_NAME_KEY, scope_name, 10U);
                nvs_set_blob(_storage_handle, STORAGE_SCOPE_KEY, scope_key, sizeof(scope_key));
                nvs_commit(_storage_handle);
                scope_name_length = 10U;
            }
            node_state_set_default_scope(scope_name, scope_name_length, scope_key);
            node_state_clear_dirty();
        }
    }
    return true;
}

/**
 * @brief Return the persistent local identity.
 *
 * @param void No parameters.
 * @return const MeshIdentity* Pointer to persistent MeshIdentity structure.
 */
const MeshIdentity *storage_identity(void) {
    return &_identity;
}

/**
 * @brief Save the dedicated node name directly to persistent flash.
 *
 * Sets and commits the node name key immediately in NVS so persistence does
 * not depend on secondary blob operations.
 *
 * @param name Pointer to null-terminated name string.
 * @return bool true if successfully saved and committed, false otherwise.
 */
bool storage_save_node_name(const char *name) {
    if (!_storage_ready || name == NULL || name[0] == '\0') {
        return false;
    }
    esp_err_t err = nvs_set_str(_storage_handle, STORAGE_NODE_NAME_KEY, name);
    if (err != ESP_OK) {
        ESP_LOGE(_storage_log_tag, "Failed to save device name to NVS: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_commit(_storage_handle);
    if (err != ESP_OK) {
        ESP_LOGE(_storage_log_tag, "Failed to commit device name to NVS: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(_storage_log_tag, "Persisted device name: %s", name);
    return true;
}

/**
 * @brief Serialize and commit changed node state to NVS.
 *
 * Saves dedicated node name, channel scope, and state blob, performing
 * low-space recovery if necessary.
 *
 * @param void No parameters.
 * @return bool true on success or if state was clean, false on error.
 */
bool storage_save_if_dirty(void) {
    size_t length = 0U;
    if (!_storage_ready || !node_state_is_dirty()) {
        return true;
    }
    {
        uint8_t scope_name[31U];
        uint8_t scope_key[16U];
        size_t scope_name_length;
        const char *current_name = node_state_get_name();
        esp_err_t err;
        if (current_name != NULL && current_name[0] != '\0') {
            err = nvs_set_str(_storage_handle, STORAGE_NODE_NAME_KEY, current_name);
            if (err == ESP_OK) {
                nvs_commit(_storage_handle);
            } else {
                ESP_LOGE(_storage_log_tag, "Failed to save device name: %s", esp_err_to_name(err));
            }
        }
        node_state_get_default_scope(scope_name, scope_key);
        scope_name_length = strnlen((const char *)scope_name, sizeof(scope_name));
        if (node_state_export(_storage_state_data, sizeof(_storage_state_data), &length)) {
            err = nvs_set_blob(_storage_handle, STORAGE_STATE_KEY, _storage_state_data, length);
            if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
                ESP_LOGW(_storage_log_tag, "NVS space low, reclaiming state blob before rewrite");
                nvs_erase_key(_storage_handle, STORAGE_STATE_KEY);
                nvs_commit(_storage_handle);
                err = nvs_set_blob(_storage_handle, STORAGE_STATE_KEY, _storage_state_data, length);
            }
            if (err != ESP_OK) {
                ESP_LOGE(_storage_log_tag,
                         "Failed to save state blob: %s (length=%zu)",
                         esp_err_to_name(err),
                         length);
            }
        }
        if (scope_name_length == 0U) {
            nvs_erase_key(_storage_handle, STORAGE_SCOPE_NAME_KEY);
            nvs_erase_key(_storage_handle, STORAGE_SCOPE_KEY);
        } else {
            nvs_set_blob(_storage_handle, STORAGE_SCOPE_NAME_KEY, scope_name, scope_name_length);
            nvs_set_blob(_storage_handle, STORAGE_SCOPE_KEY, scope_key, sizeof(scope_key));
        }
        if (nvs_commit(_storage_handle) != ESP_OK) {
            ESP_LOGE(_storage_log_tag, "Failed to commit NVS");
            return false;
        }
    }
    node_state_clear_dirty();
    return true;
}

/**
 * @brief Claim the one-time BLE bond reset for the current firmware.
 *
 * Verifies if the reset marker key exists, and sets it to 1 to prevent
 * repeat bond clears across subsequent reboots.
 *
 * @param void No parameters.
 * @return bool true if reset was successfully claimed, false otherwise.
 */
bool storage_claim_ble_bond_reset(void) {
    uint8_t claimed = 0U;
    if (!_storage_ready || nvs_get_u8(_storage_handle, STORAGE_BLE_RESET_KEY, &claimed) == ESP_OK) {
        return false;
    }
    return nvs_set_u8(_storage_handle, STORAGE_BLE_RESET_KEY, 1U) == ESP_OK &&
           nvs_commit(_storage_handle) == ESP_OK;
}