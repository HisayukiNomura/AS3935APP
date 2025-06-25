#include "GUIMsgBox.h"
#include "pico/stdlib.h"
#include <cstring>

GUIMsgBox::GUIMsgBox(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts)
    : m_tft(tft), m_ts(ts) {}

// ユーティリティ: \r\n区切りで複数行を描画
static void drawMultiline(Adafruit_ILI9341* tft, int x, int y, const char* message, uint16_t textColor, uint16_t bgColor, int lineHeight = 18) {
    char buf[128];
    strncpy(buf, message, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* line = strtok(buf, "\r\n");
    int curY = y;
    while (line) {
        // 先頭の空白をスキップ（左揃え）
        while (*line == ' ' || *line == '\t') line++;
        tft->setCursor(x, curY);
        tft->setTextColor(textColor, bgColor);
        tft->printf("%s", line);
        curY += lineHeight;
        line = strtok(NULL, "\r\n");
    }
}

bool GUIMsgBox::showOKCancel(int x, int y, const char* caption, const char* message, const char* okMsg, const char* cancelMsg) 
{
	m_tft->pushTextColor();
	const int w = 180, h = 126; // 高さ+10
    m_tft->fillRoundRect(x, y, w, h, 8, 0x7BEF); // 薄グレー
    m_tft->drawRoundRect(x, y, w, h, 8, 0xFFFF); // 白枠
    m_tft->setTextColor(0x0000, 0x7BEF);
    m_tft->setCursor(x + 10, y + 10);
    m_tft->printf("%s", caption);
    // キャプションとメッセージの間に8ドット空ける
    drawMultiline(m_tft, x + 10, y + 30 + 8, message, 0x0000, 0x7BEF);

    // OK/Cancelボタン
    int btnW = 68, btnH = 28;
    int okX = x + 20, okY = y + h - 32; // 8ドット上へ（+8を削除）
    int cancelX = x + w - 80 - 8, cancelY = okY;
    m_tft->fillRect(okX, okY, btnW, btnH, 0x0010); // OK:暗い青 (0x0010)
    m_tft->setTextColor(0xFFFF, 0x0010);
    m_tft->setCursor(okX + 10, okY + 8);
    m_tft->printf("%s", okMsg);

    m_tft->fillRect(cancelX, cancelY, btnW, btnH, 0x8000); // Cancel:暗い赤 (0x8000)
    m_tft->setTextColor(0xFFFF, 0x8000);
    m_tft->setCursor(cancelX + 5, cancelY + 8);
    m_tft->printf("%s", cancelMsg);

    while (true) {
        if (m_ts->touched()) {
            TS_Point p = m_ts->getPointOnScreen();
            while (m_ts->touched()) {}
            // OKボタン
            if (p.x >= okX && p.x <= okX + btnW && p.y >= okY && p.y <= okY + btnH) {
                m_tft->popTextColor(); // テキストカラーを元に戻す
                return true;
            }
            // Cancelボタン
            if (p.x >= cancelX && p.x <= cancelX + btnW && p.y >= cancelY && p.y <= cancelY + btnH) {
				m_tft->popTextColor(); // テキストカラーを元に戻す
				return false;
			}
        }
        sleep_ms(10);
    }
}

void GUIMsgBox::showOK(int x, int y, const char* caption, const char* message, const char* okMsg) {
    const int w = 180, h = 116; // 高さ+10
    m_tft->fillRoundRect(x, y, w, h, 8, 0x7BEF);
    m_tft->drawRoundRect(x, y, w, h, 8, 0xFFFF);
    m_tft->setTextColor(0x0000, 0x7BEF);
    m_tft->setCursor(x + 10, y + 10);
    m_tft->printf("%s", caption);
    // キャプションとメッセージの間に8ドット空ける
    drawMultiline(m_tft, x + 10, y + 30 + 8, message, 0x0000, 0x7BEF);

    int btnW = 68, btnH = 28;
    int okX = x + w / 2 - btnW / 2, okY = y + h - 28; // 8ドット上へ（+8を削除）
    m_tft->fillRect(okX, okY, btnW, btnH, 0x0010); // OK:暗い青 (0x0010)
    m_tft->setTextColor(0xFFFF, 0x0010);
    m_tft->setCursor(okX + 10, okY + 8);
    m_tft->printf("%s", okMsg);

    while (true) {
        if (m_ts->touched()) {
            TS_Point p = m_ts->getPointOnScreen();
            while (m_ts->touched()) {}
            if (p.x >= okX && p.x <= okX + btnW && p.y >= okY && p.y <= okY + btnH) {
                break;
            }
        }
        sleep_ms(10);
    }
}
