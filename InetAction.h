#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "settings.h"
#include "lwip/ip_addr.h"
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/cyw43_arch.h"

/**
 * @file InetAction.h
 * @brief Wi-Fi・SNTP等のネットワーク制御を行うクラス定義
 * @details Raspberry Pi Pico WでWi-Fi接続やNTP時刻同期、DNS解決などを行うための機能を提供します。
 */

using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief ネットワーク関連のアクションをまとめたクラス
 * @details Wi-Fi初期化、接続、DNS解決、SNTP時刻取得などを担当します。
 */
class InetAction
{
  public:
	/**
	 * @brief TFTディスプレイへのポインタ
	 * @details 進捗やエラー表示などに利用
	 */
	Adafruit_ILI9341* pTft;
	/**
	 * @brief 設定情報への参照
	 * @details SSIDやパスワード、ロケール等の設定を保持
	 */
	Settings& settings;

  public:
	/**
	 * @brief DNS名前解決の完了フラグ
	 * @details 1:成功 2:失敗 0:未完了
	 */
	uint8_t isDNSFinished;
	/**
	 * @brief SNTPサーバーのIPv4アドレス
	 * @details DNS解決後に格納される
	 */
	ip4_addr_t addrSNTP;

	int lastErr_Connect; ///< 最後の接続エラーコード
	int lastErr_Sntp;    ///< 最後のSNTPエラーコード

  public:
	/**
	 * @brief コンストラクタ
	 * @param settings 設定情報への参照
	 * @param a_pTft TFTディスプレイへのポインタ
	 */
	InetAction(Settings& settings, Adafruit_ILI9341* a_pTft);

	/**
	 * @brief Wi-Fiチップの初期化
	 * @retval true 初期化成功
	 * @retval false 初期化失敗
	 */
	bool init();
	/**
	 * @brief Wi-Fiアクセスポイントへ接続
	 * @retval 0 接続成功
	 * @retval 負値またはエラーコード 接続失敗
	 */
	int connect();
	/**
	 * @brief SNTPサーバーから時刻を取得しRTCに設定
	 * @retval true 成功
	 * @retval false 失敗
	 */
	bool setTime();
	/**
	 * @brief ホスト名からIPアドレスを取得
	 * @param hostname 解決したいホスト名
	 * @param timeout_ms タイムアウト（ミリ秒）
	 * @retval true 成功
	 * @retval false 失敗
	 */
	bool getHostByName(const char* hostname, int timeout_ms);

  private:
	/**
	 * @brief SNTPサーバーのホスト名
	 * @details デフォルトは pool.ntp.org
	 */
	// const char* sntp_server_address = "0.pool.ntp.org";
	const char* sntp_server_address = "pool.ntp.org";

  public:
	/**
	 * @brief SNTPサーバーアドレスを設定
	 * @param addr 新しいサーバーアドレス
	 */
	void setSntpServer(const char* addr) { sntp_server_address = addr; }
	/**
	 * @brief SNTPサーバーアドレスを取得
	 * @retval サーバーアドレス文字列
	 */
	const char* getSntpServer() const { return sntp_server_address; }

	const char* getConLasterror() const
	{
		if (lastErr_Connect == 0) return "Success";
		return Settings::getPicoErrorSummary(lastErr_Connect);
	}
	const char* getSntpLasterror() const
	{
		if (lastErr_Connect == 0) return "Success";
		return Settings::getPicoErrorSummary(lastErr_Sntp);
	}
};

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * @brief RTC時刻を設定するC関数
	 * @param sec UNIXエポック秒
	 */
	void set_system_time(u32_t sec);
#ifdef __cplusplus
}
#endif