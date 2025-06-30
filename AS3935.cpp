#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "AS3935.h"
#include "FreqCounter.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "settings.h"
#include <ctime>

#include "printfDebug.h"

// https://www.ne.jp/asahi/shared/o-family/ElecRoom/AVRMCOM/AS3935/AS3935_test.html
// https://esphome.io/components/sensor/as3935.html

using namespace ardPort;
using namespace ardPort::spi;
// ダイレクトコマンド
#define PRESET_DEFAULT 0x3C96 ///< デフォルト値にリセットするためのダイレクトコマンド
#define CALIB_RCO 0x3D96      ///< RCOキャリブレーションを開始するためのダイレクトコマンド

// レジスタ
#define REG00_AFEGB_PWD 0x00              ///< AFE GAin Boost / Power Down レジスタアドレス
#define REG01_NFLEV_WDTH 0x01             ///< Noise Floor Level/ Watch Dog Threshold レジスタアドレス
#define REG02_CLSTAT_MINNUMLIGH_SREJ 0x02 ///< Clear Status / Minimum Number of Lightning / Spike Rejection レジスタアドレス
#define REG03_LCOFDIV_MDIST_INT 0x03      ///< LCO Frequency Division Ratio / Mask Disturber / Interrupt レジスタアドレス
#define REG04_S_LIGL 0x04                 ///< Energy of the Single Lightning LSBYTE レジスタアドレス
#define REG05_S_LIGM 0x05                 ///< Energy of the Single Lightning MSBYTE レジスタアドレス
#define REG06_S_LIGMM 0x06                ///< Energy of the Single Lightning MMSBYTE レジスタアドレス
#define REG07_DIST 0x07                   ///< Distance estimation レジスタアドレス
#define REG08_LCO_SRCO_TRCO_CAP 0x08      ///< LCO/SRCO/TRCO/キャパシタ設定レジスタアドレス

#define REG3A_TRCO_CALIBRSLT 0x3A ///< TRCOキャリブレーション結果レジスタアドレス
#define REG3B_SRCO_CALIBRSLT 0x3B ///< SRCOキャリブレーション結果レジスタアドレス

// Frequency Division Ratio
#define FDIV_RATIO_1_16 (0x00 << 6)  ///< LCO分周比=1/16
#define FDIV_RATIO_1_32 (0x01 << 6)  ///< LCO分周比=1/32
#define FDIV_RATIO_1_64 (0x02 << 6)  ///< LCO分周比=1/64
#define FDIV_RATIO_1_128 (0x03 << 6) ///< LCO分周比=1/128

// Mask Disturber
#define MASK_DISTURBER_FALSE (0x00) ///< Mask Disturber=0（無効）
#define MASK_DISTURBER_TRUE (0x01)  ///< Mask Disturber=1（有効）

// Interrupt Noise LEVEL
#define INTNOISE_TOHIGH 0b0001          ///< ノイズレベル過大割り込み
#define INTNOISE_DISTERBERDETECT 0b0010 ///< ディスターバ検出割り込み
#define INTNOISE_LIGHTNINGINTR 0b1000   ///< 雷検出割り込み
#define INTNOISE_CLEARSTATSTICS 0b0000  ///< 統計情報クリア割り込み

// Display LCO
#define DISPLCO_ON (0x1 << 7)  ///< LCO出力ON
#define DISPLCO_OFF (0x0 << 7) ///< LCO出力OFF
#define TUN_CAP_MASK (0x0F)    ///< チューニングキャパシタマスク（0～120pF/8pF刻み）



// 設定情報
extern Settings settings;

