/**
 * @file DispClock.cpp
 * @brief TFTディスプレイに時計を描画するクラスの実装
 * @details
 * - pictData.hのビットマップを利用し、時・分・秒・AM/PMをグラフィカルに描画。
 * - 24時間/12時間表示切替、描画最適化のための前回値キャッシュ、ビットマップ描画座標計算などを実装。
 * - 秒表示は15秒ごとに異なるビットマップでアニメーション風に表示。
 */
#include "DispClock.h"
#include <ctime>
#include <cstdio>
#include "pictData.h" ///< 時計用ビットマップ定義

using namespace ardPort;
using namespace ardPort::spi;

// 数字と表示するビットマップの定義
const uint16_t* DispClock::numTable[13] = {
	num_0, num_1, num_2, num_3, num_4,
	num_5, num_6, num_7, num_8, num_9,
	num_10, num_11, num_12}; ///< 0～12の数字ビットマップテーブル

Adafruit_ILI9341* DispClock::pTFT = nullptr; ///< TFTディスプレイへのポインタ
uint8_t DispClock::prevHour = 0xFF;          ///< 前回描画した時（12h用）
uint8_t DispClock::prevHourU = 0xFF;         ///< 前回描画した時の十の位（24h用）
uint8_t DispClock::prevHourL = 0xFF;         ///< 前回描画した時の一の位（24h用）
uint8_t DispClock::prevMinutesU = 0xFF;      ///< 前回描画した分の十の位
uint8_t DispClock::prevMinutesL = 0xFF;      ///< 前回描画した分の一の位
uint8_t DispClock::prevSeconds = 0xFF;       ///< 前回描画した秒
uint8_t DispClock::prevAmPm = 0xFF;          ///< 前回描画したAM/PM
bool DispClock::isClock24Hour = true;        ///< 24時間表示かどうかのフラグ

/**
 * @brief 時計描画用の初期化
 * @details
 * TFTディスプレイのポインタと24時間表示フラグを保存する。
 * @param a_ptft TFTディスプレイへのポインタ
 * @param a_isClock24Hour 24時間表示かどうか
 * @retval なし
 */
void DispClock::init(Adafruit_ILI9341* a_ptft, bool a_isClock24Hour)
{
	pTFT = a_ptft;                   ///< TFTディスプレイへのポインタを保存
	isClock24Hour = a_isClock24Hour; ///< 24時間表示かどうかのフラグを保存
}

/**
 * @brief 現在時刻をTFT画面に描画する
 * @details
 * - pictData.hのビットマップをdrawRGBBitmapで表示し、時刻をグラフィカルに描画。
 * - 24時間/12時間表示切替、AM/PM表示、前回値キャッシュによる描画最適化。
 * - 秒表示は15秒ごとに異なるビットマップでアニメーション風に表示。
 *
 * @param a_x ビットマップ表示のX座標
 * @param a_y ビットマップ表示のY座標
 * @retval なし
 */
