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

/**
 * @brief InetActionクラスのコンストラクタ
 * @details Wi-FiやSNTPなどのネットワーク関連処理の初期化に必要な情報を受け取ります。
 */
InetAction::InetAction(Settings& a_settings, Adafruit_ILI9341* a_pTft) :
	pTft(a_pTft),
	settings(a_settings)
{
}

/**
 * @brief Wi-Fiチップの初期化
 * @details cyw43_arch_init()でWi-Fiチップを初期化し、ステーションモードを有効化します。
 * @retval true 初期化成功
 * @retval false 初期化失敗
 */
bool InetAction::init()
{
	// Wi-Fiチップの初期化
	if (cyw43_arch_init()) {
		pTft->printf("cyw43_arch_init failed\n");
		return false;
	} else {
		pTft->printf("Success!\n");
	}
	// ステーションモード有効化
	pTft->printf("Enable Wifi on station mode\n");
	cyw43_arch_enable_sta_mode();
	return true;
}

/**
 * @brief Wi-Fiアクセスポイントへ接続
 * @details SSID/PASSWORDはsettingsから取得。接続成功時はIPアドレスを設定。
 * @retval 0 接続成功
 * @retval 負値またはエラーコード 接続失敗
 */
int InetAction::connect()
{
	pTft->printf("Connect Wi-Fi ... ");
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

/**
 * @brief DNS名前解決のコールバック関数
 * @details dns_gethostbyname()の非同期結果を受け取り、成功時はIPアドレスを保存します。
 */
void dns_callback(const char* name, const ip_addr_t* ipaddr, void* callback_arg)
{
	InetAction* pThis = static_cast<InetAction*>(callback_arg);
	if (ipaddr) {
		pThis->addrSNTP.addr = ipaddr->addr;
		pThis->isDNSFinished = 1;
	} else {
		pThis->isDNSFinished = 2; // DNS名前解決が失敗
	}
}

/**
 * @brief ホスト名からIPアドレスを取得
 * @details dns_gethostbyname()を使い、非同期の場合はタイムアウト付きで待機します。
 * @param hostname 解決したいホスト名
 * @param timeout_ms タイムアウト（ミリ秒）
 * @retval true 成功
 * @retval false 失敗
 */
bool InetAction::getHostByName(const char* hostname, int timeout_ms)
{
	isDNSFinished = 0; // DNS名前解決の完了フラグを初期化
	ip4_addr_t addr;

	err_t err = dns_gethostbyname(hostname, &addr, dns_callback, this);
	if (err == ERR_OK) {           // キャッシュヒット
		addrSNTP.addr = addr.addr; // キャッシュから取得したIPアドレスを設定
		return true;
	} else if (err != ERR_INPROGRESS) {
		pTft->printf("DNS ERROR: %s, (%d)\n", hostname, err);
		return false; // 初期化に失敗
	} else {
		// 非同期の場合はポーリングで待機
		while (timeout_ms > 0) {
			sleep_ms(100);            // 100ミリ秒待機
			cyw43_arch_poll();        // lwIPイベント処理
			if (isDNSFinished == 1) { // 成功
				return true;
			} else if (isDNSFinished == 2) {
				pTft->printf("UNKNOWN ERROR: %s\n", hostname);
				return false; // 失敗
			}
			timeout_ms -= 100;
		}
		pTft->printf("DNS TIMEOUT: %s\n", hostname);
		return false;
	}
}

#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)


/// @brief NTP の状態を保持する構造体。コールバック関数とクラス本体の間でやり取りするために使われる
typedef struct NTP_T_ {
	uint8_t status;							// NTPの状態 0...取得中 1...成功 2...失敗 3...タイムアウト
	ip_addr_t ntp_server_address;			// NTPサーバーのIPアドレス
//	bool dns_request_sent;					// DNSリクエストが送信されたかどうか
	struct udp_pcb* ntp_pcb;				// NTP用のUDPプロトコル制御ブロック
	absolute_time_t ntp_test_time;
	alarm_id_t ntp_resend_alarm;			// NTPリクエストのタイムアウトアラームのハンドル
} NTP_T;

extern "C" void setRTC(u32_t sec)
{
	time_t epoch = sec;
	struct tm* t = gmtime(&epoch);
	datetime_t dt = {
		.year = (int16_t)(t->tm_year + 1900),
		.month = (int8_t)(t->tm_mon + 1),
		.day = (int8_t)t->tm_mday,
		.dotw = (int8_t)t->tm_wday,
		.hour = (int8_t)t->tm_hour,
		.min = (int8_t)t->tm_min,
		.sec = (int8_t)t->tm_sec};
	bool bRet = rtc_set_datetime(&dt);
	if (bRet == false) {
		printf("rtc_set_datetime failed\n");
	} else {
		printf("rtc_set_datetime success: %04d/%02d/%02d %02d:%02d:%02d\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
		time_t now;
		rtc_get_datetime(&dt);
		struct tm t;
		t.tm_year = dt.year - 1900;
		t.tm_mon = dt.month - 1;
		t.tm_mday = dt.day;
		t.tm_hour = dt.hour;
		t.tm_min = dt.min;
		t.tm_sec = dt.sec;
		now = mktime(&t);
		struct timeval tv;
		tv.tv_sec = now;
		tv.tv_usec = 0;
		settimeofday(&tv, NULL);
		setlocale(LC_TIME, "ja_JP.UTF-8");
	}
}

extern "C" int64_t ntp_failed_handler(alarm_id_t id, void* user_data)
{
	NTP_T* state = (NTP_T*)user_data;
	if (state->ntp_resend_alarm > 0) {
		cancel_alarm(state->ntp_resend_alarm);
		state->ntp_resend_alarm = 0;
	}
	state->status = 3;
	return 0;
}


extern "C" void ntp_recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port)
{
	NTP_T* state = (NTP_T*)arg;
	uint8_t mode = pbuf_get_at(p, 0) & 0x7;
	uint8_t stratum = pbuf_get_at(p, 1);

	// Check the result
	if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
		mode == 0x4 && stratum != 0) {
		uint8_t seconds_buf[4] = {0};
		pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
		uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
		uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
		time_t epoch = seconds_since_1970;

		//結果の表示など
		struct tm* utc = gmtime(&epoch);
//		printf("got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday,  utc->tm_mon + 1, utc->tm_year + 1900, utc->tm_hour, utc->tm_min, utc->tm_sec);
		// 成功によるタイマーのキャンセル
		if (state->ntp_resend_alarm > 0) {
			cancel_alarm(state->ntp_resend_alarm);
			state->ntp_resend_alarm = 0;
		}
		setRTC(seconds_since_1970);
		state->status = 1; // 成功
	} else {  // 失敗
//		printf("invalid ntp response\n");
		// 失敗によるタイマーのキャンセル
		if (state->ntp_resend_alarm > 0) {
			cancel_alarm(state->ntp_resend_alarm);
			state->ntp_resend_alarm = 0;
		}
		state->status = 2; // 成功
	}
	pbuf_free(p);
}

