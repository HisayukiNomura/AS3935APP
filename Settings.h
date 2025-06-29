/**
 * @file Settings.h
 * @brief システム設定値の保存・管理クラスの宣言
 * @details
 * フラッシュメモリへの保存・読込、Wi-Fiや画面設定、AS3935等の各種パラメータを一元管理する。
 * 設定値はSettingValue構造体で保持し、アクセッサや編集用メソッドを提供する。
 */
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include "FlashMem.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
using namespace ardPort;
using namespace ardPort::spi;
class ScreenKeyboard; // 前方宣言

/**
 * @brief 設定値を格納する構造体
 * @details
 * Wi-Fiや画面補正、AS3935用パラメータなど、全システム設定値を1つの構造体で管理する。
 * フラッシュ保存のため __attribute__((packed)) で詰めている。
 */
class __attribute__((packed)) SettingValue
{
  public:
	char chunk[4];             ///< チャンク識別子（"HNTE"）
	bool isEnableWifi;         ///< Wi-Fiを有効にするか
	char SSID[32];             ///< Wi-Fi SSID
	char PASSWORD[32];         ///< Wi-Fiパスワード
	uint8_t ipAddr[4];         ///< IPアドレス
	bool isClock24Hour;        ///< 24時間表示か
	char localeStr[32];        ///< ロケール文字列（例: "ja_JP.UTF-8"）
	char tzStr[16];            ///< タイムゾーン文字列（例: "JST-9"）
	uint16_t minX;             ///< タッチパネルX最小値
	uint16_t minY;             ///< タッチパネルY最小値
	uint16_t maxX;             ///< タッチパネルX最大値
	uint16_t maxY;             ///< タッチパネルY最大値
	uint8_t gainBoost;         ///< AFEゲインブースト（0-31）
	uint8_t noiseFloor;        ///< ノイズフロアレベル（0-7）
	uint8_t watchDogThreshold; ///< ウォッチドッグスレッショルド（0-15）
	uint8_t spikeReject;       ///< スパイクリジェクト（0-3）
	uint8_t minimumEvent;      ///< 最小イベント数（0-3）
	uint8_t i2cAddr;           ///< I2Cアドレス
	uint8_t i2cReadMode;	   ///< ブロックリード（Single: 無効, Block: 有効）
	char end[4];               ///< チャンク終端（"ENDC"）

	/**
	 * @brief 代入演算子（ディープコピー）
	 * @param rhs コピー元
	 * @return 自身への参照
	 */
	SettingValue& operator=(const SettingValue& rhs)
	{
		if (this != &rhs) {
			memcpy(this, &rhs, sizeof(SettingValue));
		}
		return *this;
	}
};

/**
 * @brief システム設定値の保存・管理クラス
 * @details
 * フラッシュメモリへの保存・読込、Wi-Fiや画面設定、AS3935等の各種パラメータを一元管理する。
 * 設定値はSettingValue構造体で保持し、アクセッサや編集用メソッドを提供する。
 */
class Settings
{
  private:
	FlashMem flash;           ///< フラッシュメモリ管理用
	Adafruit_ILI9341* ptft;   ///< ディスプレイ制御用
	XPT2046_Touchscreen* pts; ///< タッチスクリーン制御用
  public:
	SettingValue value; ///< 現在の設定値

	bool isMustReboot = false; ///< 設定変更後、再起動が必要かどうか
	bool isMustSave = false;   ///< 設定が変更されたかどうか

  public:
	/**
	 * @brief コンストラクタ
	 */
	Settings();
	/**
	 * @brief デフォルト値を設定
	 */
	void setDefault();
	/**
	 * @brief 設定値をフラッシュに保存
	 */
	void save();
	/**
	 * @brief フラッシュから設定値を読込
	 * @return 成功時true
	 */
	bool load();

