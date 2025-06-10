#include <stdio.h>

#include "KanjiHelper.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "AS3935.h"
// ライブラリのヘッダファイル　　　　　　　　＜＜＜＜＜　追加　　（１）　
#include "KanjiHelper.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"

// 漢字フォント情報ファイル　　　　　　　　　＜＜＜＜＜　追加　　（４）　
#include "Kanji/Fonts/JF-Dot-Shinonome16_16x16_ALL.inc"
#include "pictData.h" // 画像データ
#include "FlashMem.h"
#include "Settings.h"
#include "InetAction.h"
#include "DispClock.h"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/sync.h" // WFE（Wait For Event）を扱うためのライブラリ

// 名前空間の使用を宣言　　　　　　　　　　　　＜＜＜＜＜　追加　（２）
using namespace ardPort;
using namespace ardPort::spi;

// 使用するポートの定義　　　　　　　　　　　　＜＜＜＜＜　追加　（３）
#define TFT_SCK 18  // 液晶表示の SCK
#define TFT_MOSI 19 // 液晶表示の MOSI
#define TFT_DC 20   // 液晶画面の DC
#define TFT_RST 21  // 液晶画面の RST
#define TFT_CS 22   // 液晶画面の CS

// I2Cのポート番号とピン番号の定義
#define I2C_PORT 1 // I2Cのポート番号として０
#define I2C_SDA 14 // I2CのSDAピン番号としてGP14
#define I2C_SCL 15 // I2CのSCLピン番号としてGP15
// AS3935からのIRQ入力
#define AS3935_IRQ 13 // AS3935のIRQピン GP13
// AS3935のアドレス。ボードにより0か３のどちらか
#define AS3935_ADDRESS 0

Settings settings;

/// @brief 	Wi-Fi接続関数
/// @param ipAddr 		IPアドレスを格納する配列（4バイト）
/// @param ssid 		接続するWi-FiのSSID
/// @param password 	接続するWi-Fiのパスワード
/// @return 	接続が成功した場合は0、失敗した場合はエラーコードを返す
int ConnectToWifi(uint8_t* ipAddr, const char* ssid, const char* password)
{
	ipAddr[0] = ipAddr[1] = ipAddr[2] = ipAddr[3] = 0; // IPアドレスの初期化
	int ret = cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA3_WPA2_AES_PSK, 5000000);
	if (ret != 0) {
		return ret;
	}
	uint8_t* ip_addr = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
	ipAddr[0] = ip_addr[0];
	ipAddr[1] = ip_addr[1];
	ipAddr[2] = ip_addr[2];
	ipAddr[3] = ip_addr[3];
	return true;
}

/// @brief 	DMAの初期化関数
/// @return 	初期化が成功した場合はtrue、失敗した場合はfalseを返す
bool InitDMA()
{

	// Get a free channel, panic() if there are none
	// Data will be copied from src to dst
	const char src[] = "DMA SOURCE BUFFER";
	char dst[count_of(src)];

	int chan = dma_claim_unused_channel(true);

	// 8 bit transfers. Both read and write address increment after each
	// transfer (each pointing to a location in src or dst respectively).
	// No DREQ is selected, so the DMA transfers as fast as it can.

	dma_channel_config c = dma_channel_get_default_config(chan);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_read_increment(&c, true);
	channel_config_set_write_increment(&c, true);

	dma_channel_configure(
		chan,          // Channel to be configured
		&c,            // The configuration we just created
		dst,           // The initial write address
		src,           // The initial read address
		count_of(src), // Number of transfers; in this case each is 1 byte.
		true           // Start immediately.
	);

	// We could choose to go and do something else whilst the DMA is doing its
	// thing. In this case the processor has nothing else to do, so we just
	// wait for the DMA to finish.
	dma_channel_wait_for_finish_blocking(chan);

	// The DMA has now copied our text from the transmit buffer (src) to the
	// receive buffer (dst), so we can print it out from there.
	puts(dst);
	bool RetVal = (strcmp(src, dst) == 0) ? true : false;

	return RetVal;
}
bool isIRQTriggered = false; // IRQがトリガーされたかどうかのフラグ
/// @brief IRQピンの割り込みに対するコールバック関数
/// @param gpio
/// @param events
static void as3935IRQCallback(uint gpio, uint32_t events)
{
	isIRQTriggered = true; // IRQがトリガーされたことを記録
}
/// @brief 	タイマー割り込みのコールバック関数
/// @param rt　	タイマーのリピート割り込み構造体
/// @return　割り込みを継続するかどうか
bool hartbeatCallback(repeating_timer_t* rt)
{
	return true; // 割り込みを継続
}




