#pragma once
#include <stdint.h>
#include <cstddef>

/**
 * @brief I2Cデバイス用の基底抽象クラス
 * @details
 * I2CBaseはI2C通信を行うデバイス向けの基底クラスです。
 * I2Cポート番号、SDA/SCLピン番号、I2Cアドレスなどの共通メンバを持ち、
 * I2C初期化やレジスタ書き込みなどの基本的な操作を提供します。
 * 派生クラスでデバイス固有の処理を追加・拡張できます。
 */
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
	int writeWord(uint16_t cmddata);
};
