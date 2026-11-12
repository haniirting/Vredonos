#pragma once
#include <furi.h>
#include <furi_hal_subghz.h>

typedef struct {
    uint32_t code;
    uint32_t bit;
    bool     sending;
} SubGhzBruteCtx;

// Колбэк: отдаёт LevelDuration для OOK-пакета.
// 1 = несущая включена, 0 = выключена. Длительность в микросекундах.
static LevelDuration subghz_brute_cb(void* context) {
    SubGhzBruteCtx* ctx = context;
    if(!ctx->sending) {
        return level_duration_reset();
    }

    // Формат: preamble (8 бит 10101010) + sync + 24-битный код.
    // Упрощённо: каждый бит — 500 мкс high + 500 мкс low для "1",
    // 500 мкс low + 500 мкс high для "0". Для реального протокола
    // тайминги надо подбирать под конкретный пульт.
    uint32_t total_bits = 32; // 8 preamble + 24 code
    if(ctx->bit >= total_bits) {
        ctx->sending = false;
        return level_duration_reset();
    }

    bool bit_val;
    if(ctx->bit < 8) {
        // preamble: чередование 1/0
        bit_val = (ctx->bit % 2) == 0;
    } else {
        uint32_t code_bit = 31 - (ctx->bit - 8); // старший бит кода первым
        bit_val = (ctx->code >> code_bit) & 1;
    }

    // Каждый бит = два уровня: несущая / пауза.
    // Для простоты: 500 мкс несущей + 500 мкс паузы для "1",
    // 500 мкс паузы + 500 мкс несущей для "0".
    // Реальные тайминги зависят от протокола (Princeton, CAME и т.д.).
    if(bit_val) {
        return level_duration_make(true, 500);  // high
    } else {
        return level_duration_make(false, 500); // low
    }
}

void subghz_brute_run(uint32_t freq, uint32_t start, uint32_t end,
                      volatile bool* stop_flag) {
    SubGhzBruteCtx ctx = {.code = start, .bit = 0, .sending = false};

    furi_hal_subghz_reset();
    furi_hal_subghz_set_frequency_and_path(freq);

    for(uint32_t code = start; code < end && !*stop_flag; code++) {
        ctx.code = code;
        ctx.bit = 0;
        ctx.sending = true;

        if(!furi_hal_subghz_start_async_tx(subghz_brute_cb, &ctx)) {
            break; // регион запретил TX
        }

        // Ждём завершения текущего пакета
        while(!furi_hal_subghz_is_async_tx_complete()) {
            furi_delay_ms(1);
        }
        furi_hal_subghz_stop_async_tx();

        furi_delay_ms(15); // пауза между кодами
    }

    furi_hal_subghz_sleep();
}
