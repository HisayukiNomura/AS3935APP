/**
 * @file Settings.cpp
 * @brief システム設定値の保存・管理クラスの実装
 * @details
 * フラッシュメモリへの保存・読込、Wi-Fiや画面設定、AS3935等の各種パラメータを一元管理する。
 * 設定値はSettingValue構造体で保持し、アクセッサや編集用メソッドを提供する。
 */
#include "Settings.h"
#include <cstring>
#include <cstdio>
#include "FlashMem.h"
#include <ctime>
#include "pico/stdlib.h"
#include "lwip/apps/sntp.h"
#include "hardware/watchdog.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "ScreenKeyboard.h"
#include "GUIEditbox.h"
#include "hardware/rtc.h"
#include "TouchCalibration.h"
#include "AS3935.h"
#include "GUIMsgBox.h"

using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief Settingsクラスのコンストラクタ
 * @details
 * フラッシュメモリ管理インスタンスを初期化し、デフォルト値を設定する。
 */
Settings::Settings() :
	flash(31, 1) // フラッシュメモリのブロック0を管理するインスタンスを作成
{

	setDefault();
}

/**
 * @brief 設定値をデフォルト値に初期化
 * @details
 * Wi-Fiや画面補正、AS3935用パラメータなど、全システム設定値を初期値でセットする。
 */
void Settings::setDefault()
{

	strncpy(value.chunk, "HNTE", sizeof(value.chunk));
	value.isEnableWifi = true;
	strncpy(value.SSID, "NOMGUEST", sizeof(value.SSID) - 1);
	value.SSID[sizeof(value.SSID) - 1] = '\0';
	strncpy(value.PASSWORD, "0422111111", sizeof(value.PASSWORD) - 1);
	value.PASSWORD[sizeof(value.PASSWORD) - 1] = '\0';
	value.isClock24Hour = true;
	strcpy(value.localeStr, "ja_JP.UTF-8");
	strcpy(value.tzStr, "JST-9");
	strcpy(value.end, "ENDC");
	value.ipAddr[0] = value.ipAddr[1] = value.ipAddr[2] = value.ipAddr[3] = 0;
	value.minX = 392;
	value.minY = 287;
	value.maxX = 3775;
	value.maxY = 3719;
	value.gainBoost = AFE_GB_INDOOR;       // AFEのゲインブースト設定（0-31）
	value.noiseFloor = NFLEV_DEF;          // ノイズフロアレベル（0-7）
	value.watchDogThreshold = 0;           // ウォッチドッグスレッショル
	value.i2cAddr = 0x03;                  // I2Cアドレス（デフォルトは0x00）
	value.spikeReject = SREJ_DEFAULT;      // スパイクリジェクト（0-3）
	value.minimumEvent = NUMLIGHT_DEFAULT; // 最小イベント数（0-3）
}

/**
 * @brief 設定値をフラッシュメモリに保存
 */
void Settings::save()
{
	flash.write(0, (void*)(&value), sizeof(value));
}

/**
 * @brief フラッシュメモリから設定値を読込
 * @return 成功時true
 */
bool Settings::load()
{
	if (flash.read(0, (void*)(&value), sizeof(value)) == false) {
		setDefault(); // エラーが出たらデフォルト値を設定
		return false;
	}
	if (isActive() == false) {
		setDefault(); // チャンク識別子が一致しない場合はデフォルト値を設定
		return false;
	}
	return true;
}

/**
 * @brief 設定値が有効か（チャンク識別子・終端チェック）
 * @return 有効な場合true
 */
bool Settings::isActive()
{
	if (strncmp(value.chunk, "HNTE", 4) != 0) return false;
	if (strncmp(value.end, "ENDC", 4) != 0) return false;
	return true;
}

/**
 * @brief IPアドレスを文字列で取得
 * @return IPアドレス文字列
 */
const char* Settings::getIpString() const
{
	static char ipStr[16];
	snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", value.ipAddr[0], value.ipAddr[1], value.ipAddr[2], value.ipAddr[3]);
	return ipStr;
}

