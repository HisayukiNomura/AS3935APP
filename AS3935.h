#pragma once
#include <stdint.h>
#include <queue>
#include <ctime>
#include "RingBuffer.h"
#include "I2CBase.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
// Forward declaration to avoid include errors if only pointer is used

using namespace ardPort;
using namespace ardPort::spi;

//  Analog Front End Gain Boost
#define AFE_GB_MIN 0b00000     // ANALOG FRONT END GAIN BOOST = 0 (min)
#define AFE_GB_OUTDOOR 0b01110 // ANALOG FRONT END GAIN BOOST = 14 (outdoor)
#define AFE_GB_INDOOR 0b10010  // ANALOG FRONT END GAIN BOOST = 18 (indoor)
#define AFE_GB_MAX 0b11111      // ANALOG FRONT END GAIN BOOST = 31 (max)

// Noise Floor Level
#define NFLEV_0 0x00 // NOISE FLOOR LEVEL = 0 (min)
#define NFLEV_1 0x01  // NOISE FLOOR LEVEL = 1
#define NFLEV_2 0x02  // NOISE FLOOR LEVEL = 2(default)
#define NFLEV_3 0x03  // NOISE FLOOR LEVEL = 3
#define NFLEV_4 0x04  // NOISE FLOOR LEVEL = 4
#define NFLEV_5 0x05  // NOISE FLOOR LEVEL = 5
#define NFLEV_6 0x06  // NOISE FLOOR LEVEL = 6
#define NFLEV_7 0x07  // NOISE FLOOR LEVEL = 7 (max)
#define NFLEV_DEF NFLEV_2 // デフォルトのノイズレベルを設定

// Watchdog Threshold  (0～0x0F)
#define WDTH_DEFAULT 0x02   // デフォルトのウォッチドッグスレッショルドを設定

/*
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
*/

enum AS3935_SIGNAL {
	NONE = 0,    // 信号が無効(信号なし)
	VALID = 1,   // 信号が有効（雷の検出）
	INVALID = 2, // 信号が無効（ノイズや誤検出など）
	STATCLEAR = 3, // ステータスクリア（距離の古いイベントがパージされたため、距離推定が変更された）
};

class AS3935 : public I2CBase
{
  private:
	Adafruit_ILI9341* m_pTft;

	uint8_t m_u8IrqPin;
	uint8_t m_u8calibratedCap;
	uint16_t m_timeCalibration;
	uint32_t m_FreqCalibration;

	AS3935_SIGNAL m_latestSignalValid = AS3935_SIGNAL::NONE; // 最新の信号が有効かどうか
	uint8_t m_latestBufAlarmSummary = 0;
	int m_latestBufAlarmDist = 0;
	int m_latestBufSingleEnergy = 0;
	time_t m_latestBufDateTime = 0; // 最新の日時

  public:
	const uint8_t SUMM_NONE = 0x00;     // アラームサマリー
	const uint8_t SUMM_THUNDER = 0x01;  // アラームサマリー
	const uint8_t SUMM_TOOFAR = 0x02;   // 遠すぎる
	const uint8_t SUMM_NUMZERO = 0x03;  // 数がゼロ
	const uint8_t SUMM_DISTERBER = 0x4; // ディスターバ（誤検出）
	const uint8_t SUMM_NOISEHIGH = 0x5; // ノイズレベル課題
	const char* SUMM_STRINGS[6] = {
		"なし　", "　雷　", "距離超", "信号無", "誤信号", "雑音多"};

	RingBufferT<uint8_t> m_bufAlarmSummary;
	RingBufferT<uint8_t> m_bufAlarmDist;
	RingBufferT<long> m_bufSingleEnergy;
	RingBufferT<long long> m_bufDateTime;

	AS3935_SIGNAL getLatestSignalValid() const { return m_latestSignalValid; }
	int getLatestSummary() const { return m_latestBufAlarmSummary; }
	int getLatestDist() const { return m_latestBufAlarmDist; }
	int getLatestEnergy() const { return m_latestBufSingleEnergy; }
	time_t getLatestDateTime() const { return m_latestBufDateTime; }
	const char* getLatestSummaryStr() { return GetAlarmSummaryString(m_latestBufAlarmSummary); }

  public:
	AS3935(Adafruit_ILI9341* a_pTft);
	bool Init(uint8_t a_u8IU2cAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin);
	// キャリブレーション実行
	uint32_t Calibrate();
	// その他、AS3935の操作メソッドをここに追加
	void PresetDefault();
	void StartCalibration(uint16_t a_timeCalibration = 1000); // デフォルトで1秒間キャリブレーションを行う

	AS3935_SIGNAL validateSignal();
	/**
	 * @brief 指定レジスタから1バイト読み出す
	 * @details
	 * AS3935のI2Cレジスタから1バイト値を読み出します。
	 *
	 * @param reg レジスタアドレス
	 * @return 読み出した値
	 */
	uint8_t readReg(uint8_t reg);
	void Reset();
	bool GetLatestEvent(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
	bool GetLatestAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
	bool GetLatestFalseAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
	const char* GetAlarmSummaryString(uint8_t a_u8AlarmSummary)
	{
		if (a_u8AlarmSummary < 6) {
			return SUMM_STRINGS[a_u8AlarmSummary];
		}
		return "UNKNOWN";
	}
	};
