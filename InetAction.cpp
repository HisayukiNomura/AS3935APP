#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "hardware/rtc.h"

#pragma GCC optimize("O0")

#include "InetAction.h"

#include <ctime>


InetAction::InetAction(Settings& a_settings, Adafruit_ILI9341* a_pTft) :
	pTft(a_pTft),
	settings(a_settings)
{
}

bool InetAction::init()
{
	// Initialise the Wi-Fi chip
	if (cyw43_arch_init()) {
		pTft->printf("cyw43_arch_init failed\n");
		return false;
	} else {
		pTft->printf("Success!\n");
	}
	// Enable wifi station
	pTft->printf("Enable Wifi on station mode\n");
	cyw43_arch_enable_sta_mode();
	return true;
}
int InetAction::connect()
{
	pTft->printf("Connecting to Wi-Fi ... ");
	settings.resetIP(); // IPアドレスをリセット
	int iRet = cyw43_arch_wifi_connect_timeout_ms(settings.SSID, settings.PASSWORD, CYW43_AUTH_WPA3_WPA2_AES_PSK, 5000000);
	if (iRet != 0) {
		printf("Failed(%d).\n", iRet);
		return iRet;
	}
	uint8_t* ip_addr = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
	settings.setIP(ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
	pTft->printf("Success!\n");
	pTft->printf("IP address: %s\n", settings.getIpString());
	return 0;
}

void dns_callback(const char* name, const ip_addr_t* ipaddr, void* callback_arg)
{
	InetAction* pThis = static_cast<InetAction*>(callback_arg);
	if (ipaddr) {
		pThis->addrSNTP.addr = ipaddr->addr;
		pThis->isDNSFinished = 1;
	} else {
		pThis->isDNSFinished = 2; // DNS名前解決が失敗したことを示す
	}
}

bool InetAction::getHostByName(const char* hostname, int timeout_ms)
{
	isDNSFinished = 0; // DNS名前解決の完了フラグを初期化
	ip4_addr_t addr;

	err_t err = dns_gethostbyname(hostname, &addr, dns_callback, this);
	if (err == ERR_OK) { // キャッシュヒット
		addrSNTP.addr = addr.addr; // キャッシュから取得したIPアドレスを設定
		return true;
	} else if (err != ERR_INPROGRESS) {
		pTft->printf("DNS ERROR: %s, (%d)\n", hostname, err);
		return false; // 初期化に失敗した場合はfalseを返す
	} else {
		while (timeout_ms > 0) {
			sleep_ms(100);            // 100ミリ秒待機
			cyw43_arch_poll();        // これがないとコールバックが呼ばれません
			if (isDNSFinished == 1) { // DNS名前解決が成功した場合
				return true;
			} else if (isDNSFinished == 2) {
				pTft->printf("UNKNOWN ERROR: %s\n", hostname);
				return false; // DNS名前解決が失敗した場合
			} 
			timeout_ms -= 100; // タイムアウト時間を減らす
		}
		pTft->printf("DNS TIMEOUT: %s\n", hostname);
		return false;
	}
}

extern "C" void set_system_time(u32_t sec) {
    time_t epoch = sec;
    struct tm* t = gmtime(&epoch);
    datetime_t dt = {
        .year = (int16_t)(t->tm_year + 1900),
        .month = (int8_t)(t->tm_mon + 1),
        .day = (int8_t)t->tm_mday,
        .dotw = (int8_t)t->tm_wday,
        .hour = (int8_t)t->tm_hour,
        .min = (int8_t)t->tm_min,
        .sec = (int8_t)t->tm_sec
    };
    rtc_set_datetime(&dt);
}

bool InetAction::setTime()
{
	//__setSystemTime(0);
	pTft->printf("Setting time from SNTP ... ");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);

	// 不定期にエラーが戻るので、３回リトライする
	bool bRet;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 10;j++) {
			sleep_ms(10);
		    cyw43_arch_poll();
        }
		bRet = getHostByName(this->sntp_server_address, 50000);
		if (bRet) break; // 成功したらループを抜ける
	}
	if (!bRet) {
		pTft->printf("Failed.\nSNTP ADDR::%s\n", this->sntp_server_address);
		return false;
	}
    //sntp_setoperatingmode(SNTP_OPMODE_POLL); // Pollingモードに設定
	sntp_setserver(0, (const ip_addr_t*)&addrSNTP); // SNTPサーバーのIPアドレスを設定
	sntp_init();

	for (int i = 0; i < 10;i++) {
        sleep_ms(1000);
    }
		/*

		// SNTP同期を待つ（最大10秒）
		for (int i = 0; i < 10; i++) {
			sleep_ms(1000);
			cyw43_arch_poll();

			// lwIPのsntp_get_sync_status()が使える場合はこちらを優先
	#ifdef SNTP_SYNC_STATUS_COMPLETED
			if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
				pTft->printf("Success!\n");
				return true;
			}
	#else
			time_t now = time(NULL);
			if (now > 1609459200) { // 2021-01-01 00:00:00 UTC より大きければOK
				pTft->printf("Success!\n");
				struct tm* tm_info = localtime(&now);
				char buf[32];
				strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
				pTft->printf("Time: %s\n", buf);
				return true;
			}
	#endif
		}
		*/
	pTft->printf("Failed.\n");
	sntp_stop();

	return false;
}
