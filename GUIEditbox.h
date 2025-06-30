/**
 * @file GUIEditbox.h
 * @brief 画面エディットボックスの宣言
 * @details
 * 画面上にキーボードと連動したエディットボックスを表示し、
 * 文字列編集・カーソル制御・挿入/上書きモード・特殊キー処理などを提供する。
 */
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include "FlashMem.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
/**
 * @namespace ardPort
 * @brief Arduino互換ポートラッパーの名前空間
 * @details
 * Pico SDK上でArduinoライクなAPIを提供するためのラッパー。
 * SPI/I2C/GPIO等の抽象化を含む。
 */
using namespace ardPort;
/**
 * @namespace ardPort::spi
 * @brief SPI関連APIの名前空間
 * @details
 * SPI通信の初期化・データ転送・ピン制御等を提供。
 */
using namespace ardPort::spi;
class ScreenKeyboard; // 前方宣言

/**
 * @brief 編集モード種別
 */
enum EditMode {
	MODE_TEXT = 0, ///< テキスト編集モード
	MODE_NUMPADOVERWRITE,   ///< 上書き制限された数字入力（時刻の入力など）
};

/**
 * @brief 画面エディットボックスクラス
 * @details
 * 画面上にキーボードと連動したエディットボックスを表示し、
 * 文字列編集・カーソル制御・挿入/上書きモード・特殊キー処理などを提供する。
 */
class GUIEditBox {
public:
    Adafruit_ILI9341* ptft; ///< ディスプレイ制御用ポインタ
    XPT2046_Touchscreen* pts; ///< タッチスクリーン制御用ポインタ
    ScreenKeyboard* psk; ///< スクリーンキーボード用ポインタ

	char* p = nullptr; ///< 編集対象の文字列ポインタ
	uint8_t edtCursor = 0; ///< 編集カーソル位置
	uint16_t edtX = 0; ///< 編集欄X座標
	uint16_t edtY = 0; ///< 編集欄Y座標
	bool isInsert = false; ///< 挿入モード
	EditMode mode; ///< 編集モード

	/**
	 * @brief コンストラクタ
	 * @param a_ptft ディスプレイ制御用ポインタ
	 * @param a_pts タッチスクリーン制御用ポインタ
	 * @param a_psk スクリーンキーボード用ポインタ
	 */
	GUIEditBox(Adafruit_GFX* a_ptft, XPT2046_Touchscreen* a_pts, ScreenKeyboard* a_psk);

	/**
	 * @brief エディットボックスを表示し、文字列編集を行う
	 * @details
	 * キーボード種別・モードに応じて編集欄を表示し、
	 * 入力・カーソル移動・挿入/上書き・特殊キー処理を行う。
	 * @param a_x 編集欄X座標
	 * @param a_y 編集欄Y座標
	 * @param a_pText 編集対象文字列バッファ
	 * @param a_size バッファサイズ
	 * @param a_mode 編集モード
	 * @return Enterでtrue、ESCでfalse
	 */
	bool show(uint16_t a_x, uint16_t a_y, char* a_pText, size_t a_size,EditMode a_mode);
	/**
	 * @brief カーソルの表示・点滅・消去を行う
	 * @details
	 * 挿入/上書きモードやtick値に応じて、カーソルや文字の描画・消去を行う。
	 * @param tick 点滅用カウンタ
	 * @param isDeleteCursor trueでカーソル消去、falseで点滅
	 */
	void dispCursor(int tick, bool isDeleteCursor = false);
	
};
