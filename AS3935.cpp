#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "AS3935.h"
#include "FreqCounter.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

// https://www.ne.jp/asahi/shared/o-family/ElecRoom/AVRMCOM/AS3935/AS3935_test.html
// https://esphome.io/components/sensor/as3935.html

using namespace ardPort;
using namespace ardPort::spi;
// ダイレクトコマンド
#define PRESET_DEFAULT 0x3C96 // デフォルト値にリセットするためのダイレクトコマンド。
#define CALIB_RCO 0x3D96      // RCOキャリブレーションを開始するためのダイレクトコマンド。

// レジスタ
#define REG00_AFEGB_PWD 0x00              // AFE GAin Boost / Power Down
#define REG01_NFLEV_WDTH 0x01             // Noise Floor Level/ Watch Dog Threshold
#define REG02_CLSTAT_MINNUMLIGH_SREJ 0x02 // Clear Status / Minimum Number of Lightning / Spike Rejection
#define REG03_LCOFDIV_MDIST_INT 0x03      // LCO Frequency Division Ratio / Mask Disturber / Interrupt
#define REG04_S_LIGL 0x04                 // Energy of the Single Lightning LSBYTE
#define REG05_S_LIGM 0x05                 // Energy of the Single Lightning MSBYTE
#define REG06_S_LIGMM 0x06                // Energy of the Single Lightning MMSBYTE
#define REG07_DIST 0x07                   // Distance estimation
#define REG08_LCO_SRCO_TRCO_CAP 0x08      // Display LCO on IRQ pin/ Display SRCO on IRQ pin/Display TRCO on IRQ pin/Internal Tuning Capacitors (from 0 to 120pF in steps of 8pF)

#define REG3A_TRCO_CALIBRSLT 0x3A // Calibration of TRCO done (1=successful) / Calibration of TRCO unsuccessful(1 = not successful)
#define REG3B_SRCO_CALIBRSLT 0x3B // Calibration of SRCO done (1=successful) / Calibration of SRCO unsuccessful(1 = not successful)
//  Analog Front End Gain Boost
#define AFE_GB_MIN (0b00000 << 1)     // ANALOG FRONT END GAIN BOOST = 0 (min)
#define AFE_GB_OUTDOOR (0b01110 << 1) // ANALOG FRONT END GAIN BOOST = 14 (outdoor)
#define AFE_GB_INDOOR (0b10010 << 1)  // ANALOG FRONT END GAIN BOOST = 18 (indoor)
#define AFE_GB_MAX (0b11111 << 1)     // ANALOG FRONT END GAIN BOOST = 31 (max)

// Noise Floor Level
#define NFLEV_0 (0x00 << 4) // NOISE FLOOR LEVEL = 0 (min)
#define NFLEV_1 (0x01 << 4) // NOISE FLOOR LEVEL = 1
#define NFLEV_2 (0x02 << 4) // NOISE FLOOR LEVEL = 2(default)
#define NFLEV_3 (0x03 << 4) // NOISE FLOOR LEVEL = 3
#define NFLEV_4 (0x04 << 4) // NOISE FLOOR LEVEL = 4
#define NFLEV_5 (0x05 << 4) // NOISE FLOOR LEVEL = 5
#define NFLEV_6 (0x06 << 4) // NOISE FLOOR LEVEL = 6
#define NFLEV_7 (0x07 << 4) // NOISE FLOOR LEVEL = 7 (max)
#define NFLEV_DEF (NFLEV_2) // デフォルトのノイズレベルを設定

// Watchdog Threshold  (0～10)
#define WDTH_DEFAULT 0x02   // WATCH DOG THRESHOLD 0x00 to 0x0F
#define WDTH (WDTH_DEFAULT) // デフォルトのウォッチドッグスレッショルドを設定

// Frequency Division Ratio
#define FDIV_RATIO_1_16 (0x00 << 6)  // LCO Frequency Division Ratio = 1/16
#define FDIV_RATIO_1_32 (0x01 << 6)  // LCO Frequency Division Ratio = 1/32
#define FDIV_RATIO_1_64 (0x02 << 6)  // LCO Frequency Division Ratio = 1/64
#define FDIV_RATIO_1_128 (0x03 << 6) // LCO Frequency Division Ratio = 1/128

