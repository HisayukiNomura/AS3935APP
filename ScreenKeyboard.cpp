/**
 * @file ScreenKeyboard.cpp
 * @brief 画面スクリーンキーボードの表示・管理クラス実装
 * @details
 * QWERTY配列やテンキー配列のソフトウェアキーボードを画面上に表示し、
 * タッチ入力からキーコードを取得する機能を提供する。
 */
#include "ScreenKeyboard.h"
#include "pictData.h"
using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief QWERTY配列キーコードテーブル
 */
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
	 '`', ' ', ' ', 0x13, 0x14, 0x7E , 0x1B, ' ', 0x0f, 0x7F},
};
/**
 * @brief テンキー配列キーコードテーブル
 */
const uint8_t ScreenKeyboard::npCodes[ScreenKeyboard::NP_COUNT] = {
    '7', '8', '9', 0x1B,
    '4', '5', '6', 0x00,
    '1', '2', '3', 0x00,
    '0', 0x13,0x14,0x0D
};

/**
 * @brief ScreenKeyboardクラスのコンストラクタ
 * @details
 * ディスプレイ・タッチスクリーンのポインタを受け取り、初期キーボード種別を設定する。
 * @param tft ディスプレイ制御用ポインタ
 * @param ts タッチスクリーン制御用ポインタ
 */
ScreenKeyboard::ScreenKeyboard(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts)
    : m_tft(tft), m_ts(ts), m_kbType(KB1) {}

/**
 * @brief QWERTYキーボードを表示
 * @details
 * 指定Y座標に現在のキーボード種別の画像を描画し、内部状態を更新する。
 * @param y キーボード表示Y座標
 */
void ScreenKeyboard::show(int y) {
    const uint16_t* bmp = nullptr;
	keyboardY = y;
    m_kbMode = KM_QWERTY_1; // デフォルトはQWERTYキーボード
	switch (m_kbType) {
        case KB0: bmp = picKB0; break;
        case KB1: bmp = picKB1; break;
        case KB2: bmp = picKB2; break;
    }
    if (bmp) {
        m_tft->drawRGBBitmap(0, y, bmp, KB_WIDTH, KB_HEIGHT);
    }
    currWidth = KB_WIDTH;
    currHeight = KB_HEIGHT;
    currCols = KEY_COLS;
    currRows = KEY_ROWS;
    currSize = KEY_SIZE;
}

/**
 * @brief テンキー（NumPad）を表示
 * @details
 * 指定Y座標にテンキー画像を描画し、内部状態を更新する。
 * @param y テンキー表示Y座標
 */
void ScreenKeyboard::showNumPad(int y) {
	m_tft->drawRGBBitmap(0, y, picKBNum, NP_WIDTH, NP_HEIGHT);
	keyboardY = y;
    m_kbMode = KM_NUMPAD_1; // 数字キーボードモード
    currWidth = NP_WIDTH;
    currHeight = NP_HEIGHT;
    currCols = NP_COLS;
    currRows = NP_ROWS;
    currSize = NP_SIZE;
}

/**
 * @brief キーボード種別を設定
 * @param type キーボード種別
 */
void ScreenKeyboard::setKBType(KBType type) {
    m_kbType = type;
}

/**
 * @brief 現在のキーボード種別を取得
 * @return キーボード種別
 */
ScreenKeyboard::KBType ScreenKeyboard::getKBType() const {
    return m_kbType;
}

/**
 * @brief タッチ座標からキー番号を取得
 * @details
 * キーボード領域外の場合は-1を返す。
 * @param x タッチX座標
 * @param y タッチY座標
 * @return キー番号（-1:該当なし）
 */
int ScreenKeyboard::getKeyFromTouch(int x, int y) const {
    // キーボード領域外なら-1
    if (x < 0 || x >= currWidth || y < 0 || y >= currHeight) return -1;
    int col = x / currSize;
    int row = y / currSize;
    if (col >= currCols || row >= currRows) return -1;
    return row * currCols + col;
}

/**
 * @brief タッチ状態をチェックし、押されたキーコードを返す
 * @details
 * タッチされた位置からキーコードを取得し、特殊キーならキーボード種別を切り替える。
 * @return キーコード（0xFF:押されていない/領域外）
 */
uint8_t ScreenKeyboard::checkTouch() {
    if (!m_ts) return 0xFF;
    if (!m_ts->touched()) return 0xFF;
	TS_Point p = m_ts->getPointOnScreen();
	p.y = p.y - keyboardY;
    if (p.y <0 || p.y >= currHeight) return 0xFF; // キーボード領域外なら-1
    uint8_t code = 0xFF;
    if (m_kbMode == KM_NUMPAD_1) {
		code = npCodes[getKeyFromTouch(p.x, p.y)];
	} else {
		code = keycodes[m_kbType][getKeyFromTouch(p.x, p.y)];
	}
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



