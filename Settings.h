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



class __attribute__((packed)) SettingValue
{
  public:
	char chunk[4];
	bool isEnableWifi; // Wi-Fiを有効にするかどうかのフラグ
	char SSID[32];
	char PASSWORD[32];
	uint8_t ipAddr[4];  // IPアドレスを格納する配列
	bool isClock24Hour; // 24時間表示かどうかのフラグ
	char localeStr[32]; // ロケール文字列（例: "ja_JP.UTF-8"）
	char tzStr[16];     // タイムゾーン文字列（例: "JST-9"）
	uint16_t minX;
	uint16_t minY;
	uint16_t maxX;
	uint16_t maxY;
	char end[4]; // チャンクの終端を示す文字列（例: "ENDC"）
};

class Settings
{
  private:
	FlashMem flash;
	Adafruit_ILI9341* ptft;
	XPT2046_Touchscreen* pts;
  public:
	SettingValue value;

  public:
	Settings();
	void setDefault();
	void save();
	bool load();

	bool isActive();
	const char* getIpString() const;
	void resetIP();
	void setIP(uint8_t a_u8Ip0, uint8_t a_u8Ip1, uint8_t a_u8Ip2, uint8_t a_u8Ip3);
	void setCalibration(uint16_t a_minX, uint16_t a_minY, uint16_t a_maxX, uint16_t a_maxY);
	const char* getLocaleStr() const;
	const char* getTzStr() const;

	// --- アクセッサ ---
	bool getIsEnableWifi() const { return value.isEnableWifi; }
	void setIsEnableWifi(bool v) { value.isEnableWifi = v; }
	const char* getLocale() const { return value.localeStr; }
	uint16_t getMinX() const { return value.minX; }
	uint16_t getMinY() const { return value.minY; }
	uint16_t getMaxX() const { return value.maxX; }
	uint16_t getMaxY() const { return value.maxY; }
	void setMinX(uint16_t v) { value.minX = v; }
	void setMinY(uint16_t v) { value.minY = v; }
	void setMaxX(uint16_t v) { value.maxX = v; }
	void setMaxY(uint16_t v) { value.maxY = v; }
	bool getIsClock24Hour() const { return value.isClock24Hour; }
	void setIsClock24Hour(bool v) { value.isClock24Hour = v; }
	const char* getSSID() const { return value.SSID; }
	void setSSID(const char* s)
	{
		strncpy(value.SSID, s, sizeof(value.SSID) - 1);
		value.SSID[sizeof(value.SSID) - 1] = '\0';
	}
	const char* getPASSWORD() const { return value.PASSWORD; }
	void setPASSWORD(const char* s)
	{
		strncpy(value.PASSWORD, s, sizeof(value.PASSWORD) - 1);
		value.PASSWORD[sizeof(value.PASSWORD) - 1] = '\0';
	}
	const void drawMenu();

	
	const void run(Adafruit_ILI9341* a_pTft, XPT2046_Touchscreen* a_pTs);
	// エディットボックスの表示
	char* p = nullptr; // 編集対象の文字列ポインタ
	uint8_t edtCursor = 0;
	uint16_t edtX = 0;
	uint16_t edtY = 0;
	bool isInsert = false;

	const void EdtDispCursor(int tick, bool isDeleteCursor = false);
	const void EditBox(ScreenKeyboard sk, uint16_t a_x, uint16_t a_y, char* a_pText, size_t a_size);
};