#include <stdio.h>

#include "KanjiHelper.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"

#include "AS3935.h"
// ライブラリのヘッダファイル　　　　　　　　＜＜＜＜＜　追加　　（１）　
#include "KanjiHelper.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
// 漢字フォント情報ファイル　　　　　　　　　＜＜＜＜＜　追加　　（４）　
#include "Kanji/Fonts/JF-Dot-Shinonome16_16x16_ALL.inc"
#include "pictData.h" // 画像データ
#include "FlashMem.h"
#include "Settings.h"
#include "InetAction.h"
#include "DispClock.h"
#include "TouchCalibration.h"
#include "GUIMsgBox.h"

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/sync.h" // WFE（Wait For Event）を扱うためのライブラリ

#include "printfDebug.h"

// 名前空間の使用を宣言　　　　　　　　　　　　＜＜＜＜＜　追加　（２）
using namespace ardPort;
using namespace ardPort::spi;

// 使用するポートの定義　　　　　　　　　　　　＜＜＜＜＜　追加　（３）
#define TFT_SCK 18  // 液晶表示の SCK
#define TFT_MOSI 19 // 液晶表示の MOSI
#define TFT_DC 20   // 液晶画面の DC
#define TFT_RST 21  // 液晶画面の RST
#define TFT_CS 22   // 液晶画面の CS

#define TOUCH_MISO 16 // タッチパネルの MISO
#define TOUCH_CS 17   // タッチパネルの CS

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

