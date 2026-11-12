#pragma once
#include <furi.h>
#include <furi_hal_subghz.h>
#include <lib/subghz/devices/devices.h>

// Простейший OOK-пакет: preamble 0xAAAA + sync + 24-битный код
static void subghz_send_code(uint32_t code) {
    uint8_t pkt[6] = {
        0xAA, 0xAA,                    // preamble
        0x2D, 0xD4,                    // sync (инвертированный)
        (uint8_t)(code >> 16),
        (uint8_t)(code >> 8),
        (uint8_t)(code),
    };
    furi_hal_subghz_tx(pkt, sizeof(pkt), 50); // 50 мкс на бит
}

// Брутфорс диапазона [start, end) на частоте freq (Гц)
// stop_flag — указатель на volatile bool для прерывания извне
void subghz_brute_run(uint32_t freq, uint32_t start, uint32_t end,
                      volatile bool* stop_flag) {
    furi_hal_subghz_reset();
    furi_hal_subghz_load_preset(FuriHalSubGhzPresetOok650Async);
    furi_hal_subghz_set_frequency(freq);
    furi_hal_subghz_set_path(FuriHalSubGhzPathIsolate); // антенна

    for (uint32_t code = start; code < end && !*stop_flag; code++) {
        furi_hal_subghz_tx_start();
        subghz_send_code(code);
        furi_hal_subghz_tx_stop();
        furi_delay_ms(15); // пауза между пакетами
    }

    furi_hal_subghz_sleep();
}
