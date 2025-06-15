#include "ScreenKeyboard.h"
#include "pictData.h"
using namespace ardPort;
using namespace ardPort::spi;

ScreenKeyboard::ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts)
    : m_tft(tft), m_ts(ts), m_kbType(KB1) {}

void ScreenKeyboard::show(int y) {
    const uint16_t* bmp = nullptr;
    switch (m_kbType) {
        case KB1: bmp = picKB1; break;
        case KB2: bmp = picKB2; break;
        case KB3: bmp = picKB3; break;
    }
    if (bmp) {
        m_tft->drawRGBBitmap(0, y, bmp, KB_WIDTH, KB_HEIGHT);
    }
}

void ScreenKeyboard::setKBType(KBType type) {
    m_kbType = type;
}

ScreenKeyboard::KBType ScreenKeyboard::getKBType() const {
    return m_kbType;
}

int ScreenKeyboard::getKeyFromTouch(int x, int y) const {
    // キーボード領域外なら-1
    if (x < 0 || x >= KB_WIDTH || y < 0 || y >= KB_HEIGHT) return -1;
    int col = x / KEY_SIZE;
    int row = y / KEY_SIZE;
    if (col >= KEY_COLS || row >= KEY_ROWS) return -1;
    return row * KEY_COLS + col;
}

int ScreenKeyboard::checkTouch() {
    if (!m_ts) return -1;
    if (!m_ts->touched()) return -1;
    TS_Point p = m_ts->getPoint();
    // キーボード表示位置がy=0以外の場合は、y座標の補正が必要
    // ここではshowで表示したY座標が0前提。必要なら引数でYオフセットを渡す設計に拡張可
    return getKeyFromTouch(p.x, p.y);
}



