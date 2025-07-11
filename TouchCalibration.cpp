/**
 * @file TouchCalibration.cpp
 * @brief タッチパネルキャリブレーション・テスト機能の実装
 * @details
 * Adafruit_ILI9341ディスプレイとXPT2046タッチスクリーンを用いた、
 * タッチパネルのキャリブレーション・テスト・メニュー表示・座標取得・描画処理を提供します。
 */
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "TouchCalibration.h"
#include <algorithm>
#include <stdio.h>
#include <pico/stdlib.h> // sleep_ms用

using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief TouchCalibrationクラスのコンストラクタ
 * @details
 * ディスプレイ・タッチスクリーンのポインタを受け取り、初期モードを選択状態に設定します。
 * @param tft ディスプレイ制御用ポインタ
 * @param ts タッチスクリーン制御用ポインタ
 */
TouchCalibration::TouchCalibration(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts) :
	m_tft(tft),
	m_ts(ts),
	m_mode(MODE_SELECT) {}

/**
 * @brief キャリブレーション・テスト・終了メニューを表示し、ユーザー操作を受け付ける
 * @details
 * メニュー画面でタッチ位置に応じてキャリブレーション・テスト・終了を選択。
 * キャリブレーションが正常に完了した場合はtrueを返す。
 * @return キャリブレーションが正常に完了した場合true、終了または未完了の場合false
 */
bool TouchCalibration::run()
{
	while (true) {
		showMenu();
		// タッチで上半分/下半分を選択、ロングタップで終了
		while (m_mode == MODE_SELECT) {
			if (m_ts->touched()) {
				TS_Point p = m_ts->getPointOnScreen();
				if (p.y > 20 && p.y < 120) {
					m_mode = MODE_CALIBRATE;
				} else if (p.y > 120 && p.y < 220) {
					m_mode = MODE_TEST;
				} else if (p.y > 220) {
					m_tft->fillScreen(ILI9341_BLACK);
					m_tft->setTextColor(ILI9341_WHITE);
					m_tft->setCursor(10, 40);
					m_tft->printf("終了します\n");
					if (m_ts->touched()) {};
					sleep_ms(1000);
					if (isCalibDone) {
						if (minX != 0 && maxX != 0xFFFF && minY != 0 && maxY != 0xFFFF) {
							return true;
						}
					}
					return false;
				}
			}
		}
		if (m_mode == MODE_CALIBRATE) {
			runCalibrateMode();
			m_mode = MODE_SELECT;
			if (m_ts->touched()) {};
		} else if (m_mode == MODE_TEST) {
			runTestMode();
			m_mode = MODE_SELECT;
			if (m_ts->touched()) {};
		} else {
			break;
		}
	}
	return false;
}

/**
 * @brief メニュー画面を描画する
 * @details
 * キャリブレーション・テスト・終了の各説明を画面に表示します。
 */
void TouchCalibration::showMenu()
{
	m_tft->fillScreen(ILI9341_BLACK);
	m_tft->setTextColor(ILI9341_WHITE);
	m_tft->fillRect(0, 0, 240, 24, STDCOLOR.BLUE);
	m_tft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLUE);
	m_tft->setCursor(10, 4);
	m_tft->printf("設定 - タッチスクリーン調整");

	m_tft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	m_tft->setCursor(0, 40);
	m_tft->printf("<<タッチの調整>>\nタッチスクリーンの調整を行います。指示に従って画面をタップしてください。一度開始すると、調整が完了するまで中断することはできません。");
	m_tft->setCursor(0, 180);
	m_tft->printf("<<タッチのテスト>>\nタッチスクリーンの確認を行います。画面をタップすると赤い丸が表示されます。ロングタップで終了します。必要なら再度調整してください。");
	m_tft->setCursor(0, 280);
	m_tft->printf("<<終了>>\n終了してメニューに戻ります");
}

/**
 * @brief ロングタップ（1秒以上）を検出するまで待機する
 * @details
 * タッチが1秒以上続いた場合にループを抜ける。
 */
