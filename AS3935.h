#pragma once
#include <stdint.h>
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

	  public:
		AS3935(Adafruit_ILI9341* a_pTft);
		bool Init(uint8_t a_u8IU2cAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin);
		// キャリブレーション実行
		void Calibrate();
		// その他、AS3935の操作メソッドをここに追加
		void PresetDefault();
		void StartCalibration();
	};
