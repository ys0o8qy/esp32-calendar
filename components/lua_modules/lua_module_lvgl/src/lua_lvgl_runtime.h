#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t lua_lvgl_lock(void);
void lua_lvgl_unlock(void);
esp_err_t lua_lvgl_ensure_initialized(void);

#ifdef __cplusplus
}
#endif