	/**
	 * @brief 設定値が有効か（チャンク識別子・終端チェック）
	 * @return 有効な場合true
	 */
	bool isActive();
	/**
	 * @brief IPアドレスを文字列で取得
	 * @return IPアドレス文字列
	 */
	const char* getIpString() const;
	/**
	 * @brief IPアドレスを0クリア
	 */
	void resetIP();
	/**
	 * @brief IPアドレスを設定
	 * @param a_u8Ip0 1バイト目
	 * @param a_u8Ip1 2バイト目
	 * @param a_u8Ip2 3バイト目
	 * @param a_u8Ip3 4バイト目
	 */
	void setIP(uint8_t a_u8Ip0, uint8_t a_u8Ip1, uint8_t a_u8Ip2, uint8_t a_u8Ip3);
	/**
	 * @brief タッチパネルキャリブレーション値を設定
	 * @param a_minX X最小
	 * @param a_minY Y最小
	 * @param a_maxX X最大
	 * @param a_maxY Y最大
	 */
	void setCalibration(uint16_t a_minX, uint16_t a_minY, uint16_t a_maxX, uint16_t a_maxY);
	/**
	 * @brief ロケール文字列取得
	 * @return ロケール文字列
	 */
	const char* getLocaleStr() const;
	/**
	 * @brief タイムゾーン文字列取得
	 * @return タイムゾーン文字列
	 */
	const char* getTzStr() const;

	// --- アクセッサ ---
	bool getIsEnableWifi() const { return value.isEnableWifi; }   ///< Wi-Fi有効フラグ取得
	void setIsEnableWifi(bool v) { value.isEnableWifi = v; }      ///< Wi-Fi有効フラグ設定
	const char* getLocale() const { return value.localeStr; }     ///< ロケール取得
	uint16_t getMinX() const { return value.minX; }               ///< X最小取得
	uint16_t getMinY() const { return value.minY; }               ///< Y最小取得
	uint16_t getMaxX() const { return value.maxX; }               ///< X最大取得
	uint16_t getMaxY() const { return value.maxY; }               ///< Y最大取得
	void setMinX(uint16_t v) { value.minX = v; }                  ///< X最小設定
	void setMinY(uint16_t v) { value.minY = v; }                  ///< Y最小設定
	void setMaxX(uint16_t v) { value.maxX = v; }                  ///< X最大設定
	void setMaxY(uint16_t v) { value.maxY = v; }                  ///< Y最大設定
	bool getIsClock24Hour() const { return value.isClock24Hour; } ///< 24時間表示取得
	void setIsClock24Hour(bool v) { value.isClock24Hour = v; }    ///< 24時間表示設定
	const char* getSSID() const { return value.SSID; }            ///< SSID取得
	void setSSID(const char* s)
	{
		strncpy(value.SSID, s, sizeof(value.SSID) - 1);
		value.SSID[sizeof(value.SSID) - 1] = '\0';
	}
	const char* getPASSWORD() const { return value.PASSWORD; } ///< パスワード取得
	void setPASSWORD(const char* s)
	{
		strncpy(value.PASSWORD, s, sizeof(value.PASSWORD) - 1);
		value.PASSWORD[sizeof(value.PASSWORD) - 1] = '\0';
	}
	int geti2cReadMode() const { return value.i2cReadMode; } ///< I2Cリードモード取得（0: Single, 1: Block）

	uint8_t menuMode = 0; ///< メニュー状態
	const void drawMenuBottom();
	const void drawMenu();
	const void drawMenu2_root();
	const void drawMenu2_system();
	const void drawMenu2_wifi();
	const void drawMenu2_as3935();

	const void run(Adafruit_ILI9341* a_pTft, XPT2046_Touchscreen* a_pTs);
	const bool run2(Adafruit_ILI9341* a_pTft, XPT2046_Touchscreen* a_pTs);
	const void run2_system();
	const void run2_wifi();
	const void run2_as3935();

	// エディットボックスの表示
	char* p = nullptr;     ///< 編集対象の文字列ポインタ
	uint8_t edtCursor = 0; ///< 編集カーソル位置
	uint16_t edtX = 0;     ///< 編集ボックスX座標
	uint16_t edtY = 0;     ///< 編集ボックスY座標
	bool isInsert = false; ///< 挿入モード

	const void EdtDispCursor(int tick, bool isDeleteCursor = false);
	const void EditBox(ScreenKeyboard sk, uint16_t a_x, uint16_t a_y, char* a_pText, size_t a_size);

  public:
	static const char* getPicoErrorSummary(int code);
};