void DispClock::show(int a_x, int a_y)
{
	int x = a_x; ///< 描画開始X座標
	int y = a_y; ///< 描画開始Y座標
	// 現在時刻を取得
	time_t now = time(NULL);              ///< 現在のUNIX時刻（エポック秒）を取得
	struct tm* local = localtime(&now);   ///< ローカルタイムゾーンの時刻構造体へ変換

	if (isClock24Hour) {
		uint8_t hourU = local->tm_hour / 10; ///< 時の十の位
		uint8_t hourL = local->tm_hour % 10; ///< 時の一の位
		if (prevHourU != hourU) {
			if (hourU == 0) {
				pTFT->drawRGBBitmap(x, y, num_blank, 32, 51); // 分の十の位
			} else {
				pTFT->drawRGBBitmap(x, y, numTable[local->tm_hour / 10], 32, 51); // 分の十の位
			}
			prevHourU = hourU;                                                ///< 前回の分の十の位を更新
		}
		if (prevHourL != hourL) {
			pTFT->drawRGBBitmap(x + 32, y, numTable[local->tm_hour % 10], 32, 51); // 分の一の位
			prevHourL = hourL;                                                     ///< 前回の分の十の位を更新
		}
		// コロン
		pTFT->drawRGBBitmap(x + 72, y, colon, 16, 51); ///< コロンのビットマップを表示
		x += 96;
	} else {
		uint8_t hour12 = local->tm_hour % 12; ///< 12時間制の時
		if (hour12 == 0) hour12 = 12; // 0時は12時に変換
		if (prevHour != hour12) {
			pTFT->drawRGBBitmap(x, y, numTable[hour12], 32, 51); ///< ビットマップを表示
			prevHour = hour12;                                   ///< 前回の時刻を更新
			pTFT->drawRGBBitmap(x + 40, y, colon, 16, 51);       ///< コロンのビットマップを表示
		}
		x += 64;
	}

	// 分
	uint8_t minutesU = local->tm_min / 10; ///< 分の十の位
	uint8_t minutesL = local->tm_min % 10; ///< 分の一の位
	if (prevMinutesU != minutesU) {
		pTFT->drawRGBBitmap(x, y, numTable[local->tm_min / 10], 32, 51); ///< 分の十の位
		prevMinutesU = minutesU;                                         ///< 前回の分の十の位を更新
	}
	if (prevMinutesL != minutesL) {
		pTFT->drawRGBBitmap(x + 32, y, numTable[local->tm_min % 10], 32, 51); ///< 分の一の位
		prevMinutesL = minutesL;                                              ///< 前回の分の十の位を更新
	}
	x += 70;
	// AMPM
	if (isClock24Hour == false) {
		uint8_t ampm = local->tm_hour / 12; ///< 0: AM, 1: PM
		if (prevAmPm != ampm) {
			if (ampm == 0) {
				pTFT->drawRGBBitmap(x, y, num_am, 30, 30); ///< AMのビットマップを表示
			} else {
				pTFT->drawRGBBitmap(x, y, num_pm, 30, 30); ///< PMのビットマップを表示
			}
			prevAmPm = ampm; ///< 前回のAM/PMを更新
		}
	}
	// 秒
	if (prevSeconds != local->tm_sec) {
		int secX; ///< 秒ビットマップのX座標
		int secY; ///< 秒ビットマップのY座標
		const uint16_t* pSecPict; ///< 秒ビットマップのポインタ
		uint8_t qIdx = local->tm_sec / 15; ///< 15秒ごとの区分
		if (qIdx == 0) { // ０～１４秒
			secX = x + 11;
			secY = y + 32;
			pSecPict = secQ1;   ///< 秒のコロンのビットマップを選択
		} else if (qIdx == 1) { // １５～２９秒
			secX = x + 11;
			secY = y + 32 + 11;
			pSecPict = secQ2;   ///< 秒のコロンのビットマップを選択
		} else if (qIdx == 2) { // ３０～４４秒
			secX = x;
			secY = y + 32 + 11;
			pSecPict = secQ3; ///< 秒のコロンのビットマップを選択
		} else {              // ４５～５９秒
			secX = x;
			secY = y + 32;
			pSecPict = secQ4; ///< 秒のコロンのビットマップを選択
		}
		if (local->tm_sec == 0) {
			pTFT->fillRect(x, y + 32, 21, 21, STDCOLOR.SUPERDARK_GRAY); ///< 秒表示エリアをクリア
		}
		if (local->tm_sec % 2 == 0 && local->tm_sec % 15 != 14) {
			pTFT->drawRGBBitmap(secX, secY, secQ0, 10, 10); ///< 秒のコロンを表示（点滅）
		} else {
			pTFT->drawRGBBitmap(secX, secY, pSecPict, 10, 10); ///< 秒のコロンを表示
		}
		prevSeconds = local->tm_sec; ///< 前回の秒を更新
	}
}
