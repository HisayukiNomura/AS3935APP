/**
 * @file GUIEditBox.cpp
 * @brief 画面エディットボックスの実装
 * @details
 * 画面上にキーボードと連動したエディットボックスを表示し、
 * 文字列編集・カーソル制御・挿入/上書きモード・特殊キー処理などを提供する。
 */
#include "Settings.h"
#include <cstring>
#include <cstdio>
#include "FlashMem.h"
#include "GUIEditbox.h"

#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
#include "ScreenKeyboard.h"
using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief GUIEditBoxクラスのコンストラクタ
 * @details
 * ディスプレイ・タッチスクリーン・キーボードのポインタを受け取る。
 * @param a_ptft ディスプレイ制御用ポインタ
 * @param a_pts タッチスクリーン制御用ポインタ
 * @param a_psk スクリーンキーボード用ポインタ
 */
GUIEditBox::GUIEditBox(Adafruit_GFX* a_ptft, XPT2046_Touchscreen* a_pts, ScreenKeyboard* a_psk)
    : ptft(static_cast<Adafruit_ILI9341*>(a_ptft)), pts(a_pts), psk(a_psk)
{
    // 初期化処理が必要ならここに追加
}

/**
 * @brief エディットボックスを表示し、文字列編集を行う
 * @details
 * - 編集欄の座標・バッファ・サイズ・編集モードを指定して呼び出す。
 * - テキストモードならQWERTYキーボード、数字上書きモードならテンキーを画面下部に表示。
 * - カーソル位置・挿入/上書きモードを初期化。
 * - ループ内でタッチ入力を監視し、
 *   - 何も押されていなければカーソルを点滅表示。
 *   - キーが押されたら、
 *     - Enter(0x0d)で編集確定（trueを返す）
 *     - ESC(0x1b)でキャンセル（falseを返す）
 *     - カーソル移動（0x13:左, 0x14:右）、挿入/上書き切替（0x7e）
 *     - バックスペース(0x08)・デリート(0x7F)・通常文字の挿入/上書き
 *   - 文字列長やカーソル範囲はバッファサイズを超えないよう制御
 *   - 入力内容が変化したら都度再描画
 * - ループを抜けた時点で、trueなら編集内容確定、falseならキャンセル
 *
 * @param a_x 編集欄X座標
 * @param a_y 編集欄Y座標
 * @param a_pText 編集対象文字列バッファ
 * @param a_size バッファサイズ
 * @param a_mode 編集モード
 * @return Enterでtrue、ESCでfalse
 */
