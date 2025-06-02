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
#define PRESET_DEFAULT 0x3C96	// デフォルト値にリセットするためのダイレクトコマンド。
#define CALIB_RCO 0x3D96	// RCOキャリブレーションを開始するためのダイレクトコマンド。

// レジスタ
#define REG00_AFEGB_PWD 0x00 				// AFE GAin Boost / Power Down
#define REG01_NFLEV_WDTH 0x01       		// Noise Floor Level/ Watch Dog Threshold
#define REG02_CLSTAT_MINNUMLIGH_SREJ 0x02	// Clear Status / Minimum Number of Lightning / Spike Rejection
#define REG03_LCOFDIV_MDIST_INT 0x03        // LCO Frequency Division Ratio / Mask Disturber / Interrupt
#define REG04_S_LIGL 0x04                   // Energy of the Single Lightning LSBYTE
#define REG05_S_LIGM 0x05                   // Energy of the Single Lightning MSBYTE
#define REG06_S_LIGMM 0x06                  // Energy of the Single Lightning MMSBYTE
#define REG07_DIST 0x07                     // Distance estimation
#define REG08_LCO_SRCO_TRCO_CAP 0x08        // Display LCO on IRQ pin/ Display SRCO on IRQ pin/Display TRCO on IRQ pin/Internal Tuning Capacitors (from 0 to 120pF in steps of 8pF)

#define REG3A_TRCO_CALIBRSLT 0x3A           // Calibration of TRCO done (1=successful) / Calibration of TRCO unsuccessful(1 = not successful)
#define REG3B_SRCO_CALIBRSLT 0x3B 			// Calibration of SRCO done (1=successful) / Calibration of SRCO unsuccessful(1 = not successful)
//  Analog Front End Gain Boost
#define AFE_GB_MIN		(0b00000 << 1) 		// ANALOG FRONT END GAIN BOOST = 0 (min)
#define AFE_GB_OUTDOOR 	(0b01110 << 1)		// ANALOG FRONT END GAIN BOOST = 14 (outdoor)
#define AFE_GB_INDOOR 	(0b10010 << 1)		// ANALOG FRONT END GAIN BOOST = 18 (indoor)
#define AFE_GB_MAX      (0b11111 << 1)    	// ANALOG FRONT END GAIN BOOST = 31 (max)

// Noise Floor Level
#define NFLEV_0		(0x00 << 4) // NOISE FLOOR LEVEL = 0 (min)
#define NFLEV_1		(0x01 << 4) // NOISE FLOOR LEVEL = 1
#define NFLEV_2		(0x02 << 4) // NOISE FLOOR LEVEL = 2(default)
#define NFLEV_3		(0x03 << 4) // NOISE FLOOR LEVEL = 3
#define NFLEV_4		(0x04 << 4) // NOISE FLOOR LEVEL = 4
#define NFLEV_5		(0x05 << 4) // NOISE FLOOR LEVEL = 5
#define NFLEV_6		(0x06 << 4) // NOISE FLOOR LEVEL = 6
#define NFLEV_7		(0x07 << 4) // NOISE FLOOR LEVEL = 7 (max)	
#define NFLEV_DEF   (NFLEV_2) // デフォルトのノイズレベルを設定

// Watchdog Threshold  (0～10)
#define WDTH_DEFAULT 0x02   // WATCH DOG THRESHOLD 0x00 to 0x0F
#define WDTH (WDTH_DEFAULT) // デフォルトのウォッチドッグスレッショルドを設定

// Frequency Division Ratio
#define FDIV_RATIO_1_16 (0x00 << 6) // LCO Frequency Division Ratio = 1/16
#define FDIV_RATIO_1_32 (0x01 << 6) // LCO Frequency Division Ratio = 1/32
#define FDIV_RATIO_1_64 (0x02 << 6) // LCO Frequency Division Ratio = 1/64
#define FDIV_RATIO_1_128 (0x03 << 6) // LCO Frequency Division Ratio = 1/128

// Mask Disturber
#define MASK_DISTURBER_FALSE (0x00) // Mask Disturber = 0 (disable)
#define MASK_DISTURBER_TRUE (0x01)  // Mask Disturber = 1 (enable)
 
// Interrupt Noise LEVEL
#define INTNOISE_TOHIGH 0b0001 	// Noise level too high
#define INTNOISE_DISTERBERDETECT 0b0010 // Disturber detected
#define INTNOISE_LIGHTNINGINTR 0b1000  // Lightning interrupt

