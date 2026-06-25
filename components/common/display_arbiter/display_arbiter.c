/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "display_arbiter.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "display_arbiter";

#define DISPLAY_ARBITER_MAX_CALLBACKS 4

typedef struct {
    display_arbiter_owner_changed_cb_t callback;
    void *user_ctx;
} display_arbiter_callback_t;

typedef struct {
    SemaphoreHandle_t lock;
    display_arbiter_owner_t owner;
    display_arbiter_owner_t owner_before_lua;
    uint32_t lua_depth;
    display_arbiter_callback_t callbacks[DISPLAY_ARBITER_MAX_CALLBACKS];
} display_arbiter_state_t;

static display_arbiter_state_t s_state = {
    .owner = DISPLAY_ARBITER_OWNER_NONE,
    .owner_before_lua = DISPLAY_ARBITER_OWNER_NONE,
};

static esp_err_t display_arbiter_lock(void)
{
    if (!s_state.lock) {
        s_state.lock = xSemaphoreCreateMutex();
    }
    ESP_RETURN_ON_FALSE(s_state.lock != NULL, ESP_ERR_NO_MEM, TAG, "create mutex failed");
    ESP_RETURN_ON_FALSE(xSemaphoreTake(s_state.lock, pdMS_TO_TICKS(1000)) == pdTRUE,
                        ESP_ERR_TIMEOUT,
                        TAG,
                        "mutex timeout");
    return ESP_OK;
}

static void display_arbiter_unlock(void)
{
    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }
}

static bool display_arbiter_valid_owner(display_arbiter_owner_t owner)
{
    return owner == DISPLAY_ARBITER_OWNER_CALENDAR ||
           owner == DISPLAY_ARBITER_OWNER_LUA ||
           owner == DISPLAY_ARBITER_OWNER_EMOTE;
}

static const char *display_arbiter_owner_to_str(display_arbiter_owner_t owner)
{
    switch (owner) {
    case DISPLAY_ARBITER_OWNER_NONE:
        return "none";
    case DISPLAY_ARBITER_OWNER_CALENDAR:
        return "calendar";
    case DISPLAY_ARBITER_OWNER_LUA:
        return "lua";
    case DISPLAY_ARBITER_OWNER_EMOTE:
        return "emote";
    default:
        return "unknown";
    }
}

static esp_err_t display_arbiter_change_owner_locked(display_arbiter_owner_t owner)
{
    s_state.owner = owner;
    ESP_LOGI(TAG, "display owner changed to %s", display_arbiter_owner_to_str(owner));
    return ESP_OK;
}

static void display_arbiter_notify_owner_changed(display_arbiter_owner_t owner)
{
    for (size_t i = 0; i < DISPLAY_ARBITER_MAX_CALLBACKS; i++) {
        if (s_state.callbacks[i].callback) {
            s_state.callbacks[i].callback(owner, s_state.callbacks[i].user_ctx);
        }
    }
}

esp_err_t display_arbiter_acquire(display_arbiter_owner_t owner)
{
    esp_err_t ret = display_arbiter_lock();
    display_arbiter_owner_t notify_owner = DISPLAY_ARBITER_OWNER_NONE;
    bool owner_changed = false;

    if (ret != ESP_OK) {
        return ret;
    }

    ESP_GOTO_ON_FALSE(display_arbiter_valid_owner(owner), ESP_ERR_INVALID_ARG, fail, TAG, "invalid owner");

    if (owner == DISPLAY_ARBITER_OWNER_LUA) {
        if (s_state.lua_depth == 0) {
            s_state.owner_before_lua = s_state.owner;
        }
        s_state.lua_depth++;
    }

    if (s_state.owner != owner) {
        ESP_GOTO_ON_ERROR(display_arbiter_change_owner_locked(owner), fail, TAG, "switch owner failed");
        notify_owner = owner;
        owner_changed = true;
    }

fail:
    display_arbiter_unlock();
    if (ret == ESP_OK && owner_changed) {
        display_arbiter_notify_owner_changed(notify_owner);
    }
    return ret;
}

