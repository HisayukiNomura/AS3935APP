#include "I2CBase.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

/**
 * @brief I2CBaseクラスのデフォルトコンストラクタ
 * @details
 * I2Cポート番号、SDA/SCLピン番号を0で初期化します。
 * 派生クラスでI2Cデバイスの初期化前に呼び出されます。
 */
I2CBase::I2CBase() :
	m_u8I2cPort(0),
	m_u8SdaPin(0),
	m_u8SclPin(0)
{

}

/**
 * @brief I2C通信の初期化を行う
 * @details
 * 指定したI2Cアドレス、I2Cポート、SDA/SCLピンでI2C通信を初期化します。
 * ポート番号やピン番号が不正な場合はfalseを返します。
 * 初期化成功時はtrueを返します。
 *
 * @param a_u8I2CAddress I2Cデバイスアドレス
 * @param a_u8I2cPort    I2Cポート番号
 * @param a_u8SdaPin     SDAピン番号
 * @param a_u8SclPin     SCLピン番号
 * @return 初期化が成功した場合はtrue、失敗した場合はfalse
 */
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

/**
 * @brief 16ビットコマンド/データをI2Cバスに送信
 * @details
 * 16ビットのコマンドやデータをI2Cバスに送信します。
 * 上位バイト・下位バイトに分割して送信します。
 *
 * @param cmddata 送信する16ビット値
 * @return 書き込み結果
 */
int I2CBase::writeWord(uint16_t cmddata)
{
	uint8_t data[2];
	data[0] = (cmddata >> 8) & 0xFF; // 上位バイト
	data[1] = cmddata & 0xFF;       // 下位バイト
	int iRet = writeBlocking(data, sizeof(data), false);
	return iRet;
}

/**
 * @brief I2Cバスにデータを書き込む
 * @details
 * 指定したデータをI2Cバスに送信します。
 * ストップコンディションを送信しない場合はnostop=trueを指定します。
 *
 * @param src 送信データのポインタ
 * @param len 送信データ長
 * @param nostop ストップコンディションを送信しない場合true
 * @return 書き込んだバイト数
 */
int I2CBase::writeBlocking(const uint8_t* src, size_t len, bool nostop)
{
    int iRet = i2c_write_blocking((m_u8I2cPort == 0) ? i2c0 : i2c1, m_u8I2CAddress, src, len, nostop);
	return iRet;
}

/**
 * @brief 指定レジスタに1バイト書き込む
 * @details
 * 指定レジスタアドレスに1バイトのデータを書き込みます。
 * レジスタアドレスとデータを2バイト配列で送信します。
 *
 * @param reg レジスタアドレス
 * @param dataByte 書き込むデータ
 * @return 書き込み結果
 */
int I2CBase::writeRegAndData_1(uint8_t reg, uint8_t dataByte)
{

	uint8_t data[2] = { reg, dataByte };
    int iRet = writeBlocking(data, sizeof(data), false);
	return iRet;
}