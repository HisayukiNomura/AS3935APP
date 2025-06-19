#include "Settings.h"
#include <cstring>
#include <cstdio>
#include "FlashMem.h"
#include <ctime>
#include "pico/stdlib.h"
#include "lwip/apps/sntp.h"

#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "ScreenKeyboard.h"
#include "GUIEditbox.h"
#include "hardware/rtc.h"
#include "TouchCalibration.h"
using namespace ardPort;
using namespace ardPort::spi;

Settings::Settings() :
	flash(31, 1) // フラッシュメモリのブロック0を管理するインスタンスを作成
{

	setDefault();
}
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
	value.minX = 432;
	value.minY = 374;
	value.maxX = 3773;
	value.maxY = 3638;
}

void Settings::save()
{
	flash.write(0, (void*)(&value), sizeof(value));
}

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
bool Settings::isActive()
{
	if (strncmp(value.chunk, "HNTE", 4) != 0) return false;
	if (strncmp(value.end, "ENDC", 4) != 0) return false;
	return true;
}

const char* Settings::getIpString() const
{
	static char ipStr[16];
	snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", value.ipAddr[0], value.ipAddr[1], value.ipAddr[2], value.ipAddr[3]);
	return ipStr;
}

void Settings::resetIP()
{
	value.ipAddr[0] = value.ipAddr[1] = value.ipAddr[2] = value.ipAddr[3] = 0;
}

void Settings::setIP(uint8_t a_u8Ip0, uint8_t a_u8Ip1, uint8_t a_u8Ip2, uint8_t a_u8Ip3)
{
	value.ipAddr[0] = a_u8Ip0;
	value.ipAddr[1] = a_u8Ip1;
	value.ipAddr[2] = a_u8Ip2;
	value.ipAddr[3] = a_u8Ip3;
}

void Settings::setCalibration(uint16_t a_minX, uint16_t a_minY, uint16_t a_maxX, uint16_t a_maxY)
{
	value.minX = a_minX;
	value.minY = a_minY;
	value.maxX = a_maxX;
	value.maxY = a_maxY;
}

const char* Settings::getLocaleStr() const
{
	return value.localeStr;
}

const char* Settings::getTzStr() const
{
	return value.tzStr;
}

const void Settings::drawMenu()
{
	ptft->fillScreen(ILI9341_BLACK);
	ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	ptft->setCursor(10, 40);
	ptft->printf("    SSID:%s", value.SSID);
	ptft->setCursor(10, 72);
	ptft->printf("PASSWORD:%s", value.PASSWORD);
	ptft->setCursor(10, 104);
	ptft->printf("    TIME:%s", (value.isClock24Hour ? "24H" : "12H"));
	ptft->setCursor(10, 136);
	ptft->printf("DATETIME:");
	ptft->setCursor(10, 168);
	ptft->printf("--------:");
	ptft->setCursor(10, 200);
	ptft->printf("--------:");
	ptft->setCursor(10, 232);
	ptft->printf("<<< Calibrate Touchscreen>>>");
	ptft->setCursor(0, 264);
	ptft->printf("---------+---------+---------+");
	ptft->setCursor(10, 296);
	ptft->printf("  [ OK ]            [CANCEL]");
}

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
			ptft->setCursor(80, 136);
			ptft->printf(timeBuf);
			prevSec = local->tm_sec;
		}

		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			while (pts->touched()) {}					// タッチが終わるまで待つ
			if YRANGE (32) { // SSID
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 9 * 8, 40, value.SSID, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // SSIDが変更された
				}
				drawMenu();
			} else if YRANGE (64) {
				GUIEditBox editbox(ptft, pts, &sk);
				bool bRet = editbox.show(10 + 9 * 8, 72, value.PASSWORD, 18, EditMode::MODE_TEXT);
				if (bRet) {
					isMustSave = true; // PASSWORDが変更された
				}
				drawMenu();
			} else if YRANGE (96) {
				value.isClock24Hour = !value.isClock24Hour; // 24時間表示の切り替え
				drawMenu();
				isMustSave = true;   // 24時間表示が変更された
			} else if YRANGE (128) { // 日時設定
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
			} else if YRANGE (160) {
			} else if YRANGE (192) {
			} else if YRANGE (224) {
				TouchCalibration tsCalib(ptft, pts); // タッチパネルのキャリブレーションを行うインスタンスを作成
				bool bRet = tsCalib.run();
				if (bRet) {
					setCalibration(tsCalib.minX, tsCalib.minY, tsCalib.maxX, tsCalib.maxY); // キャリブレーション値を設定
					isMustSave = true;                                                      // キャリブレーション結果を保存
				}
				drawMenu();
			} else if YRANGE (256) {
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