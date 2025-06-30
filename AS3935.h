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
#define AFE_GB_MIN 0b00000     ///< AFEゲインブースト最小値（0）
#define AFE_GB_OUTDOOR 0b01110 ///< AFEゲインブースト屋外用（14）
#define AFE_GB_INDOOR 0b10010  ///< AFEゲインブースト屋内用（18）
#define AFE_GB_MAX 0b11111     ///< AFEゲインブースト最大値（31）

// Noise Floor Level
#define NFLEV_0 0x00 ///< ノイズフロアレベル最小値（0）
#define NFLEV_1 0x01 ///< ノイズフロアレベル1
#define NFLEV_2 0x02 ///< ノイズフロアレベル2（デフォルト）
#define NFLEV_3 0x03 ///< ノイズフロアレベル3
#define NFLEV_4 0x04 ///< ノイズフロアレベル4
#define NFLEV_5 0x05 ///< ノイズフロアレベル5
#define NFLEV_6 0x06 ///< ノイズフロアレベル6
#define NFLEV_7 0x07 ///< ノイズフロアレベル最大値（7）
#define NFLEV_DEF NFLEV_2 ///< デフォルトのノイズレベル

// Watchdog Threshold  (0～0x0F)
#define WDTH_MAX 0x0F      ///< ウォッチドッグスレッショルド最大値
#define WDTH_DEFAULT 0x02  ///< デフォルトのウォッチドッグスレッショルド
#define WDTH_MIN 0x00      ///< ウォッチドッグスレッショルド最小値

// REG_02
#define SREJ_MIN 0x00         ///< スパイクリジェクション最小値
#define SREJ_DEFAULT 0x02     ///< デフォルトのスパイクリジェクション
#define SREJ_MAX 0x0B         ///< スパイクリジェクション最大値

#define NUMLIGHT_MIN 0x00         ///< 最小雷数最小値
#define NUMLIGHT_DEFAULT 0x00     ///< デフォルトの最小雷数
#define NUMLIGHT_MAX 0x03         ///< 最小雷数最大値


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
		"なし　", "　雷　", "距離超", "距離０", "誤信号", "雑音多"};

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

    // --- キャリブレーション値のpublic getter ---
    uint8_t getCalibratedCap() const { return m_u8calibratedCap; }
    uint16_t getTimeCalibration() const { return m_timeCalibration; }
    uint32_t getFreqCalibration() const { return m_FreqCalibration; }

  public:
	AS3935(Adafruit_ILI9341* a_pTft);
	bool Init(uint8_t a_u8IU2cAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin);
	// キャリブレーション実行
	uint32_t Calibrate();
	// その他、AS3935の操作メソッドをここに追加
	bool PresetDefault();
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
	void readBlockReg(uint8_t* pReg);
	

	uint8_t readReg(uint8_t reg);
	void Reset();
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
	bool GetLatestEvent(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
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
	bool GetLatestAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
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
	bool GetLatestFalseAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, long& a_lEnergy, time_t& a_time);
	const char* GetAlarmSummaryString(uint8_t a_u8AlarmSummary)
	{
		if (a_u8AlarmSummary < 6) {
			return SUMM_STRINGS[a_u8AlarmSummary];
		}
		return "UNKNOWN";
	}
	};