enum APPMode {
	APP_MODE_STARTING, // 通常モード
	APP_MODE_NORMAL,   // キャリブレーションモード
	APP_MODE_SETTING,  // エラーモード
} appMode;
bool mustRedraw = false;
// 雷センサーの画面表示
void mainDisplay(Adafruit_ILI9341& tft, AS3935& as3935, bool isSignal, bool isBanner, bool isClock, bool isBody)
{
	if (mustRedraw) {
		isBanner = true;
		isClock = true;
		isBody = true;
		isSignal = false;   // 強制再描画の場合、信号検出は行わない。IRQがトリガーされていないのにvalidateSignalを呼びだしてレジスタアクセスしないようにするため。
		mustRedraw = false; // 再描画フラグをリセット
	}
	if (isBanner) {
		tft.fillScreen(STDCOLOR.SUPERDARK_GRAY); // 画面を暗い灰色で塗りつぶす
		tft.setCursor(0, 2);
		tft.fillRoundRect(0, 0, 240, 20, 4, STDCOLOR.DARK_GRAY); // 画面の上部に帯を描画
		tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.WHITE);
		tft.printf("Lightning Sensor\n");
		tft.setCursor(0, 22);
		if (settings.getIsEnableWifi()) {
			tft.drawRGBBitmap(240 - 16, 2, wifiIcon_OK, 16, 16, STDCOLOR.BLACK);
		} else {
			tft.drawRGBBitmap(240 - 16, 2, wifiIcon_NG, 16, 16, STDCOLOR.BLACK);
		}
	}
	if (isBody) {
		uint8_t u8Summary;
		uint8_t u8Distance;
		long lEnergy;
		time_t eventtime;

		// 最新情報表示エリアをクリア
		tft.fillRect(0, 100, 240, 32, STDCOLOR.SUPERDARK_GRAY); // 前のメッセージを消す
		tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.SUPERDARK_GRAY);
		tft.setCursor(0, 100);

		// 雷信号の検証
		AS3935_SIGNAL sigValid;
		if (isSignal) {
			sleep_ms(2); // 300ミリ秒待機してから信号を検証
			sigValid = as3935.validateSignal();
			time_t tm = time(NULL);
			struct tm* t = localtime(&tm);
			// 　こちらは信号を検出したことに伴うもの
			if (sigValid == AS3935_SIGNAL::VALID) {
				tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.RED); // 検出された場合は黄色の丸を表示
			} else if (sigValid == AS3935_SIGNAL::INVALID) {
				tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.YELLOW); // 無効な信号の場合は赤色の丸を表示
			} else if (sigValid == AS3935_SIGNAL::STATCLEAR) {
				tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.GREEN); // 信号がない場合は青色の丸を表示
			} else {
				tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.DARK_BLUE); // その他の場合は灰色の丸を表示
			}

			if (sigValid == AS3935_SIGNAL::VALID || sigValid == AS3935_SIGNAL::INVALID) { // 雷が検出された場合
				tft.printf("%02d/%02d %02d:%02d:%02d %s %s\n", t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (sigValid == AS3935_SIGNAL::VALID) ? "検出" : "ーー", as3935.getLatestSummaryStr());
				tft.printf("距離:%3d km 強さ:%d\n", as3935.getLatestDist(), as3935.getLatestEnergy());
			} else {
				tft.fillRoundRect(0, 320 - 20, 16, 16, 2, STDCOLOR.BLUE);
				tft.fillRect(0, 100, 240, 32, STDCOLOR.SUPERDARK_GRAY); // 前のメッセージを消す
				tft.setTextColor(STDCOLOR.GRAY, STDCOLOR.SUPERDARK_GRAY);
				tft.setCursor(0, 100);
			}
		} else {
			// こちらは画面再描画に伴うもの
			AS3935_SIGNAL sigValid = as3935.getLatestSignalValid();
			if (sigValid == AS3935_SIGNAL::VALID || sigValid == AS3935_SIGNAL::INVALID) { // 雷が検出された場合
				time_t tm = as3935.getLatestDateTime();
				struct tm* t = localtime(&tm);
				tft.printf("%02d/%02d %02d:%02d:%02d %s %s\n", t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (sigValid == AS3935_SIGNAL::VALID) ? "検出" : "ーー", as3935.getLatestSummaryStr());
				tft.printf("距離:%3d km 強さ:%d\n", as3935.getLatestDist(), as3935.getLatestEnergy());
			} else {
				tft.fillRect(0, 100, 240, 32, STDCOLOR.SUPERDARK_GRAY); // 前のメッセージを消す
				tft.setTextColor(STDCOLOR.GRAY, STDCOLOR.SUPERDARK_GRAY);
				tft.setCursor(0, 100);
			}
			as3935.GetLatestAlarm(0, u8Summary, u8Distance, lEnergy, eventtime);
		}
		tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.SUPERDARK_GRAY);

		// 最新から5個のアラームをアイコンで表示
		tft.setCursor(0, 150);

		for (int i = 0; i < 8; i++) {
			int16_t curY = tft.getCursorY();
			bool bRet = as3935.GetLatestEvent(i, u8Summary, u8Distance, lEnergy, eventtime);
			if (bRet == false) {
				break; // 取得できなかったら終了
			}
			struct tm* t = localtime(&eventtime);
			int hour = t->tm_hour;
			int min = t->tm_min;
			if (u8Summary == as3935.SUMM_THUNDER) {
				tft.drawRGBBitmap((int16_t)0, curY, picThndr, 15, 15, STDCOLOR.BLACK);
				tft.setCursor(24, curY);
				tft.printf("%02d:%02d  %3d km 強さ %d\n", hour, min, u8Distance, lEnergy);
			} else {
				tft.drawRGBBitmap((int16_t)0, curY, picFalse, 15, 15, STDCOLOR.BLACK);
				tft.setCursor(24, curY);
				tft.printf("%02d:%02d  --- km\n", hour, min, u8Distance);
			}
		}
		// マークを消して、元の状態に戻す
		tft.fillRect(0, 320 - 20, 16, 16, STDCOLOR.SUPERDARK_GRAY);
	}

	if (isClock) {
		// それ以外の処理があればここに追加
		DispClock::show(32, 30); // 時計の更新
	}
}

/**
 * @brief アプリケーションの初期化処理
 * @details
 * - TFTディスプレイ、I2C、Wi-Fi、DMA、AS3935キャリブレーション等の初期化を行う。
 * - 各初期化ステップの進捗やエラーをTFTに表示。
 * - Wi-Fi有効時はSNTPによる時刻同期も実施。
 * - タッチパネルのタッチまたはタイムアウトまで待機し、初期化完了後にメイン画面へ遷移。
 *
 * @param tft    ILI9341 TFTディスプレイインスタンス
 * @param ts     XPT2046タッチパネルインスタンス
 * @param as3935 AS3935雷センサインスタンス
 * @param iNet   InetActionインスタンス（Wi-Fi/時刻同期用）
 * @retval なし
 */