// Display LCO / ONにすると、TUN＿CAPで指定されたキャパシタの値に基づいてアンテナの共振周波数がデジタル信号として[IRQ] ピンに出力される。
#define DISPLCO_ON (0x1 << 7)
#define DISPLCO_OFF (0x0 << 7)
#define TUN_CAP_MASK (0x0F) // Tuning Capacitors (from 0 to 120pF in steps of 8pF)

AS3935::AS3935(Adafruit_ILI9341* a_pTft) :
	m_pTft(a_pTft)
{
    // 必要ならここで初期化
}
/**
 * @brief AS3935センサーの初期化を行う。
 * @details
 * 指定されたI2Cアドレス、I2Cポート番号、SDAピン、SCLピン、IRQピンを用いてAS3935センサーの初期化を行います。
 * まずI2C通信の初期化（InitI2C）を行い、失敗した場合はfalseを返します。
 * 次にIRQピンの初期化（gpio_init, gpio_set_dir）を行い、AS3935のレジスタをデフォルト値にリセットします（PresetDefault）。
 * 初期化が全て成功した場合はtrueを返します。
 * この関数はセンサー利用開始時に必ず呼び出してください。
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


	bool bRet = InitI2C(a_u8I2CAddress,a_u8I2cPort, a_u8SdaPin, a_u8SclPin);
	if (bRet == false) {
		return false; // I2C初期化失敗
	}
	// IRQピンの初期化
	gpio_init(m_u8IrqPin);
	gpio_set_dir(m_u8IrqPin, GPIO_IN);

	//AS3935の初期化処理
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
 * 初期化が全て成功した場合はtrueを返します。
 * この関数はセンサー利用開始時に必ず呼び出してください。
 */
void AS3935::StartCalibration()
{
	writeRegAndData_1(REG00_AFEGB_PWD, AFE_GB_INDOOR);
	writeRegAndData_1(REG01_NFLEV_WDTH,NFLEV_DEF | WDTH); // ノイズレベルとウォッチドッグスレッショルドを設定
	writeRegAndData_1(REG03_LCOFDIV_MDIST_INT, FDIV_RATIO_1_16 | MASK_DISTURBER_FALSE); // LCO Frequency Division Ratio = 1/16, Mask Disturber = 0, Interrupt = 0
	writeWord(CALIB_RCO);                                                               // RCOキャリブレーションを開始するためのダイレクトコマンドを送信
	Calibrate();                                                                        // キャリブレーションを実行
}

/**
 * @brief AS3935のキャリブレーションを実行する。
 * @details
 * 各キャパシタ値（0～15）ごとにIRQピンの周波数を測定し、frecCnt配列に格納します。
 * 測定した周波数はFDIV_RATIO_1_16の設定により1/16されているため、16倍して実際の周波数に戻します。
 * その後、各キャパシタ値での周波数と基準値（500000）との差分を計算し、最も差分が小さいキャパシタ値をm_u8calibratedCapに保存します。
 * また、測定結果をTFTディスプレイに表示します。
 * 最後に、キャリブレーションされたキャパシタ値をAS3935に設定し、IRQピンへの出力をオフにします。
 */
void AS3935::Calibrate() {
	uint32_t frecCnt[0x10] = {0}; // キャパシタごとの周波数カウント
	uint32_t minDif = 0xFFFFFFFF; // 最小の周波数分周比
	m_u8calibratedCap = 0; // キャリブレーションされたキャパシタの値
	// 周波数をカウントしていく
	for (byte b = 0; b < 0x10; b++) {
		writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, DISPLCO_ON | (TUN_CAP_MASK & b));
		delay(50);
		frecCnt[b] = FreqCounter::start(m_u8IrqPin);
		frecCnt[b] = frecCnt[b] * 16; // FDIV_RATIO_1_16 に指定してあるので、周波数は1/16になっている。ここで戻しておく。
		uint32_t curDif = frecCnt[b] >  500000 ? frecCnt[b] - 500000 : 500000 - frecCnt[b]; // 50MHzを基準にして、どれだけずれているかを計算
		if (curDif < minDif) {
			minDif = curDif; // 最小の周波数分周比を更新
			m_u8calibratedCap = b; // キャリブレーションされたキャパシタの値を更新
		}
		// m_pTft->printf("Cap:%3dpF Freq:%4.1fKHz\n", b * 8, (float)frecCnt[b] / 1000);
		m_pTft->printf("o");
	}
	m_pTft->printf("\n");
	m_pTft->printf("Cap:%3dpF Freq:%4.1fKHz\n", m_u8calibratedCap * 8, (float)frecCnt[m_u8calibratedCap] / 1000);
	writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, (DISPLCO_OFF | m_u8calibratedCap));  // キャリブレーションされたキャパシタの値を設定、IRQピンへの出力をオフにする
}