/**
 * @brief SNTPサーバーから時刻を取得しRTCに設定
 * @details DNSでSNTPサーバーのIPを取得し、UDPでNTPリクエストを送信。応答を受信してRTCを更新します。
 * @retval true 成功
 * @retval false 失敗
 */
bool InetAction::setTime()
{
	pTft->printf("SNTP ... ");

	// DNSでSNTPサーバーのIPアドレスを取得。最大3回リトライ
	bool bRet;
	for (int i = 0; i < 3; i++) {
		bRet = getHostByName(this->sntp_server_address, 50000);
		if (bRet) break;
		for (int j = 0; j < 100; j++) {
			sleep_ms(10);
			cyw43_arch_poll();
		}
	}
	if (!bRet) {
		pTft->printf("DNS Failed.");
		return false;
	}
	// SNTPサーバーのIPアドレスを取得できたので、NTPのリクエストを送信
	NTP_T* state = (NTP_T*)calloc(1, sizeof(NTP_T));
	if (!state) {
		pTft->printf("SNTP Failed.\nFailed to acllocate state.");
		return false;
	}
	state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	if (!state->ntp_pcb) {
		pTft->printf("SNTP Failed.\nFailed to create PCB.");
		free(state);
		return false;
	}
	state->ntp_server_address = this->addrSNTP; // DNSで取得したIPアドレスを設定

    // UDP受信コールバックを設定
	udp_recv(state->ntp_pcb, ntp_recv, state);

	// タイムアウト用アラームをセット
	state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);
	state->status = 0;
	// NTPリクエスト送信
	cyw43_arch_lwip_begin();
	struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
	uint8_t* req = (uint8_t*)p->payload;
	memset(req, 0, NTP_MSG_LEN);
	req[0] = 0x1b;
	udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
	pbuf_free(p);
	cyw43_arch_lwip_end();

    // 応答待ちループ
	while (true) {
		if (state->status != 0) {
			break;
		}
		sleep_ms(100);
		cyw43_arch_poll();
		cyw43_arch_wait_for_work_until(state->ntp_test_time);
	}
	if (state->status == 1) { // 成功
		datetime_t dt;
		bool bRet = rtc_get_datetime(&dt);
		pTft->printf("Success!\n");
		time_t now = time(NULL);
		setlocale(LC_TIME, settings.getLocaleStr());
		setenv("TZ", settings.getTzStr(), 1);
		tzset();
		struct tm* local = localtime(&now);

		pTft->printf("Time: %d-%02d-%02d %02d:%02d:%02d\n",
					 local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
					 local->tm_hour, local->tm_min, local->tm_sec);
		return true;
	} else if (state->status == 2) { // 失敗
		pTft->printf("SNTP Failed.\nFailed to get Info from SNTP");
		return false;
	} else if (state->status == 3) { // タイムアウト
		pTft->printf("SNTP Failed.\nSNTP Timeout");
		return false;
	}
	pTft->printf("Failed.\nUnknown Error");
	return false;
}