// Mask Disturber
#define MASK_DISTURBER_FALSE (0x00) // Mask Disturber = 0 (disable)
#define MASK_DISTURBER_TRUE (0x01)  // Mask Disturber = 1 (enable)

// Interrupt Noise LEVEL
#define INTNOISE_TOHIGH 0b0001          // Noise level too high
#define INTNOISE_DISTERBERDETECT 0b0010 // Disturber detected
#define INTNOISE_LIGHTNINGINTR 0b1000   // Lightning interrupt

// Display LCO / ONにすると、TUN＿CAPで指定されたキャパシタの値に基づいてアンテナの共振周波数がデジタル信号として[IRQ] ピンに出力される。
#define DISPLCO_ON (0x1 << 7)
#define DISPLCO_OFF (0x0 << 7)
#define TUN_CAP_MASK (0x0F) // Tuning Capacitors (from 0 to 120pF in steps of 8pF)

AS3935::AS3935(Adafruit_ILI9341* a_pTft) :
	m_pTft(a_pTft),
	m_bufAlarmSummary(20),
	m_bufAlarmDist(20),
	m_bufAlarmStatus(20),
	m_bufFalseAlarmSummary(20),
	m_bufFalseAlarmDist(20),
	m_bufFalseAlarmStatus(20)

{
	// 必要ならここで初期化
}
/**
 * @brief AS3935センサーの初期化を行う。
 * @details
 * この関数は、AS3935雷センサーの初期化を行います。
 * 指定されたI2Cアドレス、I2Cポート番号、SDAピン、SCLピン、IRQピンを用いて、
 * I2C通信の初期化、IRQピンの初期化（gpio_init, gpio_set_dir）、
 * そしてAS3935のレジスタをデフォルト値にリセット（PresetDefault）します。
 * いずれかの初期化に失敗した場合はfalseを返します。
 * センサー利用開始時に必ず呼び出してください。
 *
 * @param a_u8I2CAddress AS3935のI2Cアドレス
 * @param a_u8I2cPort    使用するI2Cポート番号
 * @param a_u8SdaPin     SDAピン番号
 * @param a_u8SclPin     SCLピン番号
 * @param a_u8IrqPin     IRQピン番号
 * @return 初期化が成功した場合はtrue、失敗した場合はfalse
 */
bool AS3935::Init(uint8_t a_u8I2CAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin)
{
	m_u8IrqPin = a_u8IrqPin;

	bool bRet = InitI2C(a_u8I2CAddress, a_u8I2cPort, a_u8SdaPin, a_u8SclPin);
	if (bRet == false) {
		return false; // I2C初期化失敗
	}
	// IRQピンの初期化
	gpio_init(m_u8IrqPin);
	gpio_set_dir(m_u8IrqPin, GPIO_IN);

	// AS3935の初期化処理
	PresetDefault();

	return bRet; // 初期化成功
}
/// @brief AS3935をデフォルトにリセットするためのダイレクトコマンド（データーシートのAS3935–23)を送信する
/// @details この関数は、AS3935センサーをデフォルトの状態にリセットします。
void AS3935::PresetDefault()
{
	writeWord(PRESET_DEFAULT); // デフォルト値にリセットするためのダイレクトコマンドを送信
}
/**
 * @brief AS3935の初期化を行う。
 * @details
 * 指定されたI2Cアドレス、I2Cポート、SDA/SCLピン、IRQピンを用いてAS3935センサーの初期化を行います。
 * まずI2C通信の初期化を行い、失敗した場合はfalseを返します。
 * 次にIRQピンの初期化（gpio_init, gpio_set_dir）を行い、AS3935のレジスタをデフォルト値にリセットします（PresetDefault）。
 * a_timeCalibrationが0の場合はキャリブレーションをスキップし、デフォルト値（4）を設定します。
 * 初期化が全て成功した場合はtrueを返します。
 * この関数はセンサー利用開始時に必ず呼び出してください。
 */
