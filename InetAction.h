#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "settings.h"
#include "lwip/ip_addr.h"
#include "pico/stdlib.h"
#include "hardware/rtc.h"




using namespace ardPort;
using namespace ardPort::spi;

class InetAction
{
  public:
	Adafruit_ILI9341* pTft; // TFTディスプレイへのポインタ
	Settings& settings;     // 設定情報への参照

	public:
	  uint8_t isDNSFinished;   // 1...成功により終了、2...失敗により終了
	  ip4_addr_t addrSNTP;

	public:
	  InetAction(Settings& settings, Adafruit_ILI9341* a_pTft);

	  bool init();
	  int connect();
	  bool setTime();
	  bool getHostByName(const char* hostname,  int timeout_ms);

	private:
	  // const char* sntp_server_address = "0.pool.ntp.org";
	  const char* sntp_server_address = "pool.ntp.org";

	public:
	  void setSntpServer(const char* addr) { sntp_server_address = addr; }
	  const char* getSntpServer() const { return sntp_server_address; }
};

#ifdef __cplusplus
extern "C" {
#endif
void set_system_time(u32_t sec);
#ifdef __cplusplus
}
#endif