esp_err_t display_arbiter_release(display_arbiter_owner_t owner)
{
    esp_err_t ret = display_arbiter_lock();
    display_arbiter_owner_t notify_owner = DISPLAY_ARBITER_OWNER_NONE;
    bool owner_changed = false;

    if (ret != ESP_OK) {
        return ret;
    }

    ESP_GOTO_ON_FALSE(display_arbiter_valid_owner(owner), ESP_ERR_INVALID_ARG, fail, TAG, "invalid owner");

    if (owner == DISPLAY_ARBITER_OWNER_LUA) {
        ESP_GOTO_ON_FALSE(s_state.lua_depth > 0, ESP_ERR_INVALID_STATE, fail, TAG, "lua owner is not active");
        s_state.lua_depth--;
        if (s_state.lua_depth == 0 && s_state.owner == DISPLAY_ARBITER_OWNER_LUA) {
            display_arbiter_owner_t restore_owner = s_state.owner_before_lua;
            if (restore_owner == DISPLAY_ARBITER_OWNER_NONE || restore_owner == DISPLAY_ARBITER_OWNER_LUA) {
                restore_owner = DISPLAY_ARBITER_OWNER_CALENDAR;
            }
            ESP_GOTO_ON_ERROR(display_arbiter_change_owner_locked(restore_owner), fail, TAG, "restore owner failed");
            notify_owner = restore_owner;
            owner_changed = true;
        }
    } else if (s_state.owner == owner) {
        display_arbiter_owner_t restore_owner = owner == DISPLAY_ARBITER_OWNER_CALENDAR
                                                ? DISPLAY_ARBITER_OWNER_NONE
                                                : DISPLAY_ARBITER_OWNER_CALENDAR;
        ESP_GOTO_ON_ERROR(display_arbiter_change_owner_locked(restore_owner), fail, TAG, "release owner failed");
        notify_owner = restore_owner;
        owner_changed = true;
    }

fail:
    display_arbiter_unlock();
    if (ret == ESP_OK && owner_changed) {
        display_arbiter_notify_owner_changed(notify_owner);
    }
    return ret;
}

display_arbiter_owner_t display_arbiter_get_owner(void)
{
    return s_state.owner;
}

bool display_arbiter_is_owner(display_arbiter_owner_t owner)
{
    return display_arbiter_get_owner() == owner;
}

esp_err_t display_arbiter_set_owner_changed_callback(display_arbiter_owner_changed_cb_t callback, void *user_ctx)
{
    esp_err_t ret = display_arbiter_lock();

    if (ret != ESP_OK) {
        return ret;
    }

    s_state.callbacks[0].callback = callback;
    s_state.callbacks[0].user_ctx = user_ctx;
    display_arbiter_unlock();
    return ESP_OK;
}

esp_err_t display_arbiter_register_owner_changed_callback(display_arbiter_owner_changed_cb_t callback, void *user_ctx)
{
    esp_err_t ret = display_arbiter_lock();

    if (ret != ESP_OK) {
        return ret;
    }

    if (callback == NULL) {
        display_arbiter_unlock();
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < DISPLAY_ARBITER_MAX_CALLBACKS; i++) {
        if (s_state.callbacks[i].callback == callback && s_state.callbacks[i].user_ctx == user_ctx) {
            display_arbiter_unlock();
            return ESP_OK;
        }
    }

    for (size_t i = 0; i < DISPLAY_ARBITER_MAX_CALLBACKS; i++) {
        if (s_state.callbacks[i].callback == NULL) {
            s_state.callbacks[i].callback = callback;
            s_state.callbacks[i].user_ctx = user_ctx;
            display_arbiter_unlock();
            return ESP_OK;
        }
    }

    display_arbiter_unlock();
    return ESP_ERR_NO_MEM;
}