AS3935::AS3935(Adafruit_ILI9341* a_pTft) :
	m_pTft(a_pTft),
	m_bufAlarmSummary(100),
	m_bufAlarmDist(100),
	m_bufSingleEnergy(100),
	m_bufDateTime(100)
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
	bRet = PresetDefault();

	return bRet;
}
/// @brief AS3935をデフォルトにリセットするためのダイレクトコマンド（データーシートのAS3935–23)を送信する
/// @details この関数は、AS3935センサーをデフォルトの状態にリセットします。
bool AS3935::PresetDefault()
{
	int ret = writeWord(PRESET_DEFAULT); // デフォルト値にリセットするためのダイレクトコマンドを送信
	if (ret < 0) return false;
	return true;
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
		writeRegAndData_1(REG00_AFEGB_PWD, (settings.value.gainBoost << 1));
		writeRegAndData_1(REG01_NFLEV_WDTH, (settings.value.noiseFloor << 4) | settings.value.watchDogThreshold); // ノイズレベルとウォッチドッグスレッショルドを設定
		writeRegAndData_1(REG03_LCOFDIV_MDIST_INT, FDIV_RATIO_1_16 | MASK_DISTURBER_FALSE);                       // LCO Frequency Division Ratio = 1/16, Mask Disturber = 0, Interrupt = 0
		/*
		writeRegAndData_1(REG00_AFEGB_PWD, (AFE_GB_INDOOR << 1));
		writeRegAndData_1(REG01_NFLEV_WDTH, (NFLEV_DEF << 4) | WDTH_DEFAULT);               // ノイズレベルとウォッチドッグスレッショルドを設定
		*/
		writeWord(CALIB_RCO);            // RCOキャリブレーションを開始するためのダイレクトコマンドを送信
		m_FreqCalibration = Calibrate(); // キャリブレーションを実行
	}
	dbgprintf("Cap:%3dpF Freq:%4.1fKHz\n", m_u8calibratedCap * 8, (float)m_FreqCalibration / 1000);
	writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, (DISPLCO_OFF | m_u8calibratedCap)); // キャリブレーションされたキャパシタの値を設定、IRQピンへの出力をオフにする
	// AFEのゲインブースト、ノイズフロアレベル、ウォッチドッグスレッショルドを設定
	Reset(); // AS3935をリセットして、設定を適用する

	/*
	writeRegAndData_1(REG00_AFEGB_PWD, (settings.value.gainBoost << 1));
	writeRegAndData_1(REG01_NFLEV_WDTH, (settings.value.noiseFloor << 4) | settings.value.watchDogThreshold); // ノイズレベルとウォッチドッグスレッショルドを設定
	writeRegAndData_1(REG02_CLSTAT_MINNUMLIGH_SREJ, (settings.value.minimumEvent << 4) || settings.value.spikeReject); // 最小イベント数とスパイクリジェクトを設定
	writeRegAndData_1(REG03_LCOFDIV_MDIST_INT, FDIV_RATIO_1_16 | MASK_DISTURBER_FALSE);                                // LCO Frequency Division Ratio = 1/16, Mask Disturber = 0, Interrupt = 0
	*/
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
		dbgprintf("o");
	}
	dbgprintf("\n");
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
AS3935_SIGNAL AS3935::validateSignal()
{
	/*
	テスト。検出を強制的に登録する
	*/
#if false
	static uint8_t callcnt = 0;
	if (callcnt % 4 == 0) {
		m_bufAlarmSummary.push(SUMM_THUNDER);
		m_bufAlarmDist.push(0x20);                  // 距離をリングバッファに保存
		m_bufAlarmStatus.push(0x01);                // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));             // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::VALID; // 最新の信号が有効かどうかを保存
		m_latestBufAlarmSummary = SUMM_THUNDER;
		m_latestBufAlarmDist = 0x20;
		m_latestBufAlarmStatus = 0x01;
		m_latestBufDateTime = m_bufDateTime.getLastData();
	} else if (callcnt % 4 == 1) {
		m_bufAlarmSummary.push(SUMM_NOISEHIGH);
		m_bufAlarmDist.push(0);                       // 距離をリングバッファに保存
		m_bufAlarmStatus.push(0);                     // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));               // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::INVALID; // 最新の信号が有効かどうかを保存
		m_latestBufAlarmSummary = SUMM_NOISEHIGH;
		m_latestBufAlarmDist = 0;
		m_latestBufAlarmStatus = 0;
		m_latestBufDateTime = m_bufDateTime.getLastData();
	} else if (callcnt % 4 == 2) {
		m_bufAlarmSummary.push(SUMM_DISTERBER);
		m_bufAlarmDist.push(0);                       // 距離をリングバッファに保存
		m_bufAlarmStatus.push(0);                     // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));               // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::INVALID; // 最新の信号が有効かどうかを保存
		m_latestBufAlarmSummary = SUMM_DISTERBER;
		m_latestBufAlarmDist = 0;
		m_latestBufAlarmStatus = 0;
		m_latestBufDateTime = m_bufDateTime.getLastData();
	} else if (callcnt % 4 == 3) {
		m_bufAlarmSummary.push(SUMM_TOOFAR);
		m_bufAlarmDist.push(0x3F);                  // 距離をリングバッファに保存（遠すぎる）
		m_bufAlarmStatus.push(0);                   // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));             // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::INVALID; // 最新の信号が有効かどうかを保存
		m_latestBufAlarmSummary = SUMM_TOOFAR;
		m_latestBufAlarmDist = 0x3F;
		m_latestBufAlarmStatus = 0;
		m_latestBufDateTime = m_bufDateTime.getLastData();
	} else {
		m_latestSignalValid = AS3935_SIGNAL::NONE; // 信号が存在しない
		m_latestBufAlarmSummary = SUMM_NONE;
		m_latestBufAlarmDist = 0;
		m_latestBufAlarmStatus = 0;
		m_latestBufDateTime = m_bufDateTime.getLastData();
	}
	callcnt++;
	if (callcnt > 100) callcnt = 0; // カウントをリセット
	return m_latestSignalValid;
#endif

	// 割り込み発生時にREG03_LCOFDIV_MDIST_INTのINTフラグを確認

	// static bool isFirstCall = true; // 初回呼び出しフラグ。最初に１回、なぜか割り込みがかかってしまうので、最初の１回は無視する
	uint8_t u8IntSrc = 0; 
	uint8_t u8Dist = 0;
	uint8_t u8LIGL;
	uint8_t u8LIGM;
	uint8_t u8LIGMM;
	unsigned long lEnergy = 0; // 雷のエネルギーを格納する変数

	if (settings.geti2cReadMode() == 1) {
		uint8_t regBuffer[9] = {0};
		readBlockReg(regBuffer); // レジスタの値を読み込む
		dbgprintf("validateSignal: I2C Bulk Read : ");
		for (int i = 0; i < 9; i++) {
			dbgprintf("  %d-", regBuffer[i]);
		}
		dbgprintf("\n");
		u8IntSrc = regBuffer[3] & 0x0F; // REG03_LCOFDIV_MDIST_INTの下位4ビットを取得
		u8Dist = regBuffer[7] & 0x3F;           // 距離推定値を取得（0x00: 検出なし, 0x01-0x3F: 距離, 0x3F: 遠すぎる）
		u8LIGL = regBuffer[4];                  // 雷のエネルギーを読み込む
		u8LIGM = regBuffer[5];                  // 雷のエネルギーを読み込む
		u8LIGMM = regBuffer[6];                 // 雷のエネルギーを読み込む

	} else {
		dbgprintf("validateSignal: I2C Single Read\n");
		u8IntSrc = readReg(REG03_LCOFDIV_MDIST_INT) & 0x0F;
		dbgprintf("validateSignal: I2C Single Read : u8IntSrc:%02X\n", u8IntSrc);
		u8Dist = readReg(REG07_DIST) /*& 0x3F*/; // 距離推定値を取得（0x00: 検出なし, 0x01-0x3F: 距離, 0x3F: 遠すぎる）
		u8LIGL = readReg(REG04_S_LIGL);          // 雷のエネルギーを読み込む
		u8LIGM = readReg(REG05_S_LIGM);          // 雷のエネルギーを読み込む
		u8LIGMM = readReg(REG06_S_LIGMM);        // 雷のエネルギーを読み込む
	}

	dbgprintf("validateSignal: u8IntSrc:%02X u8Dist:%02X u8LIGL:%02X u8LIGM:%02X u8LIGMM:%02X\n", u8IntSrc, u8Dist, u8LIGL, u8LIGM, u8LIGMM);

	// INTNOISE_LIGHTNINGINTR: 雷検出
	// INTNOISE_DISTERBERDETECT: ディスターバ（誤検出）
	// INTNOISE_TOHIGH: ノイズレベル過大
	if (u8IntSrc & INTNOISE_LIGHTNINGINTR) {

		lEnergy = ((((unsigned long)u8LIGMM & 0x0F) * 65536) + ((unsigned long)u8LIGM * 256) + ((unsigned long)u8LIGL)) & 0x0FFFFF;

		// 最初の１回は無視する
		// if (isFirstCall) {
		// 	isFirstCall = false;        // 初回呼び出しフラグをクリア
		// 	return AS3935_SIGNAL::NONE; // 初回呼び出しは無視する
		// }

		// 距離が有効範囲内かつ、ノイズ/誤検出でなければtrue
		if (u8Dist > 0 && u8Dist < 0x3F) {
			dbgprintf("validateSignal: Thunder detected! Dist:%02X Energy:%ld\n", u8Dist, lEnergy);
			m_bufAlarmSummary.push(SUMM_THUNDER);
			m_bufAlarmDist.push(u8Dist);                // 距離をリングバッファに保存
			m_bufSingleEnergy.push(lEnergy);            // ステータスをリングバッファに保存
			m_bufDateTime.push(time(NULL));             // 現在時刻をリングバッファに保存
			m_latestSignalValid = AS3935_SIGNAL::VALID; // 雷が検出された場合
		} else {
			if (u8Dist >= 0x3F) {
				dbgprintf("validateSignal: Too far detected! Dist:%02X Energy:%ld\n", u8Dist, lEnergy);
				m_bufAlarmSummary.push(SUMM_TOOFAR);
			} else {
				dbgprintf("validateSignal: Invalid signal detected! Dist:%02X Energy:%ld\n", u8Dist, lEnergy);
				m_bufAlarmSummary.push(SUMM_NUMZERO);
			}
			m_bufAlarmDist.push(u8Dist);                  // 距離をリングバッファに保存
			m_bufSingleEnergy.push(lEnergy);              // ステータスをリングバッファに保存
			m_bufDateTime.push(time(NULL));               // 現在時刻をリングバッファに保存
			m_latestSignalValid = AS3935_SIGNAL::INVALID; // 距離が無効、またはノイズ/誤検出の場合
		}
	} else if (u8IntSrc & INTNOISE_DISTERBERDETECT) {
		dbgprintf("validateSignal: Disturber detected!\n");
		m_bufAlarmSummary.push(SUMM_DISTERBER);       // ディスターバ（誤検出）をリングバッファに保存;
		m_bufAlarmDist.push(0);                       // 距離をリングバッファに保存
		m_bufSingleEnergy.push(0);                    // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));               // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::INVALID; // 距離が無効、またはノイズ/誤検出の場合

	} else if (u8IntSrc & INTNOISE_TOHIGH) {
		dbgprintf("validateSignal: Noise level too high detected!\n");
		m_bufAlarmSummary.push(SUMM_NOISEHIGH);       // ノイズレベル過大をリングバッファに保存
		m_bufAlarmDist.push(0);                       // 距離をリングバッファに保存
		m_bufSingleEnergy.push(0);                    // ステータスをリングバッファに保存
		m_bufDateTime.push(time(NULL));               // 現在時刻をリングバッファに保存
		m_latestSignalValid = AS3935_SIGNAL::INVALID; // 距離が無効、またはノイズ/誤検出の場合
	} else if (u8IntSrc == INTNOISE_CLEARSTATSTICS) { // 統計情報が削除されたことを示すので、雷の検出ではない
		dbgprintf("validateSignal: Clear statistics detected!\n");
		m_latestSignalValid = AS3935_SIGNAL::STATCLEAR; // 信号が存在しない。記録する必要もないので、ここでは何もせず戻る
		return m_latestSignalValid;
	} else {
		dbgprintf("validateSignal: No valid signal detected! u8IntSrc:%02X\n", u8IntSrc);
		m_latestSignalValid = AS3935_SIGNAL::NONE; // 信号が存在しない
		return m_latestSignalValid; // 最新の信号が有効かどうかを返す
	}
	

	// 最新の情報を取得しておく（信号が存在しない - リングバッファに格納されていない場合でも）
	m_latestSignalValid = m_latestSignalValid; // 最新の信号が有効かどうかを保存
	m_latestBufAlarmSummary = m_bufAlarmSummary.getLastData();
	m_latestBufAlarmDist = m_bufAlarmDist.getLastData();
	m_latestBufSingleEnergy = m_bufSingleEnergy.getLastData();
	m_latestBufDateTime = m_bufDateTime.getLastData(); // 最新の日時を保存
	return m_latestSignalValid;                        // 最新の信号が有効かどうかを返す
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

void AS3935::readBlockReg(uint8_t* a_regVal)
{
	uint8_t* regVal = a_regVal;
	uint8_t reg = REG00_AFEGB_PWD;
	i2c_write_blocking((m_u8I2cPort == 0 ? i2c0 : i2c1), m_u8I2CAddress, &reg, 1, true);
	int iReadCnt;
	iReadCnt = i2c_read_blocking((m_u8I2cPort == 0 ? i2c0 : i2c1), m_u8I2CAddress, regVal, 9, false); // REG00_AFEGB_PWD
	dbgprintf("AS3935 readBlockReg: %d bytes read, ",iReadCnt);
	for (int i = 0; i < 9; i++) {
		dbgprintf("%02X-\n", a_regVal[i]);
	}
	dbgprintf("\n");
}

/**
 * @brief 指定インデックスの最新イベント情報を取得
 * @details
 * リングバッファからidx番目（新しい順）のイベント情報（サマリ・距離・エネルギー・時刻）を取得します。
 * サマリ値が0の場合は無効とみなしてfalseを返します。
 *
 * @param idx 新しい順のインデックス（0が最新）
 * @param[out] a_u8AlarmSummary イベントサマリ格納先
 * @param[out] a_u8AlarmDist 距離格納先
 * @param[out] a_lEnergy エネルギー格納先
 * @param[out] a_time 時刻格納先
 * @retval true 有効なイベントが取得できた
 * @retval false 無効（サマリ値0）
 */
bool AS3935::GetLatestEvent(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time)
{
	a_u8AlarmSummary = m_bufAlarmSummary.getFromLast(idx); ///< サマリ値
	a_u8AlarmDist = m_bufAlarmDist.getFromLast(idx); ///< 距離
	a_lEnergy = m_bufSingleEnergy.getFromLast(idx); ///< エネルギー
	a_time = m_bufDateTime.getFromLast(idx); ///< 時刻
	if (a_u8AlarmSummary == 0) return false;
	return true;
}
/**
 * @brief 指定インデックスの最新「雷」イベント情報を取得
 * @details
 * リングバッファからidx番目（新しい順）の「雷」イベント（SUMM_THUNDERのみ）を検索し、情報を取得します。
 *
 * @param idx 新しい順のインデックス（0が最新）
 * @param[out] a_u8AlarmSummary イベントサマリ格納先
 * @param[out] a_u8AlarmDist 距離格納先
 * @param[out] a_lEnergy エネルギー格納先
 * @param[out] a_time 時刻格納先
 * @retval true 有効な「雷」イベントが取得できた
 * @retval false 見つからなかった
 */
bool AS3935::GetLatestAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time)
{
	int found = 0; ///< 見つかった件数
	int total = m_bufAlarmSummary.getCount(); // count()メソッドで要素数取得
	for (int i = 0; i < total; ++i) {
		int summary = m_bufAlarmSummary.getFromLast(i); ///< サマリ値
		if (summary == SUMM_THUNDER) {
			if (found == idx) {
				a_u8AlarmSummary = summary;
				a_u8AlarmDist = m_bufAlarmDist.getFromLast(i);
				a_lEnergy = m_bufSingleEnergy.getFromLast(i);
				a_time = m_bufDateTime.getFromLast(i);
				return true;
			}
			found++;
		}
	}
	return false;
}
/**
 * @brief 指定インデックスの最新「誤検出」イベント情報を取得
 * @details
 * リングバッファからidx番目（新しい順）の「誤検出」イベント（SUMM_THUNDER以外）を検索し、情報を取得します。
 *
 * @param idx 新しい順のインデックス（0が最新）
 * @param[out] a_u8AlarmSummary イベントサマリ格納先
 * @param[out] a_u8AlarmDist 距離格納先
 * @param[out] a_lEnergy エネルギー格納先
 * @param[out] a_time 時刻格納先
 * @retval true 有効な「誤検出」イベントが取得できた
 * @retval false 見つからなかった
 */
