#include "FreqCounter.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

volatile uint32_t g_u32PulseCount = 0;
/**
 * @brief GPIO割り込みコールバック関数
 * @details
 * 指定したGPIOピンで立ち上がりエッジが発生するたびに呼び出され、
 * グローバル変数g_u32PulseCountをインクリメントします。
 * この関数はFreqCounter::startから割り込みハンドラとして登録されます。
 *
 * @param gpio   割り込みが発生したGPIOピン番号
 * @param events 割り込みイベント種別
 */
static void freqcounter_gpio_callback(uint gpio, uint32_t events) {
    g_u32PulseCount++;
}
/**
 * @brief 指定ピンのパルス周波数を1秒間測定する
 * @details
 * 指定したGPIOピン（a_u8PinNo）に入力されるパルス信号の立ち上がりエッジを
 * 1秒間カウントし、その合計値（周波数）を返します。内部で割り込みを利用して
 * パルスをカウントし、測定後は割り込みを無効化します。測定中はsleep_msで
 * ブロックされるため、他の処理は実行されません。
 *
 * @param a_u8PinNo 周波数を測定するGPIOピン番号
 * @return 1秒間に入力されたパルス数（周波数）
 */
uint32_t FreqCounter::start(uint8_t a_u8PinNo, uint16_t a_time) {
    g_u32PulseCount = 0;
    gpio_init(a_u8PinNo);
    gpio_set_dir(a_u8PinNo, GPIO_IN);
    gpio_pull_down(a_u8PinNo);
    gpio_set_irq_enabled_with_callback(a_u8PinNo, GPIO_IRQ_EDGE_RISE, true, &freqcounter_gpio_callback);
	sleep_ms(a_time); // 1秒間カウント
	gpio_set_irq_enabled(a_u8PinNo, GPIO_IRQ_EDGE_RISE, false);
    return g_u32PulseCount;
}
