#include "ScreenKeyboard.h"
#include "pictData.h"
using namespace ardPort;
using namespace ardPort::spi;

const uint8_t ScreenKeyboard::keycodes[3][ScreenKeyboard::KEY_COUNT] = {
	{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x0D,
	 'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ', 0x0f, 0x08},
	{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
	 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0x0D,
	 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ', 0x0f, 0x7F},
	{'!', '\"', '#', '$', '%', '&', '\'', '(', ')', '_',
	 '^', '~', '|', '\\', '@', '{', '}', '[', ']', '?',
	 '+', '-', '*', '/', '=', ':', ';', '<', '>', 0x0D,
	 '`', ' ', ' ', ' ', ' ', 0x7E , 0x1B, ' ', 0x0f, 0x7F},
};

ScreenKeyboard::ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts)
    : m_tft(tft), m_ts(ts), m_kbType(KB1) {}

void ScreenKeyboard::show(int y) {
    const uint16_t* bmp = nullptr;
	keyboardY = y;
	switch (m_kbType) {
        case KB0: bmp = picKB0; break;
        case KB1: bmp = picKB1; break;
        case KB2: bmp = picKB2; break;
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

uint8_t ScreenKeyboard::checkTouch() {
    if (!m_ts) return 0xFF;
    if (!m_ts->touched()) return 0xFF;
	TS_Point p = m_ts->getPointOnScreen();
	p.y = p.y - keyboardY;
    if (p.y <0 || p.y >= KB_HEIGHT) return 0xFF; // キーボード領域外なら-1
    uint8_t code = keycodes[m_kbType][getKeyFromTouch(p.x, p.y)];
    while(m_ts->touched()) {        // キーが離されてから初めて入力されたと判断する
		delay(100);
	}
    if (code == 0x0f) {
		m_kbType = static_cast<KBType>((m_kbType + 1) % 3); // 次のキーボードタイプに切り替え
        show(keyboardY); // キーボードを再描画
		return 0xFF;
	}
	return code;
}



