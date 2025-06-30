/**
 * @file FreqCounter.cpp
 * @brief GPIOパルス周波数カウンタの実装
 * @details
 * - 指定したGPIOピンの立ち上がりエッジを割り込みでカウントし、
 *   1秒間（または指定時間）に入力されたパルス数（周波数）を返す。
 * - 割り込みコールバック関数とグローバルカウンタを利用。
 */
#include "FreqCounter.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

volatile uint32_t g_u32PulseCount = 0; ///< 割り込みでカウントされるパルス数
/**
 * @brief GPIO割り込みコールバック関数
 * @details
 * 指定したGPIOピンで立ち上がりエッジが発生するたびに呼び出され、
 * グローバル変数g_u32PulseCountをインクリメントします。
 * この関数はFreqCounter::startから割り込みハンドラとして登録されます。
 *
 * @param gpio   割り込みが発生したGPIOピン番号
 * @param events 割り込みイベント種別
 * @retval なし
 */
static void freqcounter_gpio_callback(uint gpio, uint32_t events) {
    g_u32PulseCount++; ///< パルスカウントをインクリメント
}
/**
 * @brief 指定ピンのパルス周波数を指定時間測定する
 * @details
 * 指定したGPIOピン（a_u8PinNo）に入力されるパルス信号の立ち上がりエッジを
 * 指定時間（a_time[ms]）カウントし、その合計値（周波数）を返します。
 * 内部で割り込みを利用してパルスをカウントし、測定後は割り込みを無効化します。
 * 測定中はsleep_msでブロックされるため、他の処理は実行されません。
 *
 * @param a_u8PinNo 周波数を測定するGPIOピン番号
 * @param a_time 測定時間[ms]（デフォルト1000ms）
 * @retval uint32_t 測定したパルス数（周波数）
 */
uint32_t FreqCounter::start(uint8_t a_u8PinNo, uint16_t a_time) {
    g_u32PulseCount = 0; ///< パルスカウンタ初期化
    gpio_init(a_u8PinNo); ///< GPIO初期化
    gpio_set_dir(a_u8PinNo, GPIO_IN); ///< 入力設定
    gpio_pull_down(a_u8PinNo); ///< プルダウン有効
    gpio_set_irq_enabled_with_callback(a_u8PinNo, GPIO_IRQ_EDGE_RISE, true, &freqcounter_gpio_callback); ///< 割り込み有効化
	sleep_ms(a_time); ///< 指定時間カウント
	gpio_set_irq_enabled(a_u8PinNo, GPIO_IRQ_EDGE_RISE, false); ///< 割り込み無効化
    return g_u32PulseCount; ///< 測定したパルス数を返す
}