void TouchCalibration::waitLongTap()
{
	int count = 0;
	while (m_ts->touched()) {
		sleep_ms(10);
		count++;
		if (count > 100) break; // 1秒以上タッチでロングタップ
	}
}

/**
 * @brief タッチテストモードを実行し、座標を表示・描画する
 * @details
 * タッチ位置の座標を画面に表示し、赤丸で描画。ロングタップで終了。
 */
void TouchCalibration::runTestMode()
{
	m_tft->fillScreen(STDCOLOR.BLACK);
	m_tft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	m_tft->setCursor(10, 10);
	m_tft->printf("テストモード\n\nロングタップで終了\n");
	while (true) {
		if (m_ts->touched()) {
			TS_Point p = m_ts->getPointOnScreen();
			m_tft->setCursor(10, 100);
			m_tft->printf("X:%4d Y:%4d\n", p.x, p.y);

			drawTouchPoint(p.x, p.y, ILI9341_RED);
			int touchCnt = 0;
			while (m_ts->touched()) {
				sleep_ms(100); // タッチが続いている間は待機
				touchCnt++;
				if (touchCnt > 20) {

					return; // キャリブレーション後は終了
				}
			}
		}
	}
}

/**
 * @brief キャリブレーションモードを実行し、四隅のタッチ座標を取得・補正値を計算する
 * @details
 * 各コーナーを10回ずつタップし、最小・最大座標を記録。
 * 計算結果をタッチスクリーンに設定し、完了メッセージを表示。
 */
void TouchCalibration::runCalibrateMode()
{

	const char* cornerMsg[4] = {"左上", "右上", "右下", "左下"};
	minX = 0xFFFF;
	maxX = 0;
	minY = 0xFFFF;
	maxY = 0;

	for (int corner = 0; corner < 4; ++corner) {
		m_tft->fillScreen(ILI9341_BLUE);
		m_tft->setTextColor(ILI9341_WHITE);
		m_tft->setCursor(10, 30);
		m_tft->printf("%sを10回タップ\n", cornerMsg[corner]);
		for (int i = 0; i < 10; ++i) {
			while (!m_ts->touched()) {}
			TS_Point p = m_ts->getPoint(); // 生のデータ
			TS_Point pOnScr = m_ts->getPointOnScreen();
			if (p.x > maxX) maxX = p.x; // 最大値更新
			if (p.x < minX) minX = p.x; // 最小値更新
			if (p.y > maxY) maxY = p.y; // 最大値更新
			if (p.y < minY) minY = p.y; // 最小値更新
			drawTouchPoint(pOnScr.x, pOnScr.y, ILI9341_YELLOW);

			m_tft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
			m_tft->setCursor(10, 60);
			m_tft->printf("RAW:%d: X:%4d Y:%4d(\n", i + 1, p.x, p.y);
			m_tft->printf("MOD:%d: X:%4d Y:%4d(\n", i + 1, pOnScr.x, pOnScr.y);
			while (m_ts->touched()) {}
		}
		m_tft->setCursor(10, 100);
		m_tft->printf("結果：(%d,%d) - (%d,%d)", minX, minY, maxX, maxY);
		sleep_ms(1000);
	}
	// キャリブレーション値をtsに設定（setCalibrationValueは未実装のためコメントアウト）

	m_ts->setCalibration(minX, minY, maxX, maxY); // TODO: XPT2046_Touchscreen側で実装必要
	m_tft->fillScreen(STDCOLOR.BLACK);
	m_tft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
	m_tft->setCursor(10, 60);
	m_tft->printf("キャリブレーション完了\n");
	isCalibDone = true;
	sleep_ms(1000);
}

/**
 * @brief 指定座標にタッチポイントを描画する
 * @details
 * 指定座標に色付き円を描画し、外枠を白・黒で装飾します。
 * @param x X座標
 * @param y Y座標
 * @param color 円の色
 */
void TouchCalibration::drawTouchPoint(int x, int y, uint16_t color)
{
	m_tft->fillCircle(x, y, 5, color);
	m_tft->drawCircle(x, y, 5, ILI9341_WHITE);
	m_tft->drawCircle(x, y, 6, ILI9341_BLACK);
}
