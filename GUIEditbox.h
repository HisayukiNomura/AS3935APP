#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include "FlashMem.h"
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"
using namespace ardPort;
using namespace ardPort::spi;
class ScreenKeyboard; // 前方宣言

class GUIEditBox {
public:
    Adafruit_ILI9341* ptft; // ポインタを使用してAdafruit_ILI9341オブジェクトを参照
    XPT2046_Touchscreen* pts; // ポインタを使用してXPT2046_Touchscreenオブジェクトを参照
    ScreenKeyboard* psk; // ポインタを使用してScreenKeyboardオブジェクトを参照

	char* p = nullptr; // 編集対象の文字列ポインタ
	uint8_t edtCursor = 0;
	uint16_t edtX = 0;
	uint16_t edtY = 0;
	bool isInsert = false;

	GUIEditBox(Adafruit_GFX* a_ptft, XPT2046_Touchscreen* a_pts, ScreenKeyboard* a_psk);

	bool show(uint16_t a_x, uint16_t a_y, char* a_pText, size_t a_size);
	void dispCursor(int tick, bool isDeleteCursor = false);
	
};
