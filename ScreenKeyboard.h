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

    ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);
    void show(int y);
    void setKBType(KBType type);
    KBType getKBType() const;
    int getKeyFromTouch(int x, int y) const;
    
    uint8_t checkTouch();

private:
    Adafruit_ILI9341* m_tft;
    XPT2046_Touchscreen* m_ts;
    KBType m_kbType;

	int keyboardY = 0;

	static const int KB_WIDTH = 240;
    static const int KB_HEIGHT = 95;
    static const int KEY_SIZE = 24;
    static const int KEY_COLS = 10;
    static const int KEY_ROWS = 4;

	static const int KEY_COUNT = KEY_COLS * KEY_ROWS;
	static const uint8_t keycodes[3][KEY_COUNT];
};
