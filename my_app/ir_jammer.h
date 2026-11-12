#pragma once
#include <furi.h>
#include <furi_hal_infrared.h>

typedef struct {
    volatile bool* stop_flag;
    bool           phase; // false = high (несущая), true = low (пауза)
} IrJammerCtx;

static FuriHalInfraredTxGetDataState ir_jammer_data_cb(
    void* context,
    uint32_t* duration,
    bool* level) {
    IrJammerCtx* ctx = context;

    if(*ctx->stop_flag) {
        return FuriHalInfraredTxGetDataStateLastDone;
    }

    // NEC leader: 9000 мкс несущей, 4500 мкс паузы.
    if(!ctx->phase) {
        *duration = 9000;
        *level = true;
        ctx->phase = true;
    } else {
        *duration = 4500;
        *level = false;
        ctx->phase = false;
    }

    return FuriHalInfraredTxGetDataStateOk;
}

void ir_jammer_run(volatile bool* stop_flag) {
    IrJammerCtx ctx = {.stop_flag = stop_flag, .phase = false};

    furi_hal_infrared_async_tx_set_data_isr_callback(
        ir_jammer_data_cb, &ctx);

    // 38 кГц, 33% duty cycle — стандарт для бытовых IR-пультов.
    furi_hal_infrared_async_tx_start(38000, 33.0f);

    // Ждём, пока stop_flag не будет выставлен.
    while(!*stop_flag) {
        furi_delay_ms(10);
    }

    furi_hal_infrared_async_tx_stop();
    furi_hal_infrared_async_tx_wait_termination();
}