bool AS3935::GetLatestFalseAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time)
{
	int found = 0; ///< 見つかった件数
	int total = m_bufAlarmSummary.getCount(); // count()メソッドで要素数取得
	for (int i = 0; i < total; ++i) {
		int summary = m_bufAlarmSummary.getFromLast(i); ///< サマリ値
		if (summary != SUMM_THUNDER) {
			if (found == idx) {
				a_u8AlarmSummary = summary;
				a_u8AlarmDist = m_bufAlarmDist.getFromLast(i);
				a_lEnergy = m_bufSingleEnergy.getFromLast(i);
				a_time = m_bufDateTime.getFromLast(i);
				return true;
			}
			found++;
		}
	}
	return false;
}

void AS3935::Reset()
{
	// PresetDefault();
	writeRegAndData_1(REG00_AFEGB_PWD, (settings.value.gainBoost << 1));
	writeRegAndData_1(REG01_NFLEV_WDTH, (settings.value.noiseFloor << 4) | settings.value.watchDogThreshold);                      // ノイズレベルとウォッチドッグスレッショルドを設定
	writeRegAndData_1(REG02_CLSTAT_MINNUMLIGH_SREJ, 0b00000000 | (settings.value.minimumEvent << 4) | settings.value.spikeReject); // 最小イベント数とスパイクリジェクトを設定
	writeRegAndData_1(REG03_LCOFDIV_MDIST_INT, FDIV_RATIO_1_16 | MASK_DISTURBER_FALSE);                                            // LCO Frequency Division Ratio = 1/16, Mask Disturber = 0, Interrupt = 0
	writeRegAndData_1(REG02_CLSTAT_MINNUMLIGH_SREJ, 0b01000000 | (settings.value.minimumEvent << 4) | settings.value.spikeReject); // 内部データのクリア。ビット６をストローブする
	writeRegAndData_1(REG02_CLSTAT_MINNUMLIGH_SREJ, 0b00000000 | (settings.value.minimumEvent << 4) | settings.value.spikeReject); //
	writeRegAndData_1(REG02_CLSTAT_MINNUMLIGH_SREJ, 0b01000000 | (settings.value.minimumEvent << 4) | settings.value.spikeReject); //
	writeRegAndData_1(REG08_LCO_SRCO_TRCO_CAP, (DISPLCO_OFF | m_u8calibratedCap));                                                 // キャリブレーションされたキャパシタの値を設定する
}
