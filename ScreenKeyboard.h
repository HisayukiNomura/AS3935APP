/**
 * @file ScreenKeyboard.h
 * @brief 画面スクリーンキーボードの表示・管理クラス宣言
 * @details
 * Adafruit_ILI9341ディスプレイとXPT2046タッチスクリーンを用いて、
 * QWERTY配列やテンキー配列のソフトウェアキーボードを画面上に表示し、
 * タッチ入力からキーコードを取得する機能を提供する。
 */
#pragma once
#include <stdint.h>

#include "pictData.h"
#include "Adafruit_ILI9341.h"
#include "XPT2046_Touchscreen/XPT2046_Touchscreen.h"
using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief 画面にスクリーンキーボードを表示・管理するクラス
 * @details
 * show()でキーボードを表示し、タッチ入力からキーコードを取得できる。
 * QWERTY配列・テンキー配列・各種モードに対応。
 */
class ScreenKeyboard {
public:
    /**
     * @brief キーボード種別
     */
    enum KBType { KB0 = 0, KB1, KB2 };
    /**
     * @brief キーモード種別
     */
	enum KeyMode { KM_QWERTY_1 = 0, KM_NUMPAD_1 };

    /**
     * @brief コンストラクタ
     * @param tft ディスプレイ制御用ポインタ
     * @param ts タッチスクリーン制御用ポインタ
     */
	ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);
    /**
     * @brief QWERTYキーボードを表示
     * @param y キーボード表示Y座標
     */
    void show(int y);
    /**
     * @brief テンキー（NumPad）を表示
     * @param y テンキー表示Y座標
     */
	void showNumPad(int y);
    /**
     * @brief キーボード種別を設定
     * @param type キーボード種別
     */
	void setKBType(KBType type);
    /**
     * @brief 現在のキーボード種別を取得
     * @return キーボード種別
     */
    KBType getKBType() const;
    /**
     * @brief タッチ座標からキー番号を取得
     * @param x タッチX座標
     * @param y タッチY座標
     * @return キー番号（-1:該当なし）
     */
    int getKeyFromTouch(int x, int y) const;
    /**
     * @brief タッチ状態をチェックし、押されたキーコードを返す
     * @return キーコード（0:押されていない）
     */
    uint8_t checkTouch();
public:
  static const int KB_WIDTH = 240;   ///< キーボード全体の幅
  static const int KB_HEIGHT = 95;   ///< キーボード全体の高さ
  static const int KEY_SIZE = 24;    ///< 1キーのサイズ
  static const int KEY_COLS = 10;    ///< キー列数
  static const int KEY_ROWS = 4;     ///< キー行数

  static const int NP_WIDTH = 96;    ///< テンキー全体の幅
  static const int NP_HEIGHT = 96;   ///< テンキー全体の高さ
  static const int NP_SIZE = 24;     ///< テンキー1キーのサイズ
  static const int NP_COLS = 4;      ///< テンキー列数
  static const int NP_ROWS = 4;      ///< テンキー行数

  int currWidth;   ///< 現在のキーボード幅
  int currHeight;  ///< 現在のキーボード高さ
  int currSize;    ///< 現在のキーサイズ
  int currCols;    ///< 現在の列数
  int currRows;    ///< 現在の行数

private:
  Adafruit_ILI9341* m_tft; ///< ディスプレイ制御用
  XPT2046_Touchscreen* m_ts; ///< タッチスクリーン制御用
  KBType m_kbType; ///< 現在のキーボード種別
  KeyMode m_kbMode; ///< 現在のキーモード
  int keyboardY = 0; ///< キーボード表示Y座標

  static const int KEY_COUNT = KEY_COLS * KEY_ROWS; ///< キーボード全体のキー数
  static const int NP_COUNT = NP_COLS * NP_ROWS;    ///< テンキー全体のキー数
  static const uint8_t keycodes[3][KEY_COUNT]; ///< QWERTY配列キーコードテーブル
  static const uint8_t npCodes[NP_COUNT];      ///< テンキー配列キーコードテーブル
};