void Initialize(Adafruit_ILI9341& tft, XPT2046_Touchscreen& ts, AS3935& as3935 , InetAction& iNet)
{
	// 画面を黒で塗りつぶす　　　　　　　　　　　　＜＜＜＜＜　追加　（５）　
	tft.fillScreen(ILI9341_BLACK);

	tft.fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	tft.setCursor(10, 4);
	tft.printf("Initializing");

	tft.setCursor(0, 20);
	tft.setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	bool isI2cInitialized = false; ///< I2Cの初期化フラグ
	// --- I2C初期化とAS3935のIRQピン設定 ---
	{
		tft.printlocf(0, 20, "I2C");
		bool bRet = as3935.Init(settings.value.i2cAddr, I2C_PORT, I2C_SDA, I2C_SCL, AS3935_IRQ); ///< AS3935の初期化
		if (bRet) {
			tft.printlocf(200, 20, "〇\n");
			isI2cInitialized = true; ///< I2C初期化成功
		} else {
			tft.printlocf(200, 20, "×\n");
			sleep_ms(10000);          ///< 失敗時は10秒停止
			isI2cInitialized = false; ///< I2C初期化失敗
		}
		gpio_init(AS3935_IRQ); ///< AS3935 IRQピン初期化
		gpio_set_dir(AS3935_IRQ, GPIO_IN); ///< IRQピンを入力に設定
	}

	// --- Wi-Fi初期化・接続・時刻同期 ---
	if (settings.getIsEnableWifi()) {
		tft.printlocf(0, 40, "WIFI INIT");
		// Wi-Fiチップ初期化
		if (iNet.init() == false) {
			settings.setIsEnableWifi(false); ///< エラー時もWi-Fi設定は維持
			tft.printlocf(200, 40, "×\n");
		} else {
			tft.printlocf(200, 40, "〇\n");

			// Wi-Fi接続処理
			tft.printlocf(0, 60, "WIFI CONNECT");
			int iRet = iNet.connect(); ///< Wi-Fi接続
			if (iRet != 0) {
				tft.printlocf(200, 60, "×");
				tft.printlocf(10, 80, "Err:%s", iNet.getConLasterror()); ///< エラー表示
				settings.setIsEnableWifi(false);
			} else {
				tft.printlocf(200, 60, "〇");
				tft.printlocf(10, 80, "IP: %s", settings.getIpString()); ///< IPアドレス表示
			}
		}
		// SNTPで現在時刻を取得
		tft.printlocf(0, 100, "SNTP");
		bool bRet = iNet.setTime(); ///< SNTP時刻同期
		if (bRet) {
			tft.printlocf(200, 100, "〇");
			time_t now = time(NULL);
			struct tm* local = localtime(&now);
			char timeStr[32];
			sprintf(timeStr, "%d-%02d-%02d %02d:%02d:%02d\n", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
			tft.printlocf(10, 120, "Time:%s", timeStr); ///< 現在時刻表示
		} else {
			tft.printlocf(200, 100, "×");
			tft.printlocf(10, 120, "Err:%s", iNet.getSntpLasterror()); ///< SNTPエラー表示
		}
	}
	// -- 時計表示部の初期化 --
	{
		DispClock::init(&tft, settings.getIsClock24Hour()); ///< 時計初期化
	}
	// --- DMA初期化 ---
	{
		tft.printlocf(0, 140, "DMA");
		bool bRet = InitDMA(); ///< DMA初期化
		if (bRet) {
			tft.printlocf(200, 140, "〇");
		} else {
			tft.printlocf(200, 140, "×");
		}
	}

	// --- AS3935の初期化・キャリブレーション。I２C初期化エラーのときはやらない ---
	if (isI2cInitialized) {
		tft.printlocf(0, 160, "CALIB AS3935");
		as3935.StartCalibration(100); ///< キャリブレーション実行
		tft.printlocf(200, 160, "〇");
		tft.printlocf(10, 180, "Freq:%4.1fKHz at %3dpF", ((float)as3935.getFreqCalibration() / 1000), as3935.getCalibratedCap() * 8); ///< キャリブ値表示
	}

	// タッチされるか、時間が過ぎるのを待つ。
	{
		tft.printlocf(0, 300, "Initialize done."); ///< 初期化完了表示
		int timeOut = 0; ///< タイムアウトカウンタ
		if (settings.isSerialDebug()) {
			timeOut = 300; ///< シリアルデバッグ時は30秒
		} else {
			timeOut = 10; ///< 通常時は1秒
		}

		while (true) {
			if (ts.touched()) {
				while (ts.touched()) {
					sleep_ms(100);
				}
				break; ///< タッチで抜ける
			}
			timeOut--;
			if (timeOut <= 0) {
				break; ///< タイムアウトで抜ける
			}
			sleep_ms(100); ///< 100ms待機
		}
	}
}


int main()
{
	Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
	XPT2046_Touchscreen ts(TOUCH_CS); 
	appMode = APP_MODE_STARTING; // アプリケーションモードを初期化

	settings.load();                 // フラッシュメモリの内容を読み込む
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

	ts.begin();                         // タッチパネル初期化
	ts.setRotation(TFTROTATION.NORMAL); // タッチパネルの回転を設定（液晶画面と合わせる）
	// 起動時にタッチさていれば、設定値のリセット
	if (ts.touched()) {
		int touchCnt = 0;
		while (ts.touched()) { // タッチされている間は待つ。ただし、ロングタップされているときはキャリブレーションに
			delay(100);
			touchCnt++;
			if (touchCnt > 20) {               // 2秒以上タッチされている場合は設定を初期化する
				tft.fillScreen(ILI9341_BLACK); // 画面を黒で塗りつぶす
				touchCnt = 0;                  // タッチカウントをリセット
				GUIMsgBox msgbox(&tft, &ts);
				bool bRet = msgbox.showOKCancel(30, 100, "確認", "設定を初期化します", "  OK  ", " CANCEL");
				if (bRet) {
					settings.setDefault();
					settings.save(); // 設定をフラッシュメモリに保存
				}
			}
		}
	}
	ts.setCalibration(settings.getMinX(), settings.getMinY(), settings.getMaxX(), settings.getMaxY());

// シリアルデバッグが有効な場合、ここでしばらく待ち、ポートの接続を行う
	if (settings.isSerialDebug()) {
		GUIMsgBox msgbox(&tft, &ts);
		tft.fillScreen(ILI9341_BLACK); // 画面を黒で塗りつぶす
		msgbox.showOK(30, 100, "シリアルデバッグ", "COMポートを接続後、\nOKを押してください。", "  OK  ");
	}


	Initialize(tft, ts, as3935 , iNet); // 初期化関数を呼び出す


	delay(1000);

	mainDisplay(tft, as3935, false, true, false, false); // 初期画面のバナー表示

	// --- 画面初期化・タイトル表示 ---
	// --- 割り込み・タイマー・メインループ ---
	// AS3935のIRQ割り込みを有効化
	dbgprintf("AS3935_IRQ %s PIN:%d\n","Enable", AS3935_IRQ );
	gpio_set_irq_enabled_with_callback(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, true, &as3935IRQCallback);
	// 1秒ごとのハートビートタイマーを設定
	repeating_timer_t timer;
	add_repeating_timer_ms(500, hartbeatCallback, NULL, &timer);
	appMode = APP_MODE_NORMAL;
	while (true) {
		if (appMode == APP_MODE_NORMAL) {

			isIRQTriggered = false;
			__wfe(); // 割り込みが発生するまでスリープ
			// 割り込みが発生し、その処理を行っている間は新しい割り込みは起こさない

			if (isIRQTriggered) {                                   // IRQがトリガーされた場合
				dbgprintf("AS3935_IRQ %s PIN:%d\n", "Disable", AS3935_IRQ);
				gpio_set_irq_enabled(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, false); // IRQ割り込みを一時的に無効化
				mainDisplay(tft, as3935, true, false, false, true);          // 雷センサーの画面表示を更新
				// IRQ割り込みを再度有効化
				dbgprintf("AS3935_IRQ %s PIN:%d\n", "Enable", AS3935_IRQ);
				gpio_set_irq_enabled(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, true); // IRQ割り込みを再度有効化

			} else {
				if (ts.touched()) {
					TS_Point tPoint;
					tPoint = ts.getPointOnScreen();
					if (tPoint.y < 20) {
						appMode = APP_MODE_SETTING; // 設定モードに切り替え
					}
				}
				// それ以外の処理があればここに追加
				mainDisplay(tft, as3935, false, false, true, false); // 時計の更新
			}
		} else if (appMode == APP_MODE_SETTING) {
			// 設定中はIRQ割り込みを禁止する
			dbgprintf("AS3935_IRQ %s PIN:%d\n", "Disable", AS3935_IRQ);
			gpio_set_irq_enabled(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, false); // IRQ割り込みを一時的に無効化
			cancel_repeating_timer(&timer);                              // ハートビートタイマーを停止
			tft.setCursor(0, 0);
			tft.printf("設定モード");
			//			settings.run(&tft, &ts); // 設定画面の実行
			settings.run2(&tft, &ts); // 設定画面の実行
			mustRedraw = true;
			DispClock::setRedrawFlag(); // 時計の再描画フラグを立てる
			appMode = APP_MODE_NORMAL;  // 通常モードに戻す
			add_repeating_timer_ms(500, hartbeatCallback, NULL, &timer);
			// as3935.Reset(); // AS3935のリセット
			dbgprintf("AS3935_IRQ %s PIN:%d\n", "Enable", AS3935_IRQ);
			gpio_set_irq_enabled(AS3935_IRQ, GPIO_IRQ_EDGE_RISE, true); // 設定が終わったらIRQ割り込みを復旧
		}
	}
}