/**
 * @brief メイン関数（エントリーポイント）
 * @details
 * Raspberry Pi Pico上でAS3935雷センサとILI9341 TFTディスプレイを制御するアプリケーションのメイン関数です。
 * - TFTディスプレイ、I2C、DMA、Wi-Fiの初期化を行います。
 * - AS3935の初期化とキャリブレーションを実行します。
 * - Wi-Fi接続やIPアドレスの取得、ステーションモードの有効化を行います（有効時）。
 * - メインループでは、ディスプレイに日本語メッセージと経過時間を表示し続けます。
 * - 各種初期化やエラー時にはTFTに進捗やエラー内容を表示します。
 *
 *
 * @return 終了コード（正常終了時は0、初期化失敗時は-1）
 */

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

int main()
 {

	 FlashMem flash(31, 1); // フラッシュメモリのブロック0を管理するインスタンスを作成

	 if (flash.read(0, (void*)(&settings), sizeof(Settings)) == false) {
		 settings.setDefault();                                // チャンク識別子が一致しない場合はデフォルト値を設定
		 flash.write(0, (void*)(&settings), sizeof(Settings)); // デフォルト値を書き込む
	 } else {
		 if (settings.isActive() == false) {
			 settings.setDefault();                                // チャンク識別子が一致しない場合はデフォルト値を設定
			 flash.write(0, (void*)(&settings), sizeof(Settings)); // デフォルト値を書き込む
		 }
	}
	InetAction iNet(settings, &tft); // InetActionのインスタンスを作成

	stdio_init_all();
	// ILI9341ディスプレイのインスタンスを作成　　＜＜＜＜＜　追加　（３）
	SPI.setTX(TFT_MOSI); // SPI0のTX(MOSI)
	SPI.setSCK(TFT_SCK); // SPI0のSCK
	tft.begin();         // TFTを初期

	AS3935 as3935(&tft); // AS3935のインスタンスを作成

	// 漢字フォントの設定　　　　　　　　　　　　　＜＜＜＜＜　追加　（４）　
	tft.setFont(JFDotShinonome16_16x16_ALL, JFDotShinonome16_16x16_ALL_bitmap);

	// 文字の表示を高速化させる　　　　　　　　　　＜＜＜＜＜　追加　（４）　
	tft.useWindowMode(true);

	// 画面を黒で塗りつぶす　　　　　　　　　　　　＜＜＜＜＜　追加　（５）　
	tft.fillScreen(ILI9341_BLACK);

	tft.setCursor(0, 0);
	tft.printf("Start initializing...\n");

	// --- I2C初期化とAS3935のIRQピン設定 ---
	{
		tft.printf("Initializing i2C ... ");
		bool bRet = as3935.Init(AS3935_ADDRESS, I2C_PORT, I2C_SDA, I2C_SCL, AS3935_IRQ); // AS3935の初期化関数を呼び出す
		if (bRet) {
			tft.printf("Success!\n");
		} else {
			tft.printf("Failed.\n");
			return -1; // 初期化に失敗した場合は終了
		}
		gpio_init(AS3935_IRQ);
		gpio_set_dir(AS3935_IRQ, GPIO_IN);
	}



	// --- Wi-Fi初期化・接続・時刻同期 ---
	if (settings.isEnableWifi) {
		tft.printf("Initializing Wifi ... ");
		// Wi-Fiチップ初期化
		if (iNet.init() == false) { 
			// 初期化失敗時はWi-Fi無効で継続
			settings.isEnableWifi = false;
		} else {
			// Wi-Fi接続処理
			int iRet = iNet.connect();
			if (iRet != 0) {
				settings.isEnableWifi = false;
				tft.drawRGBBitmap(1, 240 - 16, wifiIcon_NG, 16, 16, STDCOLOR.BLACK);
			} 
		}
		tft.drawRGBBitmap(1, 240 - 16, wifiIcon_OK, 16, 16, STDCOLOR.BLACK);
		// SNTPで現在時刻を取得
		iNet.setTime();
	}
	// -- 時計表示部の初期化 --
	{
		DispClock::init(&tft,settings.isClock24Hour);
	}
	// --- DMA初期化 ---
	{
		tft.printf("Initializing DMA ... ");
		bool bRet = InitDMA(); // DMAの初期化関数を呼び出す
		if (bRet) {
			tft.printf("Success!\n");
		} else {
			tft.printf("Failed.\n");
			return -1; // 初期化に失敗した場合は終了
		}
	}

	// --- AS3935の初期化・キャリブレーション ---
	{
		tft.printf("Calibrating AS3935\n");
		as3935.StartCalibration(100); // AS3935のキャリブレーションを実行
		tft.printf("Done.\n");
	}

	delay(1000);
	// --- 画面初期化・タイトル表示 ---
	tft.fillScreen(STDCOLOR.SUPERDARK_GRAY); // 画面を暗い灰色で塗りつぶす
	tft.setCursor(0, 2);
	tft.fillRoundRect(0, 0, 240, 20, 4, STDCOLOR.DARK_GRAY); // 画面の上部に帯を描画
	tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.WHITE);
	tft.printf("Lightning Sensor\n");
	tft.setCursor(0, 22);
	if (settings.isEnableWifi) {
		tft.drawRGBBitmap(240 - 16,2, wifiIcon_OK, 16, 16, STDCOLOR.BLACK);
	} else {
		tft.drawRGBBitmap(240 - 16,2, wifiIcon_NG, 16, 16, STDCOLOR.BLACK);
	}

	// --- 割り込み・タイマー・メインループ ---
	// AS3935のIRQ割り込みを有効化
	gpio_set_irq_enabled_with_callback(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, true, &as3935IRQCallback);
	// 1秒ごとのハートビートタイマーを設定
	repeating_timer_t timer;
	add_repeating_timer_ms(500, hartbeatCallback, NULL, &timer);

	while (true) {
		isIRQTriggered = false;
		__wfe();              // 割り込みが発生するまでスリープ
		if (isIRQTriggered) { // IRQがトリガーされた場合
			uint8_t u8Summary;
			uint8_t u8Distance;
			uint8_t u8Status;
			time_t eventtime;

			tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.BLUE);
			// 雷信号の検証
			bool bRet = as3935.validateSignal();
			if (bRet) { // 雷が検出された場合
				tft.setTextColor(STDCOLOR.RED, STDCOLOR.SUPERDARK_GRAY);
				tft.setCursor(0, 100);
				tft.printf("雷を検出しました！\n");
				as3935.GetLatestAlarm(0, u8Summary, u8Distance, u8Status,eventtime);
				tft.printf("距離:%3d km STAT：%s(%02X)", u8Distance, as3935.GetAlarmSummaryString(u8Summary), u8Status);
			} else { // 雷が検出されなかった場合
				tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.SUPERDARK_GRAY);
				tft.setCursor(0, 100);
				tft.printf("雷は検出されませんでした。\n");
				as3935.GetLatestFalseAlarm(0, u8Summary, u8Distance, u8Status,eventtime);
				tft.printf("距離:%3d km STAT:%s(%02X)", u8Distance, as3935.GetAlarmSummaryString(u8Summary), u8Status);
			}
			tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.SUPERDARK_GRAY);

			// 最新から5個のアラームをアイコンで表示
			tft.setCursor(0, 150);

			for (int i = 0; i < 10; i++) {
				int16_t curY = tft.getCursorY();
				bool bRet = as3935.GetLatestEvent(i, u8Summary, u8Distance, u8Status, eventtime);
				if (bRet == false) {
					break; // 取得できなかったら終了
				}
				struct tm* t = localtime(&eventtime);
				int hour = t->tm_hour;
				int min = t->tm_min;
				if  (u8Summary == as3935.SUMM_THUNDER) {
					tft.drawRGBBitmap((int16_t)0, curY, picThndr, 15, 15, STDCOLOR.BLACK);
					tft.setCursor(24, curY);
					tft.printf("%02d:%02d  %3d km\n", hour, min, u8Distance);
				} else {
					tft.drawRGBBitmap((int16_t)0, curY, picFalse, 15, 15, STDCOLOR.BLACK);
					tft.printf("%02d:%02d  --- km\n", hour, min, u8Distance);
				}
			}
			// マークを消して、元の状態に戻す
			tft.fillRect(0, 320 - 20, 16, 16, STDCOLOR.SUPERDARK_GRAY);
		} else {
			// それ以外の処理があればここに追加
			DispClock::show(32,30); // 時計の更新
		}
	}
}