/**
 * @brief IPアドレスを0クリア
 */
void Settings::resetIP()
{
	value.ipAddr[0] = value.ipAddr[1] = value.ipAddr[2] = value.ipAddr[3] = 0;
}

/**
 * @brief IPアドレスを設定
 * @param a_u8Ip0 1バイト目
 * @param a_u8Ip1 2バイト目
 * @param a_u8Ip2 3バイト目
 * @param a_u8Ip3 4バイト目
 */
void Settings::setIP(uint8_t a_u8Ip0, uint8_t a_u8Ip1, uint8_t a_u8Ip2, uint8_t a_u8Ip3)
{
	value.ipAddr[0] = a_u8Ip0;
	value.ipAddr[1] = a_u8Ip1;
	value.ipAddr[2] = a_u8Ip2;
	value.ipAddr[3] = a_u8Ip3;
}

/**
 * @brief タッチパネルキャリブレーション値を設定
 * @param a_minX X最小
 * @param a_minY Y最小
 * @param a_maxX X最大
 * @param a_maxY Y最大
 */
void Settings::setCalibration(uint16_t a_minX, uint16_t a_minY, uint16_t a_maxX, uint16_t a_maxY)
{
	value.minX = a_minX;
	value.minY = a_minY;
	value.maxX = a_maxX;
	value.maxY = a_maxY;
}

/**
 * @brief ロケール文字列取得
 * @return ロケール文字列
 */
const char* Settings::getLocaleStr() const
{
	return value.localeStr;
}

/**
 * @brief タイムゾーン文字列取得
 * @return タイムゾーン文字列
 */
const char* Settings::getTzStr() const
{
	return value.tzStr;
}

/**
 * @brief メニュー下部の共通描画
 */
const void Settings::drawMenuBottom()
{
	// 下部キーボード領域
	ptft->fillRect(0, 320 - ScreenKeyboard::KB_HEIGHT, 320, ScreenKeyboard::KB_HEIGHT, STDCOLOR.BLACK);
	ptft->printlocf(0, 264, "---------+---------+---------+");

	// OKボタン
	int okX = 10, okY = 290, btnW = 90, btnH = 28, r = 8;
	ptft->pushTextColor();
	ptft->fillRoundRect(okX, okY, btnW, btnH, r, STDCOLOR.DARK_BLUE);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.DARK_BLUE);
	ptft->setCursor(okX + 24, okY + 6);
	ptft->printf("設定");

	// CANCELボタン
	int cancelX = 130, cancelY = 290;
	ptft->fillRoundRect(cancelX, cancelY, btnW, btnH, r, STDCOLOR.DARK_RED);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.DARK_RED);
	ptft->setCursor(cancelX + 5, cancelY + 6);
	ptft->printf("キャンセル");
	ptft->popTextColor();
}

/**
 * @brief メイン設定メニュー画面を描画
 */
const void Settings::drawMenu()
{
	ptft->fillScreen(ILI9341_BLACK);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->setCursor(10, 40);
	ptft->printf(" I2CAddr: %d", value.i2cAddr);
	ptft->setCursor(10, 72);
	ptft->printf("    SSID:%s", value.SSID);
	ptft->setCursor(10, 104);
	ptft->printf("PASSWORD:%s", value.PASSWORD);
	ptft->setCursor(10, 136);
	ptft->printf("    TIME:%s", (value.isClock24Hour ? "24H" : "12H"));
	ptft->setCursor(10, 168);
	ptft->printf("DATETIME:");
	ptft->setCursor(10, 200);
	ptft->printf("GB/NF/WD:＋%02dー/＋%1d－/＋%02d－", value.gainBoost, value.noiseFloor, value.watchDogThreshold);
	ptft->setCursor(10, 232);
	ptft->printf("<<< Calibrate Touchscreen>>>");
	drawMenuBottom();
}

/**
 * @brief メイン設定メニューの実行ループ
 * @param a_ptft ディスプレイ制御用
 * @param a_pts タッチスクリーン制御用
 */
