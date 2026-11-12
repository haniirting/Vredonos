#pragma once
#include <furi.h>
#include <furi_hal_infrared.h>

// Непрерывно шлём NEC-подобный мусор — ТВ/кондиционер в прямой видимости
// перестают принимать команды (эффект "jammer").
void ir_jammer_run(volatile bool* stop_flag) {
    furi_hal_infrared_async_tx_set_data_is_manchester(false);
    furi_hal_infrared_async_tx_start(38000, 33); // 38 кГц, 33% duty

    while (!*stop_flag) {
        // импульс 9 мс + пауза 4.5 мс — "лидер" NEC, ломает декодеры
        furi_hal_infrared_async_tx_wait_termination();
        furi_hal_infrared_async_tx_start(38000, 33);
        furi_delay_us(9000);
        furi_hal_infrared_async_tx_stop();
        furi_delay_us(4500);
    }

    furi_hal_infrared_async_tx_stop();
    furi_hal_infrared_async_tx_wait_termination();
}
