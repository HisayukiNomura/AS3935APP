#pragma once
#include <stdint.h>

#include "pictData.h"
#include "Adafruit_ILI9341.h"
#include "XPT2046_Touchscreen/XPT2046_Touchscreen.h"
using namespace ardPort;
using namespace ardPort::spi;
/**
 * @brief 画面にスクリーンキーボードを表示・管理するクラス
 */

class ScreenKeyboard {
public:
    enum KBType { KB0 = 0, KB1, KB2 };
	enum KeyMode { KM_QWERTY_1 = 0, KM_NUMPAD_1 };

	ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);
    void show(int y);
	void showNumPad(int y);
	void setKBType(KBType type);
    KBType getKBType() const;
    int getKeyFromTouch(int x, int y) const;
    
    uint8_t checkTouch();
public:
  static const int KB_WIDTH = 240;
  static const int KB_HEIGHT = 95;
  static const int KEY_SIZE = 24;
  static const int KEY_COLS = 10;
  static const int KEY_ROWS = 4;

  static const int NP_WIDTH = 96;
  static const int NP_HEIGHT = 96;
  static const int NP_SIZE = 24;
  static const int NP_COLS = 4;
  static const int NP_ROWS = 4;

  int currWidth;
  int currHeight;
  int currSize;
  int currCols;
  int currRows;


private:
  Adafruit_ILI9341* m_tft;
  XPT2046_Touchscreen* m_ts;
  KBType m_kbType;
  KeyMode m_kbMode;
  int keyboardY = 0;

  static const int KEY_COUNT = KEY_COLS * KEY_ROWS;
  static const int NP_COUNT = NP_COLS * NP_ROWS;
  static const uint8_t keycodes[3][KEY_COUNT];
  static const uint8_t npCodes[NP_COUNT];
};
