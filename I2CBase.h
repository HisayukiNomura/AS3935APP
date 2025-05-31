#pragma once
#include <stdint.h>
#include <cstddef>

class I2CBase {
protected:
    uint8_t m_u8I2cPort;
    uint8_t m_u8SdaPin;
    uint8_t m_u8SclPin;
	uint8_t m_u8I2CAddress;

  public :
	  I2CBase();
	virtual ~I2CBase() {}
    // I2C初期化
	virtual bool InitI2C(uint8_t a_u8I2CAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin);
	int writeBlocking(const uint8_t* src, size_t len, bool nostop);
	int writeRegAndData_1(uint8_t reg, uint8_t dataByte);
};
