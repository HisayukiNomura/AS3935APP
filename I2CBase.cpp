#include "I2CBase.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

I2CBase::I2CBase() :
	m_u8I2cPort(0),
	m_u8SdaPin(0),
	m_u8SclPin(0)
{

}

bool I2CBase::InitI2C(uint8_t a_u8I2CAddress , uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin)
{
	m_u8I2cPort = a_u8I2cPort;
	m_u8SdaPin = a_u8SdaPin;
	m_u8SclPin = a_u8SclPin;
	m_u8I2CAddress = a_u8I2CAddress; // AS3935のI2Cアドレスを設定
	if (m_u8I2cPort != 0 && m_u8I2cPort != 1) {
		return false; // 無効なポート番号
	}
	if (m_u8SdaPin > 29 || m_u8SclPin > 29) {
		return false; // 無効なピン番号
	}
	i2c_init((m_u8I2cPort == 0) ? i2c0 : i2c1, 400 * 1000);
	gpio_set_function(m_u8SdaPin, GPIO_FUNC_I2C);
	gpio_set_function(m_u8SclPin, GPIO_FUNC_I2C);
	gpio_pull_up(m_u8SdaPin);
	gpio_pull_up(m_u8SclPin);



	return true;
}

int I2CBase::writeBlocking(const uint8_t* src, size_t len, bool nostop)
{
    int iRet = i2c_write_blocking((m_u8I2cPort == 0) ? i2c0 : i2c1, m_u8I2CAddress, src, len, nostop);
	return iRet;
}

int I2CBase::writeRegAndData_1(uint8_t reg, uint8_t dataByte)
{
	uint8_t data[2] = { reg, dataByte };
    int iRet = writeBlocking(data, sizeof(data), false);
	return iRet;
}