void AS3935::StartCalibration(uint16_t a_timeCalibration)
{
	m_timeCalibration = a_timeCalibration; // キャリブレーション時間を設定
	if (m_timeCalibration == 0) {
		m_u8calibratedCap = 4;
	} else {
		writeRegAndData_1(REG00_AFEGB_PWD, AFE_GB_INDOOR);
		writeRegAndData_1(REG01_NFLEV_WDTH, NFLEV_DEF | WDTH);                              // ノイズレベルとウォッチドッグスレッショルドを設定
		writeRegAndData_1(REG03_LCOFDIV_MDIST_INT, FDIV_RATIO_1_16 | MASK_DISTURBER_FALSE); // LCO Frequency Division Ratio = 1/16, Mask Disturber = 0, Interrupt = 0
		writeWord(CALIB_RCO);                                                               // RCOキャリブレーションを開始するためのダイレクトコマンドを送信
		m_FreqCalibration = Calibrate();                                                       // キャリブレーションを実行
	}
	m_pTft->printf("Cap:%3dpF Freq:%4.1fKHz\n", m_u8calibratedCap * 8, (float)m_FreqCalibration / 1000);
	writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, (DISPLCO_OFF | m_u8calibratedCap)); // キャリブレーションされたキャパシタの値を設定、IRQピンへの出力をオフにする
}

/**
 * @brief AS3935のキャリブレーションを実行する。
 * @details
 * 各キャパシタ値（0～15）ごとにIRQピンの周波数を測定し、frecCnt配列に格納します。
 * 測定した周波数はFDIV_RATIO_1_16の設定により1/16されているため、16倍して実際の周波数に戻します。
 * その後、各キャパシタ値での周波数と基準値（500000Hz）との差分を計算し、最も差分が小さいキャパシタ値をm_u8calibratedCapに保存します。
 * 測定結果はTFTディスプレイに表示され、最適なキャパシタ値をAS3935に設定します。
 *
 * @return キャリブレーションで選択されたキャパシタ値での実測周波数（Hz）
 */
uint32_t AS3935::Calibrate()
{
	uint32_t frecCnt[0x10] = {0}; // キャパシタごとの周波数カウント
	uint32_t minDif = 0xFFFFFFFF; // 最小の周波数分周比
	m_u8calibratedCap = 0;        // キャリブレーションされたキャパシタの値
	// 周波数をカウントしていく
	for (byte b = 0; b < 0x10; b++) {
		writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, DISPLCO_ON | (TUN_CAP_MASK & b));
		delay(50);
		// 指定した秒数（デフォルト１秒）の間、IRQピンの周波数をカウントする
		frecCnt[b] = FreqCounter::start(m_u8IrqPin, m_timeCalibration);
		// カウントを1000msに換算しておく
		if (m_timeCalibration != 1000) {
			frecCnt[b] = uint32_t(((double)frecCnt[b] / (double)m_timeCalibration) * 1000); // 1秒間のパルス数（周波数）を取得
		}

		frecCnt[b] = frecCnt[b] * 16;                                                      // FDIV_RATIO_1_16 に指定してあるので、周波数は1/16になっている。ここで戻しておく。
		uint32_t curDif = frecCnt[b] > 500000 ? frecCnt[b] - 500000 : 500000 - frecCnt[b]; // 50MHzを基準にして、どれだけずれているかを計算
		if (curDif < minDif) {
			minDif = curDif;       // 最小の周波数分周比を更新
			m_u8calibratedCap = b; // キャリブレーションされたキャパシタの値を更新
		}
		// m_pTft->printf("Cap:%3dpF Freq:%4.1fKHz\n", b * 8, (float)frecCnt[b] / 1000);
		m_pTft->printf("o");
	}
	m_pTft->printf("\n");
	return frecCnt[m_u8calibratedCap];
}

/**
 * @brief 雷検出後の信号検証を行う。
 * @details
 * AS3935のIRQ割り込み後に呼び出し、雷検出信号が有効かどうかを判定します。
 * レジスタREG03_LCOFDIV_MDIST_INTのINTフラグや、REG07_DISTの距離推定値、
 * REG02_CLSTAT_MINNUMLIGH_SREJのステータスなどを参照し、
 * ノイズやディスターバ（誤検出）でないかを判定します。
 *
 * @return 有効な雷信号と判定した場合true、ノイズや誤検出の場合はfalse
 */
