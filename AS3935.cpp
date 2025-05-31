#include "AS3935.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

using namespace ardPort;

AS3935::AS3935(Adafruit_ILI9341* a_pTft): m_pTft(a_pTft)
{
    // I2C初期化

    // IRQピン初期化
    gpio_init(m_u8IrqPin);
    gpio_set_dir(m_u8IrqPin, GPIO_IN);
}

/// @brief I2Cの初期化関数
/// @param PortNo	I2Cのポート番号（0または1）
/// @param a_SDA	I2CのSDAピン番号（0〜29の範囲）
/// @param a_SCL	I2CのSCLピン番号（0〜29の範囲）
/// @return 	初期化が成功した場合はtrue、失敗した場合はfalseを返す
bool AS3935::InitI2C(uint8_t PortNo, uint8_t a_SDA, uint8_t a_SCL, uint8_t a_IRQ)
{
	// I2Cのポート番号を設定
	m_u8I2cPort =  PortNo;
	m_u8SdaPin = a_SDA;
	m_u8SclPin = a_SCL;
	m_u8IrqPin = a_IRQ;
    
	if (PortNo != 0 && PortNo != 1) {
		return false; // 無効なポート番号
	}

	// I2CのSDAとSCLピンを設定
	if (m_u8SdaPin < 0 || m_u8SdaPin > 29 || m_u8SclPin < 0 || m_u8IrqPin > 29) {
		return false; // 無効なピン番号
	}

	// I2Cの初期化

	i2c_init((PortNo == 0) ? i2c0 : i2c1, 400 * 1000); // 400KHzで初期化

	gpio_set_function(m_u8SdaPin, GPIO_FUNC_I2C);
	gpio_set_function(m_u8SclPin, GPIO_FUNC_I2C);
	gpio_pull_up(m_u8SdaPin);
	gpio_pull_up(m_u8SclPin);
    // IRQピンの初期化
	gpio_init(m_u8IrqPin);
	gpio_set_dir(m_u8IrqPin, GPIO_IN);


	return true;
}

void AS3935::calibrate() {
    if (m_pTft) {
        m_pTft->setCursor(0, 100);
        m_pTft->printf("AS3935キャリブレーション中...\n");
    }
    // ここにキャリブレーション処理を実装（例: I2Cでレジスタ書き込み等）
    // ...
    if (m_pTft) {
        m_pTft->printf("キャリブレーション完了\n");
    }
}
