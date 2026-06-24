set(VOICE_ASSISTANT_SRCS
    "src/voice_assistant.c"
    "src/voice_state.c"
    "src/voice_protocol.c"
    "src/voice_audio.c"
    "src/voice_transport_ws.c"
    "src/voice_board_waveshare_rlcd_4_2.c"
    "src/voice_esp_sr_local.c"
)
set(VOICE_ASSISTANT_PRIV_INCLUDE_DIRS "src")
set(VOICE_ASSISTANT_REQUIRES esp_timer)
set(VOICE_ASSISTANT_EMBED_TXTFILES "")

if(CONFIG_VOICE_ASSISTANT_WAVESHARE_CODEC_PORT)
    set(CODEC_BOARD_DIR
        "${CMAKE_CURRENT_LIST_DIR}/../../vendor/waveshare-esp32-s3-rlcd-4.2/02_Example/ESP-IDF/07_Audio_Test/components/ExternLib/codec_board")
    list(APPEND VOICE_ASSISTANT_SRCS
        "${CODEC_BOARD_DIR}/cfg_parse.c"
        "${CODEC_BOARD_DIR}/codec_board.c"
        "${CODEC_BOARD_DIR}/codec_init.c"
        "${CODEC_BOARD_DIR}/dummy_codec.c"
        "${CODEC_BOARD_DIR}/lcd_init.c"
        "${CODEC_BOARD_DIR}/drv/tca9554.c")
    list(APPEND VOICE_ASSISTANT_PRIV_INCLUDE_DIRS
        "${CODEC_BOARD_DIR}/include"
        "${CODEC_BOARD_DIR}/drv")
    list(APPEND VOICE_ASSISTANT_REQUIRES
        driver
        esp_codec_dev
        fatfs
        esp_lcd)
    if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.3")
        list(APPEND VOICE_ASSISTANT_REQUIRES esp_driver_i2c)
    endif()
    list(APPEND VOICE_ASSISTANT_EMBED_TXTFILES "${CODEC_BOARD_DIR}/board_cfg.txt")
endif()

if(CONFIG_VOICE_ASSISTANT_ESP_SR_LOCAL_RECOGNIZER)
    list(APPEND VOICE_ASSISTANT_REQUIRES esp-sr)
endif()

idf_component_register(
    SRCS
        ${VOICE_ASSISTANT_SRCS}
    INCLUDE_DIRS
        "include"
    PRIV_INCLUDE_DIRS
        ${VOICE_ASSISTANT_PRIV_INCLUDE_DIRS}
    REQUIRES
        ${VOICE_ASSISTANT_REQUIRES}
    EMBED_TXTFILES
        ${VOICE_ASSISTANT_EMBED_TXTFILES}
)
