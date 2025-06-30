/**
 * @file I2CBase.cpp
 * @brief I2Cデバイス用基底クラスの実装
 * @details
 * - I2C通信の初期化、レジスタ書き込み、バス書き込み等の基本操作を提供。
 * - 派生クラスでI2Cデバイス制御を拡張可能。
 */
#include "I2CBase.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

/**
 * @brief I2CBaseクラスのデフォルトコンストラクタ
 * @details
 * I2Cポート番号、SDA/SCLピン番号を0で初期化します。
 * 派生クラスでI2Cデバイスの初期化前に呼び出されます。
 * @retval なし
 */
I2CBase::I2CBase() :
	m_u8I2cPort(0), ///< I2Cポート番号初期化
	m_u8SdaPin(0),  ///< SDAピン初期化
	m_u8SclPin(0)   ///< SCLピン初期化
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
 * @retval true  初期化成功
 * @retval false 初期化失敗
 */
bool I2CBase::InitI2C(uint8_t a_u8I2CAddress , uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin)
{
	m_u8I2cPort = a_u8I2cPort; ///< I2Cポート番号保存
	m_u8SdaPin = a_u8SdaPin;   ///< SDAピン保存
	m_u8SclPin = a_u8SclPin;   ///< SCLピン保存
	m_u8I2CAddress = a_u8I2CAddress; ///< I2Cアドレス保存
	if (m_u8I2cPort != 0 && m_u8I2cPort != 1) {
		return false; // 無効なポート番号
	}
	if (m_u8SdaPin > 29 || m_u8SclPin > 29) {
		return false; // 無効なピン番号
	}
	i2c_init((m_u8I2cPort == 0) ? i2c0 : i2c1, 400 * 1000); ///< 400kHzでI2C初期化
	gpio_set_function(m_u8SdaPin, GPIO_FUNC_I2C); ///< SDAピンをI2C機能に設定
	gpio_set_function(m_u8SclPin, GPIO_FUNC_I2C); ///< SCLピンをI2C機能に設定
	gpio_pull_up(m_u8SdaPin); ///< SDAピンをプルアップ
	gpio_pull_up(m_u8SclPin); ///< SCLピンをプルアップ
	return true;
}

/**
 * @brief 16ビットコマンド/データをI2Cバスに送信
 * @details
 * 16ビットのコマンドやデータをI2Cバスに送信します。
 * 上位バイト・下位バイトに分割して送信します。
 *
 * @param cmddata 送信する16ビット値
 * @retval int 書き込んだバイト数（負値はエラー）
 */
int I2CBase::writeWord(uint16_t cmddata)
{
	uint8_t data[2];
	data[0] = (cmddata >> 8) & 0xFF; ///< 上位バイト
	data[1] = cmddata & 0xFF;        ///< 下位バイト
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
 * @retval int 書き込んだバイト数（負値はエラー）
 */
int I2CBase::writeBlocking(const uint8_t* src, size_t len, bool nostop)
{
    int iRet = i2c_write_blocking((m_u8I2cPort == 0) ? i2c0 : i2c1, m_u8I2CAddress, src, len, nostop); ///< I2Cバス書き込み
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
 * @retval int 書き込んだバイト数（負値はエラー）
 */
int I2CBase::writeRegAndData_1(uint8_t reg, uint8_t dataByte)
{
	uint8_t data[2] = { reg, dataByte }; ///< レジスタ＋データ配列
    int iRet = writeBlocking(data, sizeof(data), false);
	return iRet;
}