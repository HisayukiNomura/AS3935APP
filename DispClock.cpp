#include "DispClock.h"
#include <ctime>
#include <cstdio>
#include "pictData.h" // pictData.hのインクルード

using namespace ardPort;
using namespace ardPort::spi;

// 数字と表示するビットマップの定義
const uint16_t* DispClock::numTable[13] = {
	num_0, num_1, num_2, num_3, num_4,
	num_5, num_6, num_7, num_8, num_9,
	num_10, num_11, num_12};

Adafruit_ILI9341* DispClock::pTFT = nullptr; // TFTディスプレイへのポインタ
uint8_t DispClock::prevHour = 0xFF;
uint8_t DispClock::prevHourU = 0xFF; // 時の十の位
uint8_t DispClock::prevHourL = 0xFF; // 時の一の位
uint8_t DispClock::prevMinutesU = 0xFF;
uint8_t DispClock::prevMinutesL = 0xFF;
uint8_t DispClock::prevSeconds = 0xFF;
uint8_t DispClock::prevAmPm = 0xFF;
bool DispClock::isClock24Hour = true; // 24時間表示かどうかのフラグ

void DispClock::init(Adafruit_ILI9341* a_ptft, bool a_isClock24Hour)
{
	pTFT = a_ptft;                   // TFTディスプレイへのポインタを保存
	isClock24Hour = a_isClock24Hour; // 24時間表示かどうかのフラグを保存
}

/**
 * @brief 現在時刻をTFT画面に表示する
 * @details pictData.hのビットマップをdrawRGBBitmapで表示し、時刻を描画
 * @param tft TFTディスプレイへのポインタ
 * @param x   ビットマップ表示のX座標
 * @param y   ビットマップ表示のY座標
 */
void DispClock::show(int a_x, int a_y)
{
	int x = a_x;
	int y = a_y;
	// 現在時刻を取得
	time_t now = time(NULL);
	struct tm* local = localtime(&now);

	if (isClock24Hour) {
		uint8_t hourU = local->tm_hour / 10;
		uint8_t hourL = local->tm_hour % 10;
		if (prevHourU != hourU) {
			if (hourU == 0) {
				pTFT->drawRGBBitmap(x, y, num_blank, 32, 51); // 分の十の位
			} else {
				pTFT->drawRGBBitmap(x, y, numTable[local->tm_hour / 10], 32, 51); // 分の十の位
			}
			prevHourU = hourU;                                                // 前回の分の十の位を更新
		}
		if (prevHourL != hourL) {
			pTFT->drawRGBBitmap(x + 32, y, numTable[local->tm_hour % 10], 32, 51); // 分の一の位
			prevHourL = hourL;                                                     // 前回の分の十の位を更新
		}
		// コロン
		pTFT->drawRGBBitmap(x + 72, y, colon, 16, 51); // コロンのビットマップを表示
		x += 96;
	} else {
		uint8_t hour12 = local->tm_hour % 12;
		if (hour12 == 0) hour12 = 12; // 0時は12時に変換
		// 時

		if (prevHour != hour12) {
			pTFT->drawRGBBitmap(x, y, numTable[hour12], 32, 51); // ビットマップを表示
			prevHour = hour12;                                   // 前回の時刻を更新
			// コロン
			pTFT->drawRGBBitmap(x + 40, y, colon, 16, 51); // コロンのビットマップを表示
		}
		x += 64;
	}

	// 分
	uint8_t minutesU = local->tm_min / 10;
	uint8_t minutesL = local->tm_min % 10;
	if (prevMinutesU != minutesU) {
		pTFT->drawRGBBitmap(x, y, numTable[local->tm_min / 10], 32, 51); // 分の十の位
		prevMinutesU = minutesU;                                         // 前回の分の十の位を更新
	}
	if (prevMinutesL != minutesL) {
		pTFT->drawRGBBitmap(x + 32, y, numTable[local->tm_min % 10], 32, 51); // 分の一の位
		prevMinutesL = minutesL;                                              // 前回の分の十の位を更新
	}
	x += 70;
	// AMPM
	if (isClock24Hour == false) {
		uint8_t ampm = local->tm_hour / 12; // 0: AM, 1: PM
		if (prevAmPm != ampm) {
			if (ampm == 0) {
				pTFT->drawRGBBitmap(x, y, num_am, 30, 30); // AMのビットマップを表示
			} else {
				pTFT->drawRGBBitmap(x, y, num_pm, 30, 30); // PMのビットマップを表示
			}
			prevAmPm = ampm; // 前回のAM/PMを更新
		}
	}
	// 秒
	if (prevSeconds != local->tm_sec) {
		int secX;
		int secY;
		const uint16_t* pSecPict;
		uint8_t qIdx = local->tm_sec / 15;
		if (qIdx == 0) { // ０～１４秒
			secX = x + 11;
			secY = y + 32;
			pSecPict = secQ1;   // 秒のコロンのビットマップを選択
		} else if (qIdx == 1) { // １５～２９秒
			secX = x + 11;
			secY = y + 32 + 11;
			pSecPict = secQ2;   // 秒のコロンのビットマップを選択
		} else if (qIdx == 2) { // ３０～４４秒
			secX = x;
			secY = y + 32 + 11;
			pSecPict = secQ3; // 秒のコロンのビットマップを選択
		} else {              // ４５～５９秒
			secX = x;
			secY = y + 32;
			pSecPict = secQ4; // 秒のコロンのビットマップを選択
		}
		if (local->tm_sec == 0) {
			pTFT->fillRect(x, y + 32, 21, 21, STDCOLOR.SUPERDARK_GRAY);
		}
		// pTFT->setCursor(0, 100);
		// pTFT->setTextColor(0xFFFF, 0x0000); // 白文字、黒背景
		// pTFT->printf("sec = %d, Qidx = %d ", local->tm_sec,qIdx); // デバッグ用に秒を表示
		if (local->tm_sec % 2 == 0 && local->tm_sec % 15 != 14) {
			// pTFT->printf("NODISP"); // デバッグ用に秒を表示

			pTFT->drawRGBBitmap(secX, secY, secQ0, 10, 10); // 秒のコロンを表示
		} else {
			// pTFT->printf("DISP  ");                            // デバッグ用に秒を表示
			pTFT->drawRGBBitmap(secX, secY, pSecPict, 10, 10); // 秒のコロンを表示
		}
		prevSeconds = local->tm_sec; // 前回の秒を更新
	}
}