bool GUIEditBox::show(uint16_t a_x, uint16_t a_y, char* a_pText, size_t a_size,EditMode a_mode)
{
	mode = a_mode;
	if(mode == MODE_NUMPADOVERWRITE) {
		isInsert = false;
		edtCursor = 0;                                    // カーソルを先頭に設定
		psk->showNumPad(ptft->height() - psk->KB_HEIGHT); // キーボードを表示
	} else {
		edtCursor = strlen(a_pText);
		psk->show(ptft->height() - psk->KB_HEIGHT); // キーボードを表示
		isInsert = true;
	}
	p = a_pText;
	edtX = a_x;
	edtY = a_y;

	int waitCnt = 0;
	bool retReason = false;

	while (true) {
		uint8_t ret = psk->checkTouch();
		if (ret == 0xff) { // タッチされていない場合はカーソルを点滅させたりする
			waitCnt++;
			waitCnt = waitCnt % 0xFF;
			dispCursor(waitCnt, false); // カーソルを点滅させる
			delay(5);                      // 少し待つ
			continue;
		} else {
			dispCursor(waitCnt, true); // カーソルを消す
			waitCnt = 0;                  // カーソルの点滅をリセット
		}
		// 特殊コードの処理
		if (ret == 0x0d) {
			retReason = true;
			break;                // Enterキーが押されたら終了
		} else if (ret == 0x1b) { // ESCキーが押されたらキャンセル
			retReason  = false;
			break;
		} else if (ret == 0x13) { // ↚キー
			edtCursor--;
			if (edtCursor < 0) {
				edtCursor = 0; // カーソルが先頭を超えないようにする
			}
			if (edtCursor > strlen(p)) {
				edtCursor = strlen(p); // カーソルが文字列の末尾を超えないようにする
			}
			ret = 0;
		} else if (ret == 0x14) { // ⇒キー
			if (p[edtCursor] == 0) {
				continue;
			}
			edtCursor++;
			ret = 0;
		} else if (ret == 0x7e) {
			isInsert = !isInsert;
			ret = 0;
		}
		if (ret == 0) {
			continue;
		}
		// 一般文字の処理
		//if (strlen(p) >= a_size - 1) {
		//	continue; // 文字列が最大長に達している場合は何もしない
		//}

		if (ret == 0x08) { // Backspaceキーが押されたら削除
			if (edtCursor > 0) {
				for (size_t i = edtCursor - 1; i < strlen(p); i++) {
					p[i] = p[i + 1]; // 文字を左にシフト
				}
				edtCursor--;
			} else {
				continue; // カーソルが先頭にある場合は何もしない
			}
		} else if (ret == 0x7F) { // DELETキーが押されたらその場の文字を削除
			if (p[edtCursor] == 0) {
				continue; // カーソルが末尾にある場合は何もしない
			}
			if (edtCursor < strlen(p)) { // Deleteキーが押されたら削除
				for (size_t i = edtCursor; i < strlen(p); i++) {
					p[i] = p[i + 1]; // 文字を左にシフト
				}
			}
		} else if (p[edtCursor] == '\0') { // 普通の文字の場合、カーソルが末尾にあるときは追加
			if (edtCursor >= a_size) {
				continue; // 文字列が最大長に達している場合は何もしない
			}
			p[edtCursor++] = ret;
			p[edtCursor] = '\0'; // 文字列の終端を設定
		} else {
			if (isInsert) { // 挿入モード
				if (strlen(p) >= a_size - 1) {
					continue; // 文字列が最大長に達している場合は何もしない
				} else if (edtCursor < strlen(p)) {
					for (size_t i = strlen(p); i > edtCursor; i--) {
						p[i] = p[i - 1]; // 文字を右にシフト
					}
				}
				p[edtCursor++] = ret; // 新しい文字を挿入
			} else {                  // 上書きモード
				p[edtCursor++] = ret; // 新しい文字で上書き
				if (edtCursor >= a_size - 1) {
					edtCursor = a_size - 1; // カーソルが最大長を超えないようにする
				}
			}
		}
		// 文字の変化に伴う再描画
		ptft->fillRect(edtX, edtY, 8*a_size, 16, STDCOLOR.BLACK); // 入力欄をクリア
		ptft->setCursor(edtX, edtY);
		ptft->printf(p);
	}
	return retReason; // trueならEnterキーが押された、falseならESCキーが押された
}

/**
 * @brief カーソルの表示・点滅・消去を行う
 * @details
 * 挿入/上書きモードやtick値に応じて、カーソルや文字の描画・消去を行う。
 * @param tick 点滅用カウンタ
 * @param isDeleteCursor trueでカーソル消去、falseで点滅
 */
void GUIEditBox::dispCursor(int tick, bool isDeleteCursor)
{
	if (isDeleteCursor) {
		ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
		ptft->setCursor(edtX + edtCursor * 8, edtY);
		ptft->print(p[edtCursor]);                                               // カーソルを消去
		ptft->drawFastHLine(edtX + edtCursor * 8, edtY + 17, 8, STDCOLOR.BLACK); // カーソルを消去

	} else if (isInsert) {
		if (tick > 128) {
			ptft->setTextColor(STDCOLOR.BLACK, STDCOLOR.WHITE);
			ptft->setCursor(edtX + edtCursor * 8, edtY);
			ptft->print(p[edtCursor]); // カーソルを表示
		} else {
			ptft->setTextColor(STDCOLOR.WHITE, STDCOLOR.BLACK);
			ptft->setCursor(edtX + edtCursor * 8, edtY);
			ptft->print(p[edtCursor]); // カーソルを消す
		}

	} else {
		if (tick > 128) {
			ptft->drawFastHLine(edtX + edtCursor * 8, edtY + 17, 8, STDCOLOR.WHITE); // カーソルを表示
		} else {
			ptft->drawFastHLine(edtX + edtCursor * 8, edtY + 17, 8, STDCOLOR.BLACK); // カーソルを表示
		}
	}
}