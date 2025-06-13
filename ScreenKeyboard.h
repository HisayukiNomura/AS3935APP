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
    enum KBType { KB1 = 0, KB2, KB3 };

    ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);
    void show(int y);
    void setKBType(KBType type);
    KBType getKBType() const;
    int getKeyFromTouch(int x, int y) const;
    int checkTouch();

private:
    Adafruit_ILI9341* m_tft;
    XPT2046_Touchscreen* m_ts;
    KBType m_kbType;
    static const int KB_WIDTH = 240;
    static const int KB_HEIGHT = 95;
    static const int KEY_SIZE = 24;
    static const int KEY_COLS = 10;
    static const int KEY_ROWS = 4;
};
