#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "AS3935.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

using namespace ardPort;
using namespace ardPort::spi;
#define AFE_GB 0x1F // ANALOG FRONT END GAIN BOOST = 12(Indoor) 0x00 to 0x1F
#define NF_LEV 0x02 // NOIS FLOOR LEVEL 0x00 to 0x07
#define WDTH 0x02   // WATCH DOG THRESHOLD 0x00 to 0x0F
AS3935::AS3935(Adafruit_ILI9341* a_pTft) : m_pTft(a_pTft)
{
    // 必要ならここで初期化
}
bool AS3935::Init(uint8_t a_u8I2CAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin, uint8_t a_u8IrqPin)
{
	m_u8IrqPin = a_u8IrqPin;


	bool bRet = InitI2C(a_u8I2CAddress,a_u8I2cPort, a_u8SdaPin, a_u8SclPin);

	// IRQピンの初期化
	gpio_init(m_u8IrqPin);
	gpio_set_dir(m_u8IrqPin, GPIO_IN);

	//AS3935の初期化処理
	writeRegAndData_1(0x3C, 0x96); // Reset and set default values
	writeRegAndData_1(0x3D, 0x96); // Reset and set default values
	writeRegAndData_1(0x00, (AFE_GB << 1)); // SET ANALOG FRONT END GAIN BOOST
	writeRegAndData_1(0x01, ((NF_LEV << 4) | WDTH));
	writeRegAndData_1(0x03, 0x00); // FRQ DIV RATIO = 1/16

	return bRet; // 初期化成功
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
