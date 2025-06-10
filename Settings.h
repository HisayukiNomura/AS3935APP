#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
class __attribute__((packed)) Settings
{
  public:
	char chunk[4];
	bool isEnableWifi; // Wi-Fiを有効にするかどうかのフラグ
	char SSID[32];
	char PASSWORD[32];
	uint8_t ipAddr[4]; // IPアドレスを格納する配列
	bool isClock24Hour; // 24時間表示かどうかのフラグ
	char localeStr[32]; // ロケール文字列（例: "ja_JP.UTF-8"）
	char tzStr[16]; // タイムゾーン文字列（例: "JST-9"）
	char end[4]; // チャンクの終端を示す文字列（例: "ENDC"）

	void setDefault()
	{
		strncpy(chunk, "HNTE", sizeof(chunk));                         // チャンク識別子を設定
		isEnableWifi = true;                                           // デフォルトではWi-Fiを有効にする
		strncpy(SSID, "NOMGUEST", sizeof(SSID) - 1);                       // SSIDのデフォルト値
		SSID[sizeof(SSID) - 1] = '\0';                                 // ゼロ終端
		strncpy(PASSWORD, "0422111111", sizeof(PASSWORD) - 1);           // パスワードのデフォルト値
		PASSWORD[sizeof(PASSWORD) - 1] = '\0';                         // ゼロ終端
		isClock24Hour = true;                                        // 24時間表示をデフォルトに設定
		strcpy(localeStr, "ja_JP.UTF-8");       // ロケール文字列のデフォルト値
		strcpy(tzStr, "JST-9");       // ロケール文字列のデフォルト値
		strcpy(end, "ENDC"); // チャンクの終端を示す文字列
		ipAddr[0] = ipAddr[1] = ipAddr[2] = ipAddr[3] = 0; // IPアドレスの初期化
	}
	bool isActive()
	{
		if (strncmp(chunk, "HNTE", 4) != 0) return false; // チャンク識別子が一致しない場合は無効
		if (strncmp(end, "ENDC", 4) != 0 ) return false;
		return true;
	}

	/**
	 * @brief IPアドレスを文字列で返す
	 * @return "xxx.xxx.xxx.xxx"形式のIPアドレス文字列（static領域）
	 */
	const char* getIpString() const
	{
		static char ipStr[16];
		snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
		return ipStr;
	}

    void resetIP()
    {
        ipAddr[0] = ipAddr[1] = ipAddr[2] = ipAddr[3] = 0; // IPアドレスの初期化
    }
    void setIP(uint8_t a_u8Ip0, uint8_t a_u8Ip1, uint8_t a_u8Ip2, uint8_t a_u8Ip3)
    {
        ipAddr[0] = a_u8Ip0;
        ipAddr[1] = a_u8Ip1;
        ipAddr[2] = a_u8Ip2;
        ipAddr[3] = a_u8Ip3;
    }
	const char* getLocaleStr() const
	{
		return localeStr;
	}
	const char* getTzStr() const
	{
		return tzStr;
	}
} ;