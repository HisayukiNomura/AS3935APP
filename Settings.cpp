#include "Settings.h"
#include <cstring>
#include <cstdio>
#include "FlashMem.h"

#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "ScreenKeyboard.h"
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
	strncpy(value.PASSWORD, "value.0422111111", sizeof(value.PASSWORD) - 1);
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
	ptft->printf("--------:");
	ptft->setCursor(10, 168);
	ptft->printf("--------:");
	ptft->setCursor(10, 200);
	ptft->printf("--------:");
	ptft->setCursor(10, 232);
	ptft->printf("--------:");
	ptft->setCursor(10, 264);
	ptft->printf("--------:");
	ptft->setCursor(10, 296);
	ptft->printf("<<EXIT>>:");
}

const void Settings::run(Adafruit_ILI9341* a_ptft, XPT2046_Touchscreen* a_pts)
{
	ptft = a_ptft;
	pts = a_pts;

	ScreenKeyboard sk(ptft, pts);
	drawMenu();
#define YRANGE(__Y) (p.y >= __Y && p.y < (__Y + 32))
	while (true) {
		if (pts->touched()) {
			TS_Point p = pts->getPointOnScreen();
			if YRANGE (32) {
				sk.show(200);
			} else if YRANGE (64) {
			} else if YRANGE (96) {
			} else if YRANGE (128) {
			} else if YRANGE (160) {
			} else if YRANGE (192) {
			} else if YRANGE (224) {
			} else if YRANGE (256) {
			} else if YRANGE (288) {
				return;
			} else {
			}
		}
		delay(100); // 少し待つ
	}
}