bool AS3935::validateSignal()
{
    // 割り込み発生時にREG03_LCOFDIV_MDIST_INTのINTフラグを確認
    uint8_t u8IntSrc = readReg(REG03_LCOFDIV_MDIST_INT) & 0x0F;
    // INTNOISE_LIGHTNINGINTR: 雷検出
    // INTNOISE_DISTERBERDETECT: ディスターバ（誤検出）
    // INTNOISE_TOHIGH: ノイズレベル過大
    if (u8IntSrc & INTNOISE_LIGHTNINGINTR) {
        // 距離推定値を取得（0x00: 検出なし, 0x01-0x3F: 距離, 0x3F: 遠すぎる）
        uint8_t u8Dist = readReg(REG07_DIST) & 0x3F;
        // ステータスレジスタでノイズ/誤検出を確認
        uint8_t u8Status = readReg(REG02_CLSTAT_MINNUMLIGH_SREJ);
        // 距離が有効範囲内かつ、ノイズ/誤検出でなければtrue
        if (u8Dist > 0 && u8Dist < 0x3F && (u8Status & 0x0F) == 0) {
			m_bufAlarmSummary.push(SUMM_THUNDER);
			m_bufAlarmDist.push(u8Dist); // 距離をリングバッファに保存
			m_bufAlarmStatus.push(u8Status); // ステータスをリングバッファに保存
            return true;
        } else {
			if (u8Dist < 0 || u8Dist > 0x3F) {
				m_bufAlarmSummary.push(SUMM_TOOFAR);
			} else {
				m_bufAlarmSummary.push(SUMM_NUMZERO);
			}
			m_bufFalseAlarmDist.push(u8Dist);     // 距離をリングバッファに保存
			m_bufFalseAlarmStatus.push(u8Status); // ステータスをリングバッファに保存
			return false;                         // 距離が無効、またはノイズ/誤検出の場合はfalse
		}
    } else if (u8IntSrc & INTNOISE_DISTERBERDETECT) {
		m_bufFalseAlarmDist.push(SUMM_DISTERBER); // ディスターバ（誤検出）をリングバッファに保存;
		m_bufFalseAlarmDist.push(0);     // 距離をリングバッファに保存
		m_bufFalseAlarmStatus.push(0); // ステータスをリングバッファに保存
	} else if (u8IntSrc & INTNOISE_TOHIGH) {
		m_bufFalseAlarmDist.push(SUMM_NOISEHIGH); // ノイズレベル過大をリングバッファに保存
		m_bufFalseAlarmDist.push(0);              // 距離をリングバッファに保存
		m_bufFalseAlarmStatus.push(0);            // ステータスをリングバッファに保存
	}
	return false;
	
}

/**
 * @brief 指定レジスタから1バイト読み出す
 * @details
 * AS3935のI2Cレジスタから1バイト値を読み出します。
 *
 * @param reg レジスタアドレス
 * @return 読み出した値
 */
uint8_t AS3935::readReg(uint8_t reg)
{
    uint8_t val = 0;
    // レジスタアドレス送信（リピートスタート）
    i2c_write_blocking((m_u8I2cPort == 0 ? i2c0 : i2c1), m_u8I2CAddress, &reg, 1, true);
    // データ受信
    i2c_read_blocking((m_u8I2cPort == 0 ? i2c0 : i2c1), m_u8I2CAddress, &val, 1, false);
    return val;
}

bool AS3935::GetLatestAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, uint8_t& a_u8AlarmStatus)
{
	a_u8AlarmSummary = m_bufAlarmSummary.getFromLast(idx);
	a_u8AlarmDist = m_bufAlarmDist.getFromLast(idx);
	a_u8AlarmStatus = m_bufAlarmStatus.getFromLast(idx);
	if (a_u8AlarmSummary == 0) return false;
	return true;
}

bool AS3935::GetLatestFalseAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, uint8_t& a_u8AlarmStatus)
{
	a_u8AlarmSummary = m_bufFalseAlarmSummary.getFromLast(idx);
	a_u8AlarmDist = m_bufFalseAlarmDist.getFromLast(idx);
	a_u8AlarmStatus = m_bufFalseAlarmStatus.getFromLast(idx);
	if (a_u8AlarmSummary == 0) return false;
	return true;
}