#pragma once
#include <stdint.h>
#include <queue>
#include "RingBuffer.h"
#include "I2CBase.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
// Forward declaration to avoid include errors if only pointer is used

using namespace ardPort;
    using namespace ardPort::spi;

	

	class AS3935 : public I2CBase
	{
	  private:
		Adafruit_ILI9341* m_pTft;

		uint8_t m_u8IrqPin;
		uint8_t m_u8calibratedCap;
		uint16_t m_timeCalibration;
		uint32_t m_FreqCalibration;

	  public:
		const uint8_t SUMM_NONE = 0x00; // アラームサマリー
		const uint8_t SUMM_THUNDER = 0x01; // アラームサマリー
		const uint8_t SUMM_TOOFAR = 0x02; // 遠すぎる
		const uint8_t SUMM_NUMZERO = 0x03; // 数がゼロ
		const uint8_t SUMM_DISTERBER = 0x4; // ディスターバ（誤検出）
		const uint8_t SUMM_NOISEHIGH = 0x5; // ノイズレベル課題
		const char* SUMM_STRINGS[6] = {
			"NONE", "雷の検出", "距離超過", "信号なし", "認識誤り", "雑音過大"
		};

		RingBuffer m_bufAlarmSummary;
		RingBuffer m_bufAlarmDist;
		RingBuffer m_bufAlarmStatus;

		RingBuffer m_bufFalseAlarmSummary;
		RingBuffer m_bufFalseAlarmDist;
		RingBuffer m_bufFalseAlarmStatus;

	  public:
		AS3935(Adafruit_ILI9341* a_pTft);
		bool Init(uint8_t a_u8IU2cAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin);
		// キャリブレーション実行
		uint32_t Calibrate();
		// その他、AS3935の操作メソッドをここに追加
		void PresetDefault();
		void StartCalibration(uint16_t a_timeCalibration = 1000); // デフォルトで1秒間キャリブレーションを行う

		bool validateSignal();
		/**
		 * @brief 指定レジスタから1バイト読み出す
		 * @details
		 * AS3935のI2Cレジスタから1バイト値を読み出します。
		 *
		 * @param reg レジスタアドレス
		 * @return 読み出した値
		 */
		uint8_t readReg(uint8_t reg);

		bool GetLatestAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, uint8_t& a_u8AlarmStatus);
		bool GetLatestFalseAlarm(uint8_t idx, uint8_t& a_u8AlarmSummary, uint8_t& a_u8AlarmDist, uint8_t& a_u8AlarmStatus);
		const char* GetAlarmSummaryString(uint8_t a_u8AlarmSummary)
		{
			if (a_u8AlarmSummary < 6) {
				return SUMM_STRINGS[a_u8AlarmSummary];
			}
			return "UNKNOWN";
		}
	};
