/**
 * @file I2CBase.h
 * @brief I2Cデバイス用基底抽象クラス定義
 * @details
 * - I2C通信を行うデバイス向けの共通基底クラス。
 * - I2Cポート番号、SDA/SCLピン番号、I2Cアドレスなどの共通メンバを持つ。
 * - I2C初期化やレジスタ書き込みなどの基本操作を提供し、派生クラスで拡張可能。
 */
#pragma once
#include <stdint.h>
#include <cstddef>

/**
 * @brief I2Cデバイス用の基底抽象クラス
 * @details
 * I2C通信を行うデバイスの共通機能（初期化・レジスタ書き込み等）を提供。
 * 派生クラスでデバイス固有の処理を追加・拡張できる。
 */
class I2CBase {
protected:
    uint8_t m_u8I2cPort;    ///< 使用するI2Cポート番号
    uint8_t m_u8SdaPin;     ///< SDAピン番号
    uint8_t m_u8SclPin;     ///< SCLピン番号
	uint8_t m_u8I2CAddress;  ///< I2Cアドレス

  public :
	  /**
	   * @brief コンストラクタ
	   * @details メンバ変数を初期化
	   * @retval なし
	   */
	  I2CBase();
	  /**
	   * @brief 仮想デストラクタ
	   * @details 派生クラスでのクリーンアップ用
	   * @retval なし
	   */
	  virtual ~I2CBase() {}
    /**
     * @brief I2C初期化
     * @details
     * 指定アドレス・ポート・SDA/SCLピンでI2C通信を初期化。
     * @param a_u8I2CAddress I2Cアドレス
     * @param a_u8I2cPort    I2Cポート番号
     * @param a_u8SdaPin     SDAピン番号
     * @param a_u8SclPin     SCLピン番号
     * @retval true 初期化成功
     * @retval false 初期化失敗
     */
	  virtual bool InitI2C(uint8_t a_u8I2CAddress, uint8_t a_u8I2cPort, uint8_t a_u8SdaPin, uint8_t a_u8SclPin);
    /**
     * @brief I2Cバスにデータを書き込む
     * @details
     * 指定したデータをI2Cバスに送信する。nostop=trueでストップコンディション無し。
     * @param src 送信データバッファ
     * @param len 送信バイト数
     * @param nostop ストップコンディション無しならtrue
     * @retval 書き込んだバイト数（負値はエラー）
     */
	  int writeBlocking(const uint8_t* src, size_t len, bool nostop);
    /**
     * @brief 指定レジスタに1バイト書き込む
     * @details
     * 指定レジスタアドレスに1バイトデータを書き込む。
     * @param reg レジスタアドレス
     * @param dataByte 書き込むデータ
     * @retval 書き込んだバイト数（負値はエラー）
     */
	  int writeRegAndData_1(uint8_t reg, uint8_t dataByte);
    /**
     * @brief 16ビットコマンド/データをI2Cバスに書き込む
     * @details
     * 16ビット値をI2Cバスに送信する（主にダイレクトコマンド用）。
     * @param cmddata 送信する16ビット値
     * @retval 書き込んだバイト数（負値はエラー）
     */
	  int writeWord(uint16_t cmddata);
};