const void Settings::run(Adafruit_ILI9341* a_ptft, XPT2046_Touchscreen* a_pts)
{
	SettingValue valuePush;
	valuePush = value; // 現在の設定値を保存

	ptft = a_ptft;
	pts = a_pts;

	ScreenKeyboard sk(ptft, pts);
	isMustSave = false;
	drawMenu();
	uint8_t prevSec = 0xFF;
#define YRANGE(__Y) (p.y >= __Y && p.y < (__Y + 32))
	while (true) {
		time_t now = time(NULL);
		struct tm* local = localtime(&now);
		static char timeBuf[15];

		if (prevSec != local->tm_sec) {
			snprintf(timeBuf, sizeof(timeBuf), "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
			ptft->setCursor(80, 168);
			ptft->printf(timeBuf);
			prevSec = local->tm_sec;
		}

		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {} // タッチが終わるまで待つ
			if YRANGE (32) {          // SSID
				value.i2cAddr++;
				if (value.i2cAddr == 4) value.i2cAddr = 0;
				ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
				ptft->setCursor(10, 40);
				isMustSave = true; // I2Cアドレスが変更された
				ptft->printf(" I2CAddr: %d", value.i2cAddr);
			} else if YRANGE (64) { // SSID
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 9 * 8, 72, value.SSID, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // SSIDが変更された
				}
				drawMenu();
			} else if YRANGE (96) {
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 9 * 8, 104, value.PASSWORD, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // PASSWORDが変更された
				}
				drawMenu();
			} else if YRANGE (128) {
				value.isClock24Hour = !value.isClock24Hour; // 24時間表示の切り替え
				drawMenu();
				isMustSave = true; // 24時間表示が変更された
				ptft->setCursor(10, 136);
				ptft->printf("    TIME:%s", (value.isClock24Hour ? "24H" : "12H"));
			} else if YRANGE (160) { // 日時設定
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(80, 136, timeBuf, sizeof(timeBuf), EditMode::MODE_NUMPADOVERWRITE);
				if (bRet == true) {
					// timeBufの内容を解析して、必要な設定を行う
					int year, month, day, hour, minute, second;
					if (sscanf(timeBuf, "%04d%02d%02d%02d%02d%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
						struct tm newTime = {0};
						newTime.tm_year = year - 1900; // tm_yearは1900年からの年数
						newTime.tm_mon = month - 1;    // tm_monは0から11まで
						newTime.tm_mday = day;
						newTime.tm_hour = hour;
						newTime.tm_min = minute;
						newTime.tm_sec = second;
						time_t t = mktime(&newTime);
						struct timeval tv;
						tv.tv_sec = now;
						tv.tv_usec = 0;
						settimeofday(&tv, NULL);
					}
				}
				drawMenu();
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
				TouchCalibration tsCalib(ptft, pts); // タッチパネルのキャリブレーションを行うインスタンスを作成
				bool bRet = tsCalib.run();
				if (bRet) {
					setCalibration(tsCalib.minX, tsCalib.minY, tsCalib.maxX, tsCalib.maxY); // キャリブレーション値を設定
					isMustSave = true;                                                      // キャリブレーション結果を保存
				}
				drawMenu();
			} else if YRANGE (288) {
				if (p.x < 80) { // OKボタン
					if (isMustSave) {
						save(); // 設定を保存
					}
					ptft->fillScreen(ILI9341_BLACK);
					return;             // メニューを閉じる
				} else if (p.x > 160) { // CANCELボタン
					value = valuePush;  // 変更を破棄
					ptft->fillScreen(ILI9341_BLACK);
					return; // メニューを閉じる
				}
				return;
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}

/**
 * @brief ルートメニュー画面を描画
 */
const void Settings::drawMenu2_root()
{
	ptft->fillScreen(ILI9341_BLACK);
	ptft->fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	ptft->setCursor(10, 4);
	ptft->printf("設定");

	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->setCursor(10, 40);
	ptft->printf("システム設定");
	ptft->setCursor(10, 72);
	ptft->printf("AS3935設定(要再起動)");
	ptft->setCursor(10, 104);
	ptft->printf("ネットワーク設定(要再起動)");
	ptft->setCursor(10, 136);
	ptft->printf("タッチパネルの調整");
	ptft->setCursor(10, 168);
	ptft->printf("");
	ptft->setCursor(10, 200);
	ptft->printf("");
	ptft->setCursor(10, 232);
	ptft->printf("");
	drawMenuBottom();
}

/**
 * @brief ルートメニューの実行ループ
 * @param a_ptft ディスプレイ制御用
 * @param a_pts タッチスクリーン制御用
 */
const bool Settings::run2(Adafruit_ILI9341* a_ptft, XPT2046_Touchscreen* a_pts)
{
	SettingValue valuePush;
	valuePush = value; // 現在の設定値を保存

	ptft = a_ptft;
	pts = a_pts;

	ScreenKeyboard sk(ptft, pts);
	isMustSave = false;
	menuMode = 0;
	drawMenu2_root();
	uint8_t prevSec = 0xFF;
	while (true) {

		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {} // タッチが終わるまで待つ
			if YRANGE (32) {          // SSID
				menuMode = 1;
				run2_system(); // システム設定メニューを表示
				menuMode = 0;
				drawMenu2_root();
			} else if YRANGE (64) { // SSID
				menuMode = 2;
				run2_as3935(); // システム設定メニューを表示
				menuMode = 0;
				drawMenu2_root();
			} else if YRANGE (96) {
				menuMode = 3;
				run2_wifi(); // システム設定メニューを表示
				menuMode = 0;
				drawMenu2_root();
			} else if YRANGE (128) {
				TouchCalibration tsCalib(ptft, pts); // タッチパネルのキャリブレーションを行うインスタンスを作成
				bool bRet = tsCalib.run();
				if (bRet) {
					setCalibration(tsCalib.minX, tsCalib.minY, tsCalib.maxX, tsCalib.maxY); // キャリブレーション値を設定
					isMustSave = true;                                                      // キャリブレーション結果を保存
				}
				drawMenu2_root();
			} else if YRANGE (160) { // 日時設定
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
			} else if YRANGE (288) {
				if (p.x < 80) { // OKボタン
					if (isMustSave) {
						save();             // 設定を保存
						isMustSave = false; // 保存後はフラグをリセット
					}
					ptft->fillScreen(ILI9341_BLACK);
					if (isMustReboot) {
						// 再起動問い合わせ
						GUIMsgBox msgbox(ptft, pts);
						bool bRet = msgbox.showOKCancel(30, 100, "確認", "設定を反映するには\n再起動が必要です", "再起動", " 中断 ");
						if (bRet) {
							watchdog_reboot(0, 0, 0);
						} else {
							drawMenu2_root();
						}
					} else {
						return true;
					}

				} else if (p.x > 160) { // CANCELボタン
					value = valuePush;  // 変更を破棄
					ptft->fillScreen(ILI9341_BLACK);
					return false;
				}
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}

/**
 * @brief システム設定メニュー画面を描画
 */
const void Settings::drawMenu2_system()
{
	ptft->fillScreen(ILI9341_BLACK);
	ptft->fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	ptft->setCursor(10, 4);
	ptft->printf("設定 - システム");

	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->printlocf(10, 40, "時刻表示: %s", (value.isClock24Hour ? "24H" : "12H"));
	ptft->printlocf(10, 72, "    日時:");
	drawMenuBottom();
}

/**
 * @brief システム設定メニューの実行ループ
 */
const void Settings::run2_system()
{
	SettingValue valuePush;
	valuePush = value; // 現在の設定値を保存

	ScreenKeyboard sk(ptft, pts);
	isMustSave = false;
	menuMode = 0;
	drawMenu2_system();
	uint8_t prevSec = 0xFF;
	while (true) {
		time_t now = time(NULL);
		struct tm* local = localtime(&now);
		static char timeBuf[15]; // YYYYMMDDhhmmss

		if (prevSec != local->tm_sec) {
			snprintf(timeBuf, sizeof(timeBuf), "%04d%02d%02d%02d%02d%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
			ptft->printlocf(80, 72, timeBuf);
			prevSec = local->tm_sec;
		}
		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {}                       // タッチが終わるまで待つ
			if YRANGE (32) {                                // 時刻表記
				value.isClock24Hour = !value.isClock24Hour; // 24時間表示の切り替え
				isMustSave = true;                          // 24時間表示が変更された
				ptft->setCursor(10, 40);
				ptft->printf("時刻表示: %s", (value.isClock24Hour ? "24H" : "12H"));
			} else if YRANGE (64) { // 日時
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(80, 72, timeBuf, 14, EditMode::MODE_NUMPADOVERWRITE);
				if (bRet == true) {
					// timeBufの内容を解析して、必要な設定を行う
					int year, month, day, hour, minute, second;
					if (sscanf(timeBuf, "%04d%02d%02d%02d%02d%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
						struct tm newTime = {0};
						newTime.tm_year = year - 1900; // tm_yearは1900年からの年数
						newTime.tm_mon = month - 1;    // tm_monは0から11まで
						newTime.tm_mday = day;
						newTime.tm_hour = hour;
						newTime.tm_min = minute;
						newTime.tm_sec = second;
						time_t t = mktime(&newTime);
						struct timeval tv;
						tv.tv_sec = t;
						tv.tv_usec = 0;
						settimeofday(&tv, NULL);
					}
				}
				drawMenu2_system();

			} else if YRANGE (96) {
			} else if YRANGE (128) {
			} else if YRANGE (160) { // 日時設定
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
			} else if YRANGE (288) {
				if (p.x < 80) { // OKボタン
					if (isMustSave) {
						save();             // 設定を保存
						isMustSave = false; // 保存後はフラグをリセット
					}
					ptft->fillScreen(ILI9341_BLACK);
					return;             // メニューを閉じる
				} else if (p.x > 160) { // CANCELボタン
					value = valuePush;  // 変更を破棄
					ptft->fillScreen(ILI9341_BLACK);
					return; // メニューを閉じる
				}
				return;
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}

/**
 * @brief Wi-Fi設定メニュー画面を描画
 */
const void Settings::drawMenu2_wifi()
{
	ptft->fillScreen(ILI9341_BLACK);
	ptft->fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	ptft->setCursor(10, 4);
	ptft->printf("設定 - Wi-Fi");

	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->printlocf(10, 40, "    WIFI: %s", value.isEnableWifi ? "有効" : "無効");
	ptft->printlocf(10, 72, "    SSID: %s", value.SSID);
	ptft->printlocf(10, 104, "PASSWORD: %s", value.PASSWORD);
	drawMenuBottom();
}

/**
 * @brief Wi-Fi設定メニューの実行ループ
 */
const void Settings::run2_wifi()
{
	SettingValue valuePush;
	valuePush = value; // 現在の設定値を保存

	ScreenKeyboard sk(ptft, pts);
	isMustSave = false;
	menuMode = 0;
	drawMenu2_wifi();
	while (true) {
		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {} // タッチが終わるまで待つ
			if YRANGE (32) {          // SSID
				if (value.isEnableWifi) {
					value.isEnableWifi = false; // Wi-Fiを無効化
				} else {
					value.isEnableWifi = true;  // Wi-Fiを有効化
				}
				isMustSave = true; // WIFI設定変更
				ptft->printlocf(10, 40, "    WIFI: %s", value.isEnableWifi ? "有効" : "無効");
			} else if YRANGE (64) { // SSID
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 8 * 10, 72, value.SSID, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // SSIDが変更された
				}
				drawMenu2_wifi();
			} else if YRANGE (96) { // PASSWORD
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 8 * 10, 104, value.PASSWORD, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // PASSWORDが変更された
				}
				drawMenu2_wifi();
			} else if YRANGE (128) {
			} else if YRANGE (160) {
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
			} else if YRANGE (288) {
				if (p.x < 80) { // OKボタン
					if (isMustSave) {
						save();             // 設定を保存
						isMustSave = false; // 保存後はフラグをリセット
					}
					ptft->fillScreen(ILI9341_BLACK);
					isMustReboot = true; // Wi-Fi設定が変更されたので再起動が必要
					return;              // メニューを閉じる
				} else if (p.x > 160) {  // CANCELボタン
					value = valuePush;   // 変更を破棄
					ptft->fillScreen(ILI9341_BLACK);
					return; // メニューを閉じる
				}
				return;
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}

/**
 * @brief AS3935設定メニュー画面を描画
 */
const void Settings::drawMenu2_as3935()
{
	uint8_t minEvtTbl[] = {1, 5, 9, 16};

	ptft->fillScreen(ILI9341_BLACK);
	ptft->fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	ptft->setCursor(10, 4);
	ptft->printf("設定 - AS3935");

	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->printlocf(0, 40, "I2C ADDR: %d", value.i2cAddr);
	ptft->printlocf(0, 72, "GAINBOST: %02d ", value.gainBoost);
	ptft->printlocf(0, 104, "NOISEFLR: %1d  WATCHDOG: %02d", value.noiseFloor, value.watchDogThreshold);
	ptft->printlocf(0, 136, "MINLIGHT: %02d SPIKEREJ: %02d", minEvtTbl[value.minimumEvent], value.spikeReject);
	drawMenuBottom();
}

/**
 * @brief AS3935設定メニューの実行ループ
 */
const void Settings::run2_as3935()
{
	SettingValue valuePush;
	valuePush = value; // 現在の設定値を保存

	ScreenKeyboard sk(ptft, pts);
	isMustSave = false;
	menuMode = 0;
	drawMenu2_as3935();
	char edtBuf[15];
	memset(edtBuf, 0, sizeof(edtBuf)); // 編集用バッファをクリア
	while (true) {
		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {} // タッチが終わるまで待つ
			if YRANGE (32) {          // I2Cアドレス
				value.i2cAddr++;
				if (value.i2cAddr == 4) value.i2cAddr = 0;
				ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
				isMustSave = true; // I2Cアドレスが変更された
				ptft->printlocf(0, 40, "I2C ADDR: %d", value.i2cAddr);
			} else if YRANGE (64) {                       // ゲインブースト
				sprintf(edtBuf, "%02d", value.gainBoost); // ゲインブースト値を文字列に変換
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(8 * 10, 72, edtBuf, 2, EditMode::MODE_NUMPADOVERWRITE);
				if (bRet == true) {
					uint8_t gb = atoi(edtBuf);
					if (gb == 99) gb = AFE_GB_INDOOR; // 99はデフォルト値
					if (gb > AFE_GB_MAX) gb = AFE_GB_MAX;
					value.gainBoost = gb; // ゲインブーストを設定
					isMustSave = true;    // ゲインブーストが変更された
				}
				ptft->printlocf(0, 72, "GAINBOST: %02d", value.gainBoost);
				drawMenuBottom();

			} else if YRANGE (96) { // ノイズフロア
				if (p.x < 120) {
					sprintf(edtBuf, "%1d", value.noiseFloor); // ノイズフロア値を文字列に変換
					GUIEditBox editbox(ptft, pts, &sk);
					bool bRet = editbox.show(8 * 10, 104, edtBuf, 1, EditMode::MODE_NUMPADOVERWRITE);
					if (bRet == true) {
						uint8_t nf = atoi(edtBuf);
						if (nf == 9) nf = NFLEV_DEF; // 9はデフォルト値
						if (nf > 7) nf = 7;          // ７でサチる
						value.noiseFloor = nf;       // ノイズフロアを設定
						isMustSave = true;           // ノイズフロアが変更された
					}
					ptft->printlocf(0, 104, "NOISEFLR: %1d", value.noiseFloor);
					drawMenuBottom();
				} else {
					sprintf(edtBuf, "%02d", value.watchDogThreshold); // ウォッチドッグスレッショルド値を文字列に変換
					GUIEditBox editbox(ptft, pts, &sk);
					bool bRet = editbox.show(184, 104, edtBuf, 2, EditMode::MODE_NUMPADOVERWRITE);
					if (bRet == true) {
						uint8_t wd = atoi(edtBuf);
						if (wd == 99) wd = WDTH_DEFAULT;  // 99はデフォルト値
						if (wd > WDTH_MAX) wd = WDTH_MAX; // 最大値を超えないようにする
						value.watchDogThreshold = wd;     // ウォッチドッグスレッショルドを設定
						isMustSave = true;                // ウォッチドッグスレッショルドが変更された
					}
					ptft->printlocf(104, 104, "WATCHDOG: %02d", value.watchDogThreshold);
					drawMenuBottom();
				}
			} else if YRANGE (128) { //
				if (p.x < 120) {
					uint8_t minEvtTbl[] = {1, 5, 9, 16};
					value.minimumEvent++;
					value.minimumEvent = value.minimumEvent & 0b00000011; // 0-3の範囲に制限
					ptft->printlocf(0, 136, "MINLIGHT: %02d", minEvtTbl[value.minimumEvent]);
					drawMenuBottom();
				} else {
					sprintf(edtBuf, "%02d", value.spikeReject); // スパイクリジェクト値を文字列に変換
					GUIEditBox editbox(ptft, pts, &sk);
					bool bRet = editbox.show(184, 136, edtBuf, 2, EditMode::MODE_NUMPADOVERWRITE);
					if (bRet == true) {
						uint8_t sr = atoi(edtBuf);
						if (sr == 99) sr = SREJ_DEFAULT; // 99はデフォルト値
						if (sr > SREJ_MAX) sr = SREJ_MAX;
						value.spikeReject = sr; // スパイクリジェクトを設定
						isMustSave = true;      // スパイクリジェクトが変更された
					}
					ptft->printlocf(104, 136, "SPIKEREJ: %02d", value.spikeReject);
				}
			} else if YRANGE (160) { //
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
			} else if YRANGE (288) {
				if (p.x < 80) { // OKボタン
					if (isMustSave) {
						save();             // 設定を保存
						isMustSave = false; // 保存後はフラグをリセット
					}
					ptft->fillScreen(ILI9341_BLACK);
					isMustReboot = true; // Wi-Fi設定が変更されたので再起動が必要
					return;              // メニューを閉じる
				} else if (p.x > 160) {  // CANCELボタン
					value = valuePush;   // 変更を破棄
					ptft->fillScreen(ILI9341_BLACK);
					return; // メニューを閉じる
				}
				return;
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}

/**
 * @brief pico_error_codesの値から簡易説明文字列を返す
 * @param code pico_error_codesの値
 * @return エラー内容の簡易説明文字列
 */
const char* Settings::getPicoErrorSummary(int code)
{
	switch (code) {
		case 0: return "Success";
		case -1: return "Generic error";
		case -2: return "Timeout";
		case -3: return "No data";
		case -4: return "Not permitted";
		case -5: return "Invalid argument";
		case -6: return "I/O error";
		case -7: return "Bad auth";
		case -8: return "Connect failed";
		case -9: return "Insufficient resources";
		case -10: return "Invalid address";
		case -11: return "Bad alignment";
		case -12: return "Invalid state";
		case -13: return "Buffer too small";
		case -14: return "Precondition not met";
		case -15: return "Modified data";
		case -16: return "Invalid data";
		case -17: return "Not found";
		case -18: return "Unsupported modification";
		case -19: return "Lock required";
		case -20: return "Version mismatch";
		case -21: return "Resource in use";
		// ここから先は、ローカルで使用
		case -100: return "DNS lookup failed";
		case -101: return "Heap alloc failure";
		case -102: return "Flash write failure";

		default: return "Unknown error code";
	}
}
