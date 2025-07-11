/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERW					E)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */
#include "misc/defines.h"
#include "Adafruit_GFX_Library/Adafruit_GFX.h"
#include "glcdfont.c"
#include "Kanji/KanjiHelper.h"
#include <cstdarg>
#ifdef MICROPY_BUILD_TYPE
extern "C" {
	#include "py/runtime.h" // MicroPython のランタイム機能
	#include "py/misc.h"    // メモリ管理関連の関数
	extern "C" void msg_OnDebug(const char* format, ...);
}
#endif

#ifdef STD_SDK
using namespace ardPort;
#endif

#ifdef __AVR__
	#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
	#include <pgmspace.h>
#endif

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
	#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#endif
#ifndef pgm_read_word
	#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#endif
#ifndef pgm_read_dword
	#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
	#define pgm_read_pointer(addr) ((void*)pgm_read_dword(addr))
#else
	#define pgm_read_pointer(addr) ((void*)pgm_read_word(addr))
#endif

inline GFXglyph* pgm_read_glyph_ptr(const GFXfont* gfxFont, uint8_t c)
{
#ifdef __AVR__
	return &(((GFXglyph*)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
	// expression in __AVR__ section may generate "dereferencing type-punned
	// pointer will break strict-aliasing rules" warning In fact, on other
	// platforms (such as STM32) there is no need to do this pointer magic as
	// program memory may be read in a usual way So expression may be simplified
	return gfxFont->glyph + c;
#endif //__AVR__
}

inline uint8_t* pgm_read_bitmap_ptr(const GFXfont* gfxFont)
{
#ifdef __AVR__
	return (uint8_t*)pgm_read_pointer(&gfxFont->bitmap);
#else
	// expression in __AVR__ section generates "dereferencing type-punned pointer
	// will break strict-aliasing rules" warning In fact, on other platforms (such
	// as STM32) there is no need to do this pointer magic as program memory may
	// be read in a usual way So expression may be simplified
	return gfxFont->bitmap;
#endif //__AVR__
}

#ifndef min
	#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
	#define _swap_int16_t(a, b) \
		{                       \
			int16_t t = a;      \
			a = b;              \
			b = t;              \
		}
#endif

#pragma region コンストラクタ
/*!
   @brief  デフォルトコンストラクタ。
   @details データ部分は別途constructObject()を呼び出して初期化する必要がある。
*/
Adafruit_GFX::Adafruit_GFX() :
	WIDTH(0),
	HEIGHT(0)
{
}
/**************************************************************************/
/*!
  @brief   グラフィックス用のGFXコンテキストを生成します（スーパークラス専用）。
  @param   w   ディスプレイの幅（ピクセル単位）
  @param   h   ディスプレイの高さ（ピクセル単位）
*/
/**************************************************************************/

Adafruit_GFX::Adafruit_GFX(int16_t w, int16_t h) :
	WIDTH(w),
	HEIGHT(h)
{
	_width = WIDTH;
	_height = HEIGHT;
	rotation = 0;
	cursor_y = cursor_x = 0;
	textsize_x = textsize_y = 1;
	textcolor = textbgcolor = 0xFFFF;
	wrap = true;
	_cp437 = false;
	gfxFont = NULL;
}
/*!
@bieif  データ部分を初期化する。
@param w   ディスプレイの幅（ピクセル単位）
@param h   ディスプレイの高さ（ピクセル単位）
@details デフォルトコンストラクタを使用してインスタンスを生成した場合は、必ずこの関数を呼び出す必要がある。
*/
void Adafruit_GFX::constructObject(int16_t w, int16_t h)
{
	WIDTH = w;
	HEIGHT = h;
	_width = w;
	_height = h;
	rotation = 0;
	cursor_y = cursor_x = 0;
	textsize_x = textsize_y = 1;
	textcolor = textbgcolor = 0xFFFF;
	wrap = true;
	_cp437 = false;
	gfxFont = NULL;
}
#pragma endregion

#pragma region 設定関連

/*!
  @brief   ディスプレイの回転設定を行います。
  @param   x   0～3の値で、4方向の回転を指定
*/

void Adafruit_GFX::setRotation(uint8_t x)
{
	rotation = (x & 3);
	switch (rotation) {
		case 0:
		case 2:
			_width = WIDTH;
			_height = HEIGHT;
			break;
		case 1:
		case 3:
			_width = HEIGHT;
			_height = WIDTH;
			break;
	}
}
/**
 * @brief 英文フォントを指定する。
 * @brief この設定が行われると、親クラスのisKanjiをFalseに戻し、英文フォントが使用されるようになる。
 * @param　f GFXfontオブジェクト。NULLを指定するとビルトインの6x8フォントが使用される。
 * @details フォント設定を日本語にするには、日本語フォントを指定してsetFontを呼びだす。
 */
void Adafruit_GFX::setFont(const GFXfont* f)
{
	if (f) {            // Font struct pointer passed in?
		if (!gfxFont) { // And no current font struct?
			// Switching from classic to new font behavior.
			// Move cursor pos down 6 pixels so it's on baseline.
			cursor_y += 6;
		}
	} else if (gfxFont == NULL) { // NULL passed.  Current font struct defined?
		// Switching from new to classic font behavior.
		// Move cursor pos up 6 pixels so it's at top-left of char.
		cursor_y -= 6;
	}
	gfxFont = (GFXfont*)f;
	isKanji = false;
}
/**
 * @brief 漢字フォントを指定する。
 * @brief この設定が行われると、親クラスのisKanjiをTrueにして、漢字フォントを使用するようになる。
 * @param　a_pKanjiData 漢字フォントのデータ
 * @param　a_pBmpData 漢字フォントのビットマップデータ
 * @details 元の英文に戻すには、英文フォントを指定してsetFontを呼びだすか、Print::KanjiMode(false)を呼び出す。
 */
void Adafruit_GFX::setFont(const KanjiData* a_pKanjiData, const uint8_t* a_pBmpData)
{
	KanjiHelper::SetKanjiFont(a_pKanjiData, a_pBmpData);
	isKanji = true;
}
#pragma endregion

#pragma region 特殊画面操作

/*!
  @brief   ディスプレイを反転表示にします（ハードウェアが対応している場合のみ）。
  @param   i  trueで反転、falseで通常表示
  @details 実際には、ハードウェア依存なのでAdafruit_ILI9341でオーバーロードされている
*/
void Adafruit_GFX::invertDisplay(bool i)
{
	// Do nothing, must be subclassed if supported by hardware
	(void)i; // disable -Wunused-parameter warning
}
#pragma endregion

#pragma region 描画機能
/*!
  @brief   線を描画します（Bresenhamのアルゴリズムを使用）。
  @param   x0  開始点のx座標
  @param   y0  開始点のy座標
  @param   x1  終了点のx座標
  @param   y1  終了点のy座標
  @param   color  描画色（16ビットRGB565）
  @details サブクラスで最適化する場合はオーバーライドしてください。
*/
void Adafruit_GFX::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
							 uint16_t color)
{
#if defined(ESP8266)
	yield();
#endif
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1) {
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0 <= x1; x0++) {
		if (steep) {
			writePixel(y0, x0, color);
		} else {
			writePixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}

/*!
  @brief   描画処理の開始。
  @details 実際にはAdafruit_SPITFTでオーバーライドされている
*/
void Adafruit_GFX::startWrite() {}

/*!
  @brief   ピクセルを描画します。startWriteを定義した場合はサブクラスでオーバーライドしてください。
  @param   x     x座標
  @param   y     y座標
  @param   color 描画色（16ビットRGB565）
*/

void Adafruit_GFX::writePixel(int16_t x, int16_t y, uint16_t color)
{
	drawPixel(x, y, color);
}

/*!
  @brief   垂直線を描画します。startWriteを定義した場合はサブクラスでオーバーライドしてください。
  @param   x     線のx座標
  @param   y     線の開始y座標
  @param   h     線の長さ（ピクセル数）
  @param   color 描画色（16ビットRGB565）
  @details デフォルトではdrawFastVLineを呼び出します。
*/

void Adafruit_GFX::writeFastVLine(int16_t x, int16_t y, int16_t h,
								  uint16_t color)
{
	// Overwrite in subclasses if startWrite is defined!
	// Can be just writeLine(x, y, x, y+h-1, color);
	// or writeFillRect(x, y, 1, h, color);
	drawFastVLine(x, y, h, color);
}

/*!
  @brief   水平線を描画します。startWriteを定義した場合はサブクラスでオーバーライドしてください。
  @param   x     線の開始x座標
  @param   y     線のy座標
  @param   w     線の長さ（ピクセル数）
  @param   color 描画色（16ビットRGB565）
  @details デフォルトではdrawFastHLineを呼び出します。
*/

void Adafruit_GFX::writeFastHLine(int16_t x, int16_t y, int16_t w,
								  uint16_t color)
{
	// Overwrite in subclasses if startWrite is defined!
	// Example: writeLine(x, y, x+w-1, y, color);
	// or writeFillRect(x, y, w, 1, color);
	drawFastHLine(x, y, w, color);
}

/*!
  @brief   矩形を塗りつぶします。startWriteを定義した場合はサブクラスでオーバーライドしてください。
  @param   x     左上x座標
  @param   y     左上y座標
  @param   w     幅（ピクセル数）
  @param   h     高さ（ピクセル数）
  @param   color 塗りつぶし色（16ビットRGB565）
  @details デフォルトではfillRectを呼び出します。
*/
void Adafruit_GFX::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
								 uint16_t color)
{
	// Overwrite in subclasses if desired!
	fillRect(x, y, w, h, color);
}

/*!
  @brief   描画処理の終了。startWriteを定義した場合はサブクラスでオーバーライドしてください。
*/
void Adafruit_GFX::endWrite() {}

/*!
  @brief   垂直線を描画します。
  @param   x   線のx座標（上端）
  @param   y   線のy座標（上端）
  @param   h   線の長さ（ピクセル数）
  @param   color 描画色（16ビットRGB565）
  @details サブクラスで最適化される場合があります。
*/

void Adafruit_GFX::drawFastVLine(int16_t x, int16_t y, int16_t h,
								 uint16_t color)
{
	startWrite();
	writeLine(x, y, x, y + h - 1, color);
	endWrite();
}

/*!
  @brief   水平線を描画します。
  @param   x   線のx座標（左端）
  @param   y   線のy座標（左端）
  @param   w   線の長さ（ピクセル数）
  @param   color 描画色（16ビットRGB565）
  @details サブクラスで最適化される場合があります。
*/

void Adafruit_GFX::drawFastHLine(int16_t x, int16_t y, int16_t w,
								 uint16_t color)
{
	startWrite();
	writeLine(x, y, x + w - 1, y, color);
	endWrite();
}

/*!
  @brief   矩形を塗りつぶします。
  @param   x   左上x座標
  @param   y   左上y座標
  @param   w   幅（ピクセル数）
  @param   h   高さ（ピクセル数）
  @param   color 塗りつぶし色（16ビットRGB565）
  @details サブクラスで最適化される場合があります。
*/
void Adafruit_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
							uint16_t color)
{
	startWrite();
	for (int16_t i = x; i < x + w; i++) {
		writeFastVLine(i, y, h, color);
	}
	endWrite();
}

/*!
  @brief   画面全体を指定色で塗りつぶします。
  @param   color 塗りつぶし色（16ビットRGB565）
*/

void Adafruit_GFX::fillScreen(uint16_t color)
{
	fillRect(0, 0, _width, _height, color);
}

/*!
  @brief   線分を描画します。
  @param   x0  開始点x座標
  @param   y0  開始点y座標
  @param   x1  終了点x座標
  @param   y1  終了点y座標
  @param   color 描画色（16ビットRGB565）
  @details 垂直線・水平線の場合は高速描画関数を使用します。
*/
void Adafruit_GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
							uint16_t color)
{
	// Update in subclasses if desired!
	if (x0 == x1) {
		if (y0 > y1)
			_swap_int16_t(y0, y1);
		drawFastVLine(x0, y0, y1 - y0 + 1, color);
	} else if (y0 == y1) {
		if (x0 > x1)
			_swap_int16_t(x0, x1);
		drawFastHLine(x0, y0, x1 - x0 + 1, color);
	} else {
		startWrite();
		writeLine(x0, y0, x1, y1, color);
		endWrite();
	}
}

/*!
  @brief   円の輪郭を描画します。
  @param   x0   中心x座標
  @param   y0   中心y座標
  @param   r    半径
  @param   color 描画色（16ビットRGB565）
*/

void Adafruit_GFX::drawCircle(int16_t x0, int16_t y0, int16_t r,
							  uint16_t color)
{
#if defined(ESP8266)
	yield();
#endif
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	startWrite();
	writePixel(x0, y0 + r, color);
	writePixel(x0, y0 - r, color);
	writePixel(x0 + r, y0, color);
	writePixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		writePixel(x0 + x, y0 + y, color);
		writePixel(x0 - x, y0 + y, color);
		writePixel(x0 + x, y0 - y, color);
		writePixel(x0 - x, y0 - y, color);
		writePixel(x0 + y, y0 + x, color);
		writePixel(x0 - y, y0 + x, color);
		writePixel(x0 + y, y0 - x, color);
		writePixel(x0 - y, y0 - x, color);
	}
	endWrite();
}

/*!
  @brief   円の一部（1/4）を描画します。
  @param   x0   中心x座標
  @param   y0   中心y座標
  @param   r    半径
  @param   cornername  描画するクォーターの指定ビット
  @param   color 描画色（16ビットRGB565）
  @details 円や角丸矩形の描画に利用されます。
*/

void Adafruit_GFX::drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
									uint8_t cornername, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			writePixel(x0 + x, y0 + y, color);
			writePixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			writePixel(x0 + x, y0 - y, color);
			writePixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			writePixel(x0 - y, y0 + x, color);
			writePixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			writePixel(x0 - y, y0 - x, color);
			writePixel(x0 - x, y0 - y, color);
		}
	}
}

/*!
  @brief   円を塗りつぶします。
  @param   x0   中心x座標
  @param   y0   中心y座標
  @param   r    半径
  @param   color 塗りつぶし色（16ビットRGB565）
*/

void Adafruit_GFX::fillCircle(int16_t x0, int16_t y0, int16_t r,
							  uint16_t color)
{
	startWrite();
	writeFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
	endWrite();
}

/*!
  @brief   塗りつぶし付きクォーターサークルを描画します。
  @param   x0      中心x座標
  @param   y0      中心y座標
  @param   r       半径
  @param   corners 描画するクォーターの指定ビット
  @param   delta   オフセット（角丸矩形用）
  @param   color   塗りつぶし色（16ビットRGB565）
  @details 円や角丸矩形の塗りつぶしに利用されます。
*/

void Adafruit_GFX::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
									uint8_t corners, int16_t delta,
									uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
	int16_t px = x;
	int16_t py = y;

	delta++; // Avoid some +1's in the loop

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		// These checks avoid double-drawing certain lines, important
		// for the SSD1306 library which has an INVERT drawing mode.
		if (x < (y + 1)) {
			if (corners & 1)
				writeFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
			if (corners & 2)
				writeFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
		}
		if (y != py) {
			if (corners & 1)
				writeFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
			if (corners & 2)
				writeFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
			py = y;
		}
		px = x;
	}
}

/*!
  @brief   枠のみの矩形を描画します。
  @param   x   左上x座標
  @param   y   左上y座標
  @param   w   幅（ピクセル数）
  @param   h   高さ（ピクセル数）
  @param   color 描画色（16ビットRGB565）
*/

void Adafruit_GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
							uint16_t color)
{
	startWrite();
	writeFastHLine(x, y, w, color);
	writeFastHLine(x, y + h - 1, w, color);
	writeFastVLine(x, y, h, color);
	writeFastVLine(x + w - 1, y, h, color);
	endWrite();
}

/*!
  @brief   角丸矩形（枠のみ）を描画します。
  @param   x   左上x座標
  @param   y   左上y座標
  @param   w   幅（ピクセル数）
  @param   h   高さ（ピクセル数）
  @param   r   角の半径
  @param   color 描画色（16ビットRGB565）
*/

void Adafruit_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
								 int16_t r, uint16_t color)
{
	int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	startWrite();
	writeFastHLine(x + r, y, w - 2 * r, color);         // Top
	writeFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
	writeFastVLine(x, y + r, h - 2 * r, color);         // Left
	writeFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
	// draw four corners
	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
	endWrite();
}

/*!
  @brief   角丸矩形（塗りつぶし）を描画します。
  @param   x   左上x座標
  @param   y   左上y座標
  @param   w   幅（ピクセル数）
  @param   h   高さ（ピクセル数）
  @param   r   角の半径
  @param   color 塗りつぶし色（16ビットRGB565）
*/

void Adafruit_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
								 int16_t r, uint16_t color)
{
	int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
	if (r > max_radius)
		r = max_radius;
	// smarter version
	startWrite();
	writeFillRect(x + r, y, w - 2 * r, h, color);
	// draw four corners
	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
	endWrite();
}

/**************************************************************************/
/*!
  @brief   三角形（枠のみ）を描画します。
  @param   x0  頂点0のx座標
  @param   y0  頂点0のy座標
  @param   x1  頂点1のx座標
  @param   y1  頂点1のy座標
  @param   x2  頂点2のx座標
  @param   y2  頂点2のy座標
  @param   color 描画色（16ビットRGB565）
*/
/**************************************************************************/
void Adafruit_GFX::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
								int16_t x2, int16_t y2, uint16_t color)
{
	drawLine(x0, y0, x1, y1, color);
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
  @brief   三角形（塗りつぶし）を描画します。
  @param   x0  頂点0のx座標
  @param   y0  頂点0のy座標
  @param   x1  頂点1のx座標
  @param   y1  頂点1のy座標
  @param   x2  頂点2のx座標
  @param   y2  頂点2のy座標
  @param   color 塗りつぶし色（16ビットRGB565）
  @details Y座標でソートし、走査線ごとに水平線を描画します。
*/
/**************************************************************************/
void Adafruit_GFX::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
								int16_t x2, int16_t y2, uint16_t color)
{
	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1) {
		_swap_int16_t(y0, y1);
		_swap_int16_t(x0, x1);
	}
	if (y1 > y2) {
		_swap_int16_t(y2, y1);
		_swap_int16_t(x2, x1);
	}
	if (y0 > y1) {
		_swap_int16_t(y0, y1);
		_swap_int16_t(x0, x1);
	}

	startWrite();
	if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)
			a = x1;
		else if (x1 > b)
			b = x1;
		if (x2 < a)
			a = x2;
		else if (x2 > b)
			b = x2;
		writeFastHLine(a, y0, b - a + 1, color);
		endWrite();
		return;
	}

	int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
			dx12 = x2 - x1, dy12 = y2 - y1;
	int32_t sa = 0, sb = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2)
		last = y1; // Include y1 scanline
	else
		last = y1 - 1; // Skip it

	for (y = y0; y <= last; y++) {
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b)
			_swap_int16_t(a, b);
		writeFastHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = (int32_t)dx12 * (y - y1);
	sb = (int32_t)dx02 * (y - y0);
	for (; y <= y2; y++) {
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b)
			_swap_int16_t(a, b);
		writeFastHLine(a, y, b - a + 1, color);
	}
	endWrite();
}
#pragma endregion

#pragma region ビットマップ機能

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/*!
  @brief   指定座標に1ビットビットマップ画像を描画します。()
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap モノクロビットマップ配列(プログラムメモリ)
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @param   color  描画色（16ビットRGB565）
  @details ビットが0の部分は透明として描画されません。
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
							  int16_t w, int16_t h, uint16_t color)
{
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
			if (b & 0x80)
				writePixel(x + i, y, color);
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に1ビットビットマップ画像を描画します（前景色・背景色）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap モノクロビットマップ配列(プログラムメモリ)
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @param   color  描画色（16ビットRGB565）
  @param   bg     背景色（16ビットRGB565）
  @details ビットが0の部分も背景色で塗りつぶします。
*/

void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
							  int16_t w, int16_t h, uint16_t color,
							  uint16_t bg)
{
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
			writePixel(x + i, y, (b & 0x80) ? color : bg);
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に1ビットビットマップ画像を描画します
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap モノクロビットマップ配列(RAM)
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @param   color  描画色（16ビットRGB565）
  @details ビットが0の部分は透明として描画されません。
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, uint8_t* bitmap, int16_t w,
							  int16_t h, uint16_t color)
{
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = bitmap[j * byteWidth + i / 8];
			if (b & 0x80)
				writePixel(x + i, y, color);
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に1ビットビットマップ画像を描画します
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap モノクロビットマップ配列(RAM)
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @param   color  描画色（16ビットRGB565）
  @param   bg     背景色（16ビットRGB565）
  @details ビットが0の部分も背景色で塗りつぶします。
*/
void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, uint8_t* bitmap, int16_t w,
							  int16_t h, uint16_t color, uint16_t bg)
{
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = bitmap[j * byteWidth + i / 8];
			writePixel(x + i, y, (b & 0x80) ? color : bg);
		}
	}
	endWrite();
}

/*!
  @brief   GIMPからエクスポートしたXBM形式のビットマップ画像を描画します（PROGMEM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap モノクロビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @param   color  描画色（16ビットRGB565）
  @details RAM格納版はありません。RAMの場合はdrawBitmap()を使用してください。
*/

void Adafruit_GFX::drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
							   int16_t w, int16_t h, uint16_t color)
{
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
	uint8_t b = 0;

	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b >>= 1;
			else
				b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
			// Nearly identical to drawBitmap(), only the bit order
			// is reversed here (left-to-right = LSB to MSB):
			if (b & 0x01)
				writePixel(x + i, y, color);
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に8ビットグレースケール画像を描画します（PROGMEM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap グレースケールビットマップ配列(プログラムメモリ）
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details 8ビットディスプレイ向け。色変換は行いません。
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
									   const uint8_t bitmap[], int16_t w,
									   int16_t h)
{
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に8ビットグレースケール画像を描画します（RAM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap グレースケールビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details 8ビットディスプレイ向け。色変換は行いません。
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t* bitmap,
									   int16_t w, int16_t h)
{
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, bitmap[j * w + i]);
		}
	}
	endWrite();
}

/*!
  @brief   1ビットマスク付き8ビットグレースケール画像を描画します（PROGMEM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap グレースケールビットマップ配列
  @param   mask   マスクビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details マスクのビットが1の部分のみ描画します。
*/
void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y,
									   const uint8_t bitmap[],
									   const uint8_t mask[], int16_t w,
									   int16_t h)
{
	int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
	uint8_t b = 0;
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = pgm_read_byte(&mask[j * bw + i / 8]);
			if (b & 0x80) {
				writePixel(x + i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
			}
		}
	}
	endWrite();
}

/*!
  @brief   1ビットマスク付き8ビットグレースケール画像を描画します（RAM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap グレースケールビットマップ配列
  @param   mask   マスクビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details マスクのビットが1の部分のみ描画します。
*/

void Adafruit_GFX::drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t* bitmap,
									   uint8_t* mask, int16_t w, int16_t h)
{
	int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
	uint8_t b = 0;
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = mask[j * bw + i / 8];
			if (b & 0x80) {
				writePixel(x + i, y, bitmap[j * w + i]);
			}
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に16ビットRGB565画像を描画します（PROGMEM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap 16ビットカラーのワードマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details 16ビットディスプレイ向け。色変換は行いません。
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
								 int16_t w, int16_t h)
{
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, pgm_read_word(&bitmap[j * w + i]));
		}
	}
	endWrite();
}

/*!
  @brief   指定座標に16ビットRGB565画像を描画します（RAM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap 16ビットカラーのワードマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details 16ビットディスプレイ向け。色変換は行いません。
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, uint16_t* bitmap,
								 int16_t w, int16_t h)
{
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, bitmap[j * w + i]);
		}
	}
	endWrite();
}

uint16_t Adafruit_GFX::convert8To565(uint8_t color8)
{
	// 8ビットのRRRGGGBBフォーマットをRGB565に直接マッピング
	uint8_t red = (color8 & 0b11100000) >> 5;   // 赤: 上位3ビット
	uint8_t green = (color8 & 0b00011100) >> 2; // 緑: 中位3ビット
	uint8_t blue = (color8 & 0b00000011);       // 青: 下位2ビット

	// ビットスケールの拡張（リニアマッピングで最大値を考慮）
	uint16_t r5 = (red * 0x1F) / 0x07;   // 赤: 3ビット -> 5ビット
	uint16_t g6 = (green * 0x3F) / 0x07; // 緑: 3ビット -> 6ビット
	uint16_t b5 = (blue * 0x1F) / 0x03;  // 青: 2ビット -> 5ビット
	uint16_t col = (r5 << 11) | (g6 << 5) | b5;
	return col; // RGB565フォーマットを生成
}
/**************************************************************************/
/*!
	@brief  8bitカラーのビットマップイメージを転送する。この処理は、１ドットづつの描画なので非常に遅い。
	@param    x   Top left corner x coordinate
	@param    y   Top left corner y coordinate
	@param    bitmap  byte array with 16-bit color bitmap
	@param    w   Width of bitmap in pixels
	@param    h   Height of bitmap in pixels
	@details ウインドウ機能があるTFTについては、Adafruit_SPITFT.cpp側にオーバーロード関数を用意するほうが良い
*/
/**************************************************************************/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
								 int16_t w, int16_t h)
{
	startWrite();
	int idx = 0;
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, convert8To565(pgm_read_word(&bitmap[j * w + i])));
		}
	}
	endWrite();
}

/**************************************************************************/
/*!
	@brief  8bitカラーのビットマップイメージを転送する。この処理は、１ドットづつの描画なので非常に遅い。
	@param    x   Top left corner x coordinate
	@param    y   Top left corner y coordinate
	@param    bitmap  byte array with 16-bit color bitmap
	@param    w   Width of bitmap in pixels
	@param    h   Height of bitmap in pixels
	@details ウインドウ機能があるTFTについては、Adafruit_SPITFT.cpp側にオーバーロード関数を用意するほうが良い
*/
/**************************************************************************/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, uint8_t* bitmap,
								 int16_t w, int16_t h)
{
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			writePixel(x + i, y, convert8To565(bitmap[j * w + i]));
		}
	}
	endWrite();
}

/*!
  @brief   1ビットマスク付き16ビットRGB565画像を描画します（PROGMEM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap 16ビットカラーのビットマップ配列
  @param   mask   モノクロマスクビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details カラービットマップとマスクの両方がPROGMEM格納である必要があります。マスクのビットが1の部分のみ描画します。色変換は行いません。
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
								 const uint8_t mask[], int16_t w, int16_t h)
{
	int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
	uint8_t b = 0;
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = pgm_read_byte(&mask[j * bw + i / 8]);
			if (b & 0x80) {
				writePixel(x + i, y, pgm_read_word(&bitmap[j * w + i]));
			}
		}
	}
	endWrite();
}

/*!
  @brief   1ビットマスク付き16ビットRGB565画像を描画します（RAM格納）。
  @param   x      左上x座標
  @param   y      左上y座標
  @param   bitmap 16ビットカラーのビットマップ配列
  @param   mask   モノクロマスクビットマップ配列
  @param   w      幅（ピクセル数）
  @param   h      高さ（ピクセル数）
  @details カラービットマップとマスクの両方がRAM格納である必要があります。マスクのビットが1の部分のみ描画します。色変換は行いません。
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, uint16_t* bitmap,
								 uint8_t* mask, int16_t w, int16_t h)
{
	int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
	uint8_t b = 0;
	startWrite();
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				b <<= 1;
			else
				b = mask[j * bw + i / 8];
			if (b & 0x80) {
				writePixel(x + i, y, bitmap[j * w + i]);
			}
		}
	}
	endWrite();
}

/*!
	@brief  透過色を指定してビットマップを描画する。
			透過色は、描画するビットマップの中に含まれる色で、描画しない色。
			この場合は、ウインドウを使えないので１ドットづつ書いていくしか方法がない。
	@param  x        Top left corner horizontal coordinate.
	@param  y        Top left corner vertical coordinate.
	@param  pcolors  表示する画像データのポインタ。画像データはワードマップ
	@param  w        Width of bitmap in pixels.
	@param  h        Height of bitmap in pixels.
	@param  colorTransparent  透過色
*/
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, uint16_t* pcolors, int16_t w, int16_t h, uint16_t colorTransparent)
{
	startWrite();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			uint32_t pictIdx = i * w + j;
			uint16_t color = pcolors[pictIdx];
			if (color != colorTransparent) {
				writePixel(x + j, y + i, color);
			}
		}
	}
	endWrite();
}

/// @brief Canvas16を使ったビットマップ描画。透過色をサポートする
/// @param x　描画するX座標
/// @param y 	描画するY座標
/// @param pCanvas 	描画するGFXcanvas16のポインタ　
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, GFXcanvas16* pCanvas)
{
	startWrite();
	uint16_t w = pCanvas->width();
	uint16_t h = pCanvas->height();
	uint16_t* bitmap = pCanvas->getBuffer();
	bool p = pCanvas->isBackground;
	uint16_t bg = pCanvas->bckColor;

	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (p && (bitmap[j * w + i] == bg)) {
				// 透明色を指定している場合は、背景色と同じ色のドットは描画しない
			} else {
				writePixel(x + i, y, bitmap[j * w + i]);
			}
		}
	}
	endWrite();
}

/// @brief Canvas16を使ったビットマップ描画。透過色をサポートする
/// @param x　描画するX座標
/// @param y  描画するY座標
/// @param pCanvas 	描画するGFXcanvas16のポインタ　
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, GFXcanvas8* pCanvas)
{
	startWrite();
	uint16_t w = pCanvas->width();
	uint16_t h = pCanvas->height();
	uint8_t* bitmap = pCanvas->getBuffer();
	bool p = pCanvas->isBackground;
	uint8_t bg = pCanvas->bckColor;

	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (p && (bitmap[j * w + i] == bg)) {
				// 透明色を指定している場合は、背景色と同じ色のドットは描画しない
			} else {
				writePixel(x + i, y, convert8To565(bitmap[j * w + i]));
			}
		}
	}
	endWrite();
}

/// @brief Canvas16を使ったビットマップ描画。透過色をサポートする
/// @param x　描画するX座標
/// @param y  描画するY座標
/// @param pCanvas 	描画するGFXcanvas16のポインタ　
void Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, GFXcanvas1* pCanvas)
{
	startWrite();
	uint16_t w = pCanvas->width();
	uint16_t h = pCanvas->height();
	uint8_t* bitmap = pCanvas->getBuffer();
	bool p = pCanvas->isBackground;

	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (p && pCanvas->getPixel(i, j) == false) {
				// 透明色を指定している場合は、背景色と同じ色のドットは描画しない
			} else {
				if (pCanvas->getPixel(i, j)) {
					writePixel(x + i, y, 0xFFFF);
				} else {
					writePixel(x + i, y, 0x0000);
				}
			}
		}
	}
	endWrite();
}
#pragma endregion

#pragma region 文字描画機能
// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

// Draw a character
/*!
  @brief   1文字を描画します。
  @param   x   左下隅のx座標
  @param   y   左下隅のy座標
  @param   c   8ビットのフォントインデックス文字（通常はASCII）
  @param   color  描画色（16ビットRGB565）
  @param   bg     背景色（16ビットRGB565、colorと同じ場合は背景なし）
  @param   size   フォント拡大率（1で等倍）
*/
void Adafruit_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
							uint16_t color, uint16_t bg, uint8_t size)
{
	drawChar(x, y, c, color, bg, size, size);
}

// Draw a character
/*!
  @brief   1文字を描画します。
  @param   x   左下隅のx座標
  @param   y   左下隅のy座標
  @param   c   8ビットのフォントインデックス文字（通常はASCII）
  @param   color  描画色（16ビットRGB565）
  @param   bg     背景色（16ビットRGB565、colorと同じ場合は背景なし）
  @param   size_x 横方向の拡大率（1で等倍）
  @param   size_y 縦方向の拡大率（1で等倍）
  @details カスタムフォントの場合、背景色はサポートされません。背景を消したい場合はgetTextBounds()で範囲を取得しfillRect()で消去してください。
*/
void Adafruit_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
							uint16_t color, uint16_t bg, uint8_t size_x,
							uint8_t size_y)
{
	if (!gfxFont) { // 'Classic' built-in font

		if ((x >= _width) ||              // Clip right
			(y >= _height) ||             // Clip bottom
			((x + 6 * size_x - 1) < 0) || // Clip left
			((y + 8 * size_y - 1) < 0))   // Clip top
			return;

		if (!_cp437 && (c >= 176))
			c++; // Handle 'classic' charset behavior

		startWrite();
		for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
			uint8_t line = pgm_read_byte(&font[c * 5 + i]);
			for (int8_t j = 0; j < 8; j++, line >>= 1) {
				if (line & 1) {
					if (size_x == 1 && size_y == 1)
						writePixel(x + i, y + j, color);
					else
						writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y,
									  color);
				} else if (bg != color) {
					if (size_x == 1 && size_y == 1)
						writePixel(x + i, y + j, bg);
					else
						writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
				}
			}
		}
		if (bg != color) { // If opaque, draw vertical line for last column
			if (size_x == 1 && size_y == 1)
				writeFastVLine(x + 5, y, 8, bg);
			else
				writeFillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
		}
		endWrite();

	} else { // Custom font

		// Character is assumed previously filtered by write() to eliminate
		// newlines, returns, non-printable characters, etc.  Calling
		// drawChar() directly with 'bad' characters of font may cause mayhem!

		c -= (uint8_t)pgm_read_byte(&gfxFont->first);
		GFXglyph* glyph = pgm_read_glyph_ptr(gfxFont, c);
		uint8_t* bitmap = pgm_read_bitmap_ptr(gfxFont);

		uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
		uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
		int8_t xo = pgm_read_byte(&glyph->xOffset),
			   yo = pgm_read_byte(&glyph->yOffset);
		uint8_t xx, yy, bits = 0, bit = 0;
		int16_t xo16 = 0, yo16 = 0;

		if (size_x > 1 || size_y > 1) {
			xo16 = xo;
			yo16 = yo;
		}

		// Todo: Add character clipping here

		// NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
		// THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
		// has typically been used with the 'classic' font to overwrite old
		// screen contents with new data.  This ONLY works because the
		// characters are a uniform size; it's not a sensible thing to do with
		// proportionally-spaced fonts with glyphs of varying sizes (and that
		// may overlap).  To replace previously-drawn text when using a custom
		// font, use the getTextBounds() function to determine the smallest
		// rectangle encompassing a string, erase the area with fillRect(),
		// then draw new text.  This WILL infortunately 'blink' the text, but
		// is unavoidable.  Drawing 'background' pixels will NOT fix this,
		// only creates a new set of problems.  Have an idea to work around
		// this (a canvas object type for MCUs that can afford the RAM and
		// displays supporting setAddrWindow() and pushColors()), but haven't
		// implemented this yet.

		startWrite();
		for (yy = 0; yy < h; yy++) {
			for (xx = 0; xx < w; xx++) {
				if (!(bit++ & 7)) {
					bits = pgm_read_byte(&bitmap[bo++]);
				}
				if (bits & 0x80) {
					if (size_x == 1 && size_y == 1) {
						writePixel(x + xo + xx, y + yo + yy, color);
					} else {
						writeFillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
									  size_x, size_y, color);
					}
				}
				bits <<= 1;
			}
		}
		endWrite();

	} // End classic vs custom font
}

/// @brief 画面に漢字を１文字表示する。仮想関数なので、Adafruit_GFXを継承したクラスでオーバーロードされる可能性がある。
/// @param   x   描画開始位置の左上座標
/// @param   y   描画開始位置の左上座標
/// @param   w   描画するフォントビットマップの横ドット数
/// @param   h   描画するフォントビットマップの縦ドット数
/// @param   bmpData  描画するフォントビットマップデータ
/// @param   color 文字の前景色（565カラー）
/// @param   bg 文字の背景色（565カラー）　前景色と同じ色が指定されたら、透過表示とみなす。
/// @param   size_x 文字の横方向の拡大率
/// @param   size_y 文字の縦方向の拡大率
/// @details ここで定義されるのは、最も汎用的になると思われる実装。例えば、ILI9341の場合（おそらくTFT7735も）、描画ウインドウを指定することでより高速に表示できる。
/// 必要に応じて、このクラスを継承するクラスでは、より効率的な実装を行うべき。
/// このライブラリでは、Adafruit_ILI9341.cppなどで、drawCharをオーバーロードしている。
void Adafruit_GFX::drawChar(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t* bmpData, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y)
{
	// 表示するデータの上下左右が、表示可能領域を超えていたら何もしない。クリッピング処理
	// 元々の判断そのままだが、すこしオカシイ気がする。
	// たとえば、表示領域を右にはみ出さないかの判断を、 x > _width　としているが、これだと描画開始地点（文字の左）が横幅を超えるかの判断しかしていない。
	// 本来、表示領域の右と、表示する文字の右側を比較する必要があるのでは？ x > _width ではなく、 (x+w) >= _width と判断すべきでは？
	if ((x >= _width) || (y >= _height) || ((x + w * size_x - 1) < 0) || ((y + h * size_y - 1) < 0)) return;

	uint8_t w_bytes = (w + 8 - 1) / 8;  // 横方向のバイト数
	uint8_t h_bytes = h;                // 縦方向のバイト数
	uint16_t bmpIdx = 0;                // ビットマップ情報には、bmp + yy*w_bytes + xx でアクセスできるが、順番に並んでいるので最初から順に読むほうが速いのでは？
	bool isByteMultiple = (w % 8 == 0); // 横幅が8の倍数かのフラグ。横１２ドットなどの場合は、８の倍数にならないので調整が必要になる

	startWrite();

	for (int8_t yy = 0; yy < h_bytes; yy++) {
		for (int8_t xx = 0; xx < w_bytes; xx++) {
			uint8_t bitCnt;
			if (isByteMultiple) {
				bitCnt = 8;
			} else {
				bitCnt = (xx != (w_bytes - 1)) ? 8 : (w % 8);
			}
			uint8_t bits = bmpData[bmpIdx];
			for (int8_t bb = 0; bb < bitCnt; bb++) {
				if (bits & 0x80) {
					if (size_x == 1 && size_y == 1)
						writePixel(x + xx * 8 + bb, y + yy, color);
					else
						writeFillRect(x + (xx * 8 + bb) * size_x, y + yy * size_y, size_x, size_y, color);
				} else {
					if (color != bg) { // 前景色と背景色が同じときは、透過色として背景色は描画しない。
						if (size_x == 1 && size_y == 1)
							writePixel(x + xx * 8 + bb, y + yy, bg);
						else
							writeFillRect(x + (xx * 8 + bb) * size_x, y + yy * size_y, size_x, size_y, bg);
					}
				}

				bits <<= 1;
			}
			bmpIdx++;
		}
	}
	if (bg != color) { // If opaque, draw vertical line for last column
		if (size_x == 1 && size_y == 1)
			writeFastVLine(x + w, y, h, bg);
		else
			writeFillRect(x + w * size_x, y, size_x, h * size_y, bg);
	}
	endWrite();
}

/*!
	@brief  ASCII１文字を画面に表示する。printなどで使用される
	@param  c  ８ビットのアスキー文字
*/

size_t Adafruit_GFX::write(uint8_t c)
{
	if (!gfxFont) { // 'Classic' built-in font

		if (c == '\n') {                                          // Newline?
			cursor_x = 0;                                         // Reset x to zero,
			cursor_y += textsize_y * 8;                           // advance y one line
		} else if (c != '\r') {                                   // Ignore carriage returns
			if (wrap && ((cursor_x + textsize_x * 6) > _width)) { // Off right?
				cursor_x = 0;                                     // Reset x to zero,
				cursor_y += textsize_y * 8;                       // advance y one line
			}
			drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
					 textsize_y);
			cursor_x += textsize_x * 6; // Advance x one char
		}

	} else { // Custom font

		if (c == '\n') {
			cursor_x = 0;
			cursor_y +=
				(int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
		} else if (c != '\r') {
			uint8_t first = pgm_read_byte(&gfxFont->first);
			if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
				GFXglyph* glyph = pgm_read_glyph_ptr(gfxFont, c - first);
				uint8_t w = pgm_read_byte(&glyph->width),
						h = pgm_read_byte(&glyph->height);
				if ((w > 0) && (h > 0)) {                                // Is there an associated bitmap?
					int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
					if (wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
						cursor_x = 0;
						cursor_y += (int16_t)textsize_y *
									(uint8_t)pgm_read_byte(&gfxFont->yAdvance);
					}
					drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
							 textsize_y);
				}
				cursor_x +=
					(uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
			}
		}
	}
	return 1;
}
/// @brief 32bitで渡された文字（UTF)を表示する。
/// @param utf8Code 表示するUTF-8文字
/// @return 表示された文字数。常に１
size_t Adafruit_GFX::write(uint32_t utf8Code)
{

	if (utf8Code == 0x0000000A) {
		cursor_x = 0;
		cursor_y += KanjiHelper::getKanjiWidth() * textsize_y; // 改行
	} else if (utf8Code != 0x0000000D) {
		const uint8_t* bmpData;
		uint8_t w;
		uint8_t h;
		if (utf8Code <= 0xFF) { // １バイト文字
			const KanjiData* pFont = KanjiHelper::FindAscii(utf8Code);
			if (pFont == NULL) {
				cursor_x = cursor_x + KanjiHelper::getAsciiWidth();
				return 1;
			}
			bmpData = KanjiHelper::getBmpData(pFont);
			w = pFont->width;
			h = pFont->height;
		} else {
			const KanjiData* pFont = KanjiHelper::FindKanji(utf8Code);
			if (pFont == NULL) {
				cursor_x = cursor_x + KanjiHelper::getKanjiWidth();
				return 1;
			}
			bmpData = KanjiHelper::getBmpData(pFont);
			w = pFont->width;
			h = pFont->height;
		}
		if (wrap) {
			if ((cursor_x + w) > _width) {
				cursor_x = 0;
				cursor_y = cursor_y + h * textsize_y;
			}
		}
		drawChar(cursor_x, cursor_y, w, h, bmpData, textcolor, textbgcolor, textsize_x, textsize_y);
		cursor_x = cursor_x + w * textsize_x;
		return 1;
	}

	return 0;
}
/*!
	@brief  文字の倍率を設定する
   that much bigger.
	@param  s  期待されるテキストサイズ。１がデフォルト、２が２倍、３が３倍
*/
void Adafruit_GFX::setTextSize(uint8_t s)
{
	setTextSize(s, s);
}

/*!
  @brief   テキストの拡大率を設定します。
  @param   s_x  横方向の拡大率（1が標準）
  @param   s_y  縦方向の拡大率（1が標準）
*/
void Adafruit_GFX::setTextSize(uint8_t s_x, uint8_t s_y)
{
	textsize_x = (s_x > 0) ? s_x : 1;
	textsize_y = (s_y > 0) ? s_y : 1;
}

/*!
  @brief   現在のフォント・サイズで1文字のサイズを取得します。
  @param   c     対象のASCII文字
  @param   x     文字のx座標（次の文字位置に更新されます）
  @param   y     文字のy座標（次の文字位置に更新されます）
  @param   minx  最小X座標（更新されます）
  @param   miny  最小Y座標（更新されます）
  @param   maxx  最大X座標（更新されます）
  @param   maxy  最大Y座標（更新されます）
  @details 文字列のバウンディングボックス計算に利用されます。
*/

void Adafruit_GFX::charBounds(unsigned char c, int16_t* x, int16_t* y,
							  int16_t* minx, int16_t* miny, int16_t* maxx,
							  int16_t* maxy)
{
	if (gfxFont) {
		if (c == '\n') { // Newline?
			*x = 0;      // Reset x to zero, advance y by one line
			*y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
		} else if (c != '\r') { // Not a carriage return; is normal char
			uint8_t first = pgm_read_byte(&gfxFont->first),
					last = pgm_read_byte(&gfxFont->last);
			if ((c >= first) && (c <= last)) { // Char present in this font?
				GFXglyph* glyph = pgm_read_glyph_ptr(gfxFont, c - first);
				uint8_t gw = pgm_read_byte(&glyph->width),
						gh = pgm_read_byte(&glyph->height),
						xa = pgm_read_byte(&glyph->xAdvance);
				int8_t xo = pgm_read_byte(&glyph->xOffset),
					   yo = pgm_read_byte(&glyph->yOffset);
				if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > _width)) {
					*x = 0; // Reset x to zero, advance y by one line
					*y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
				}
				int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
						x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
						y2 = y1 + gh * tsy - 1;
				if (x1 < *minx)
					*minx = x1;
				if (y1 < *miny)
					*miny = y1;
				if (x2 > *maxx)
					*maxx = x2;
				if (y2 > *maxy)
					*maxy = y2;
				*x += xa * tsx;
			}
		}

	} else { // Default font

		if (c == '\n') {                                    // Newline?
			*x = 0;                                         // Reset x to zero,
			*y += textsize_y * 8;                           // advance y one line
															// min/max x/y unchaged -- that waits for next 'normal' character
		} else if (c != '\r') {                             // Normal char; ignore carriage returns
			if (wrap && ((*x + textsize_x * 6) > _width)) { // Off right?
				*x = 0;                                     // Reset x to zero,
				*y += textsize_y * 8;                       // advance y one line
			}
			int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
				y2 = *y + textsize_y * 8 - 1;
			if (x2 > *maxx)
				*maxx = x2; // Track max x, y
			if (y2 > *maxy)
				*maxy = y2;
			if (*x < *minx)
				*minx = *x; // Track min x, y
			if (*y < *miny)
				*miny = *y;
			*x += textsize_x * 6; // Advance x one char
		}
	}
}

/*!
  @brief   現在のフォント・サイズで文字列のバウンディングボックスを取得します。
  @param   str  対象のASCII文字列
  @param   x    カーソルのX座標
  @param   y    カーソルのY座標
  @param   x1   バウンディングボックスの左上X座標（返却値）
  @param   y1   バウンディングボックスの左上Y座標（返却値）
  @param   w    バウンディングボックスの幅（返却値）
  @param   h    バウンディングボックスの高さ（返却値）
*/
void Adafruit_GFX::getTextBounds(const char* str, int16_t x, int16_t y,
								 int16_t* x1, int16_t* y1, uint16_t* w,
								 uint16_t* h)
{
	uint8_t c;                                                  // Current character
	int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1; // Bound rect
	// Bound rect is intentionally initialized inverted, so 1st char sets it

	*x1 = x; // Initial position is value passed in
	*y1 = y;
	*w = *h = 0; // Initial size is zero

	while ((c = *str++)) {
		// charBounds() modifies x/y to advance for each character,
		// and min/max x/y are updated to incrementally build bounding rect.
		charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
	}

	if (maxx >= minx) {       // If legit string bounds were found...
		*x1 = minx;           // Update x1 to least X coord,
		*w = maxx - minx + 1; // And w to bound rect width
	}
	if (maxy >= miny) { // Same for height
		*y1 = miny;
		*h = maxy - miny + 1;
	}
}

/*!
  @brief   現在のフォント・サイズでPROGMEM文字列のバウンディングボックスを取得します。
  @param   str  PROGMEM格納のASCII文字列
  @param   x    カーソルのX座標
  @param   y    カーソルのY座標
  @param   x1   バウンディングボックスの左上X座標（返却値）
  @param   y1   バウンディングボックスの左上Y座標（返却値）
  @param   w    バウンディングボックスの幅（返却値）
  @param   h    バウンディングボックスの高さ（返却値）
*/
void Adafruit_GFX::getTextBounds(const String& str, int16_t x, int16_t y,
								 int16_t* x1, int16_t* y1, uint16_t* w,
								 uint16_t* h)
{
	if (str.length() != 0) {
		getTextBounds(const_cast<char*>(str.c_str()), x, y, x1, y1, w, h);
	}
}

/*!
  @brief   現在のフォント・サイズでPROGMEM文字列のバウンディングボックスを取得します。
  @param   str  PROGMEM格納のASCII文字列
  @param   x    カーソルのX座標
  @param   y    カーソルのY座標
  @param   x1   バウンディングボックスの左上X座標（返却値）
  @param   y1   バウンディングボックスの左上Y座標（返却値）
  @param   w    バウンディングボックスの幅（返却値）
  @param   h    バウンディングボックスの高さ（返却値）
  @details 文字列とカーソル位置を指定すると、左上座標と幅・高さを返します。
*/
void Adafruit_GFX::getTextBounds(const __FlashStringHelper* str, int16_t x,
								 int16_t y, int16_t* x1, int16_t* y1,
								 uint16_t* w, uint16_t* h)
{
	uint8_t *s = (uint8_t*)str, c;

	*x1 = x;
	*y1 = y;
	*w = *h = 0;

	int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

	while ((c = pgm_read_byte(s++)))
		charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

	if (maxx >= minx) {
		*x1 = minx;
		*w = maxx - minx + 1;
	}
	if (maxy >= miny) {
		*y1 = miny;
		*h = maxy - miny + 1;
	}
}

void Adafruit_GFX::printlocf(uint16_t x, uint16_t y, const char* format, ...)
{
	setCursor(x, y);
	va_list args;
	va_start(args, format);
	char buffer[64];
	Print::vprintf(format, args);
	va_end(args);

}
#pragma endregion

/***************************************************************************/
#pragma region ボタン描画機能 - Adafruit_GFX_Button
/**************************************************************************/
/*!
  @brief   ボタンUI要素のデフォルトコンストラクタ。
  @details メンバー変数の初期化のみ行います。
*/

Adafruit_GFX_Button::Adafruit_GFX_Button(void)
{
	_gfx = 0;
}

/*!
  @brief   ボタンの色・サイズ・設定を初期化します（中心座標指定）。
  @param   gfx        描画先ディスプレイのポインタ
  @param   x          ボタン中心のX座標
  @param   y          ボタン中心のY座標
  @param   w          ボタンの幅
  @param   h          ボタンの高さ
  @param   outline    枠線色（16ビットRGB565）
  @param   fill       塗りつぶし色（16ビットRGB565）
  @param   textcolor  ラベル文字色（16ビットRGB565）
  @param   label      ボタン内に表示するASCII文字列
  @param   textsize   ラベルの拡大率
  @details 座標はボタンの中心を基準とします。内部的には左上座標へ変換して処理します。
*/

// Classic initButton() function: pass center & size
void Adafruit_GFX_Button::initButton(Adafruit_GFX* gfx, int16_t x, int16_t y,
									 uint16_t w, uint16_t h, uint16_t outline,
									 uint16_t fill, uint16_t textcolor,
									 char* label, uint8_t textsize)
{
	// Tweak arguments and pass to the newer initButtonUL() function...
	initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor,
				 label, textsize);
}

/*!
  @brief   ボタンの色・サイズ・設定を初期化します（中心座標指定、ラベル拡大率個別指定）。
  @param   gfx        描画先ディスプレイのポインタ
  @param   x          ボタン中心のX座標
  @param   y          ボタン中心のY座標
  @param   w          ボタンの幅
  @param   h          ボタンの高さ
  @param   outline    枠線色（16ビットRGB565）
  @param   fill       塗りつぶし色（16ビットRGB565）
  @param   textcolor  ラベル文字色（16ビットRGB565）
  @param   label      ボタン内に表示するASCII文字列
  @param   textsize_x ラベルの横方向拡大率
  @param   textsize_y ラベルの縦方向拡大率
  @details 座標はボタンの中心を基準とします。内部的には左上座標へ変換して処理します。
*/
// Classic initButton() function: pass center & size
void Adafruit_GFX_Button::initButton(Adafruit_GFX* gfx, int16_t x, int16_t y,
									 uint16_t w, uint16_t h, uint16_t outline,
									 uint16_t fill, uint16_t textcolor,
									 char* label, uint8_t textsize_x,
									 uint8_t textsize_y)
{
	// Tweak arguments and pass to the newer initButtonUL() function...
	initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor,
				 label, textsize_x, textsize_y);
}

/*!
  @brief   ボタンの色・サイズ・設定を初期化します（左上座標指定）。
  @param   gfx        描画先ディスプレイのポインタ
  @param   x1         ボタン左上のX座標
  @param   y1         ボタン左上のY座標
  @param   w          ボタンの幅
  @param   h          ボタンの高さ
  @param   outline    枠線色（16ビットRGB565）
  @param   fill       塗りつぶし色（16ビットRGB565）
  @param   textcolor  ラベル文字色（16ビットRGB565）
  @param   label      ボタン内に表示するASCII文字列
  @param   textsize   ラベルの拡大率
  @details 座標はボタンの左上を基準とします。
*/
void Adafruit_GFX_Button::initButtonUL(Adafruit_GFX* gfx, int16_t x1,
									   int16_t y1, uint16_t w, uint16_t h,
									   uint16_t outline, uint16_t fill,
									   uint16_t textcolor, char* label,
									   uint8_t textsize)
{
	initButtonUL(gfx, x1, y1, w, h, outline, fill, textcolor, label, textsize,
				 textsize);
}

/*!
  @brief   ボタンの色・サイズ・設定を初期化します（左上座標指定、ラベル拡大率個別指定）。
  @param   gfx        描画先ディスプレイのポインタ
  @param   x1         ボタン左上のX座標
  @param   y1         ボタン左上のY座標
  @param   w          ボタンの幅
  @param   h          ボタンの高さ
  @param   outline    枠線色（16ビットRGB565）
  @param   fill       塗りつぶし色（16ビットRGB565）
  @param   textcolor  ラベル文字色（16ビットRGB565）
  @param   label      ボタン内に表示するASCII文字列
  @param   textsize_x ラベルの横方向拡大率
  @param   textsize_y ラベルの縦方向拡大率
  @details 座標はボタンの左上を基準とします。
*/
void Adafruit_GFX_Button::initButtonUL(Adafruit_GFX* gfx, int16_t x1,
									   int16_t y1, uint16_t w, uint16_t h,
									   uint16_t outline, uint16_t fill,
									   uint16_t textcolor, char* label,
									   uint8_t textsize_x, uint8_t textsize_y)
{
	_x1 = x1;
	_y1 = y1;
	_w = w;
	_h = h;
	_outlinecolor = outline;
	_fillcolor = fill;
	_textcolor = textcolor;
	_textsize_x = textsize_x;
	_textsize_y = textsize_y;
	_gfx = gfx;
	strncpy(_label, label, 9);
	_label[9] = 0; // strncpy does not place a null at the end.
				   // When 'label' is >9 characters, _label is not terminated.
}

/*!
  @brief   ボタンを画面に描画します。
  @param   inverted  trueで押下状態（塗り色と文字色を反転）で描画
  @details 角丸矩形でボタンを描画し、ラベルを中央に表示します。
*/
void Adafruit_GFX_Button::drawButton(bool inverted)
{
	uint16_t fill, outline, text;

	if (!inverted) {
		fill = _fillcolor;
		outline = _outlinecolor;
		text = _textcolor;
	} else {
		fill = _textcolor;
		outline = _outlinecolor;
		text = _fillcolor;
	}

	uint8_t r = min(_w, _h) / 4; // Corner radius
	_gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
	_gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

	_gfx->setCursor(_x1 + (_w / 2) - (strlen(_label) * 3 * _textsize_x),
					_y1 + (_h / 2) - (4 * _textsize_y));
	_gfx->setTextColor(text);
	_gfx->setTextSize(_textsize_x, _textsize_y);
	_gfx->print(_label);
}

/*!
  @brief   指定座標がボタン内かどうかを判定します。
  @param   x  判定するX座標
  @param   y  判定するY座標
  @retval  true  ボタン内
  @retval  false ボタン外
*/
bool Adafruit_GFX_Button::contains(int16_t x, int16_t y)
{
	return ((x >= _x1) && (x < (int16_t)(_x1 + _w)) && (y >= _y1) &&
			(y < (int16_t)(_y1 + _h)));
}

/*!
  @brief   前回未押下→今回押下となったかを判定します。
  @retval  true  押下イベント発生
  @retval  false 変化なし
*/
bool Adafruit_GFX_Button::justPressed()
{
	return (currstate && !laststate);
}

/*!
  @brief   前回押下→今回未押下となったかを判定します。
  @retval  true  離上イベント発生
  @retval  false 変化なし
*/
bool Adafruit_GFX_Button::justReleased()
{
	return (!currstate && laststate);
}

#pragma endregion
// -------------------------------------------------------------------------

#pragma region キャンバス機能 - GFXcanvas1, GFXcanvas8, GFXcanvas16

// GFXcanvas1, GFXcanvas8 and GFXcanvas16 (currently a WIP, don't get too
// comfy with the implementation) provide 1-, 8- and 16-bit offscreen
// canvases, the address of which can be passed to drawBitmap() or
// pushColors() (the latter appears only in a couple of GFX-subclassed TFT
// libraries at this time).  This is here mostly to help with the recently-
// added proportionally-spaced fonts; adds a way to refresh a section of the
// screen without a massive flickering clear-and-redraw...but maybe you'll
// find other uses too.  VERY RAM-intensive, since the buffer is in MCU
// memory and not the display driver...GXFcanvas1 might be minimally useful
// on an Uno-class board, but this and the others are much more likely to
// require at least a Mega or various recent ARM-type boards (recommended,
// as the text+bitmap draw can be pokey).  GFXcanvas1 requires 1 bit per
// pixel (rounded up to nearest byte per scanline), GFXcanvas8 is 1 byte
// per pixel (no scanline pad), and GFXcanvas16 uses 2 bytes per pixel (no
// scanline pad).
// NOT EXTENSIVELY TESTED YET.  MAY CONTAIN WORST BUGS KNOWN TO HUMANKIND.
#pragma region 白黒キャンバス機能 -  GFXcanvas1
#ifdef __AVR__
// Bitmask tables of 0x80>>X and ~(0x80>>X), because X>>Y is slow on AVR
const uint8_t PROGMEM GFXcanvas1::GFXsetBit[] = {0x80, 0x40, 0x20, 0x10,
												 0x08, 0x04, 0x02, 0x01};
const uint8_t PROGMEM GFXcanvas1::GFXclrBit[] = {0x7F, 0xBF, 0xDF, 0xEF,
												 0xF7, 0xFB, 0xFD, 0xFE};
#endif

/**************************************************************************/
/*!
   @brief    Instatiate a GFX 1-bit canvas context for graphics
   @param    w   Display width, in pixels
   @param    h   Display height, in pixels
   @param    allocate_buffer If true, a buffer is allocated with malloc. If
   false, the subclass must initialize the buffer before any drawing operation,
   and free it in the destructor. If false (the default), the buffer is
   allocated and freed by the library.
*/
/**************************************************************************/
GFXcanvas1::GFXcanvas1(uint16_t w, uint16_t h, bool allocate_buffer) :
	Adafruit_GFX(w, h),
	buffer_owned(allocate_buffer)
{
	isBackground = false;

	if (allocate_buffer) {
		uint32_t bytes = ((w + 7) / 8) * h;
		if ((buffer = (uint8_t*)malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
	} else {
		buffer = nullptr;
	}
}

/**************************************************************************/
/*!
   @brief    Delete the canvas, free memory
*/
/**************************************************************************/
GFXcanvas1::~GFXcanvas1(void)
{
#ifdef MICROPY_BUILD_TYPE
	if (buffer && buffer_owned)
		m_free(buffer);
#else
	if (buffer && buffer_owned)
		free(buffer);
#endif
}
void GFXcanvas1::useBackgroundColor(uint16_t color)
{
	isBackground = true;
	bckColor = color;
}
void GFXcanvas1::disableBackgroundColor()
{
	isBackground = false;
}

/**************************************************************************/
/*!
   @brief    コピーコンストラクタ
   @param    pSrc  複製元のキャンバスへのポインタ
   @param    allocate_buffer Trueの場合、画像のバッファはこのクラス内でmallocされ、
   デストラクタでfreeされる。
   Falseの場合、バッファはコピーコンストラクタのバッファを共有する。
*/
/**************************************************************************/
GFXcanvas1::GFXcanvas1(const GFXcanvas1* pSrc, bool allocate_buffer) :
	Adafruit_GFX(pSrc->width(), pSrc->height()),
	buffer_owned(allocate_buffer)
{
	isBackground = false;
	bckColor = pSrc->bckColor;

	if (allocate_buffer) {
		uint32_t bytes = ((WIDTH + 7) / 8) * HEIGHT;
		if ((buffer = (uint8_t*)malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
	} else {
		buffer = pSrc->buffer;
	}
}

/**************************************************************************/
/*!
	@brief  Draw a pixel to the canvas framebuffer
	@param  x     x coordinate
	@param  y     y coordinate
	@param  color Binary (on or off) color to fill with
*/
/**************************************************************************/
void GFXcanvas1::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if (buffer) {
		if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
			return;

		int16_t t;
		switch (rotation) {
			case 1:
				t = x;
				x = WIDTH - 1 - y;
				y = t;
				break;
			case 2:
				x = WIDTH - 1 - x;
				y = HEIGHT - 1 - y;
				break;
			case 3:
				t = x;
				x = y;
				y = HEIGHT - 1 - t;
				break;
		}

		uint8_t* ptr = &buffer[(x / 8) + y * ((WIDTH + 7) / 8)];
#ifdef __AVR__
		if (color)
			*ptr |= pgm_read_byte(&GFXsetBit[x & 7]);
		else
			*ptr &= pgm_read_byte(&GFXclrBit[x & 7]);
#else
		if (color)
			*ptr |= 0x80 >> (x & 7);
		else
			*ptr &= ~(0x80 >> (x & 7));
#endif
	}
}

/**********************************************************************/
/*!
		@brief    Get the pixel color value at a given coordinate
		@param    x   x coordinate
		@param    y   y coordinate
		@returns  The desired pixel's binary color value, either 0x1 (on) or 0x0
   (off)
*/
/**********************************************************************/
bool GFXcanvas1::getPixel(int16_t x, int16_t y) const
{
	int16_t t;
	switch (rotation) {
		case 1:
			t = x;
			x = WIDTH - 1 - y;
			y = t;
			break;
		case 2:
			x = WIDTH - 1 - x;
			y = HEIGHT - 1 - y;
			break;
		case 3:
			t = x;
			x = y;
			y = HEIGHT - 1 - t;
			break;
	}
	return getRawPixel(x, y);
}

/**********************************************************************/
/*!
		@brief    Get the pixel color value at a given, unrotated coordinate.
			  This method is intended for hardware drivers to get pixel value
			  in physical coordinates.
		@param    x   x coordinate
		@param    y   y coordinate
		@returns  The desired pixel's binary color value, either 0x1 (on) or 0x0
   (off)
*/
/**********************************************************************/
bool GFXcanvas1::getRawPixel(int16_t x, int16_t y) const
{
	if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
		return 0;
	if (buffer) {
		uint8_t* ptr = &buffer[(x / 8) + y * ((WIDTH + 7) / 8)];

#ifdef __AVR__
		return ((*ptr) & pgm_read_byte(&GFXsetBit[x & 7])) != 0;
#else
		return ((*ptr) & (0x80 >> (x & 7))) != 0;
#endif
	}
	return 0;
}

/**************************************************************************/
/*!
	@brief  Fill the framebuffer completely with one color
	@param  color Binary (on or off) color to fill with
*/
/**************************************************************************/
void GFXcanvas1::fillScreen(uint16_t color)
{
	if (buffer) {
		uint32_t bytes = ((WIDTH + 7) / 8) * HEIGHT;
		memset(buffer, color ? 0xFF : 0x00, bytes);
	}
}

/**************************************************************************/
/*!
   @brief  Speed optimized vertical line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  h      Length of vertical line to be drawn, including first point
   @param  color  Color to fill with
*/
/**************************************************************************/
void GFXcanvas1::drawFastVLine(int16_t x, int16_t y, int16_t h,
							   uint16_t color)
{
	if (h < 0) { // Convert negative heights to positive equivalent
		h *= -1;
		y -= h - 1;
		if (y < 0) {
			h += y;
			y = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
		return;
	}

	if (y < 0) { // Clip top
		h += y;
		y = 0;
	}
	if (y + h > height()) { // Clip bottom
		h = height() - y;
	}

	if (getRotation() == 0) {
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		x -= h - 1;
		drawFastRawHLine(x, y, h, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		y -= h - 1;
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		drawFastRawHLine(x, y, h, color);
	}
}

/**************************************************************************/
/*!
   @brief  Speed optimized horizontal line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  w      Length of horizontal line to be drawn, including first point
   @param  color  Color to fill with
*/
/**************************************************************************/
void GFXcanvas1::drawFastHLine(int16_t x, int16_t y, int16_t w,
							   uint16_t color)
{
	if (w < 0) { // Convert negative widths to positive equivalent
		w *= -1;
		x -= w - 1;
		if (x < 0) {
			w += x;
			x = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
		return;
	}

	if (x < 0) { // Clip left
		w += x;
		x = 0;
	}
	if (x + w >= width()) { // Clip right
		w = width() - x;
	}

	if (getRotation() == 0) {
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		drawFastRawVLine(x, y, w, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		x -= w - 1;
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		y -= w - 1;
		drawFastRawVLine(x, y, w, color);
	}
}

/**************************************************************************/
/*!
   @brief    Speed optimized vertical line drawing into the raw canvas buffer
   @param    x   Line horizontal start point
   @param    y   Line vertical start point
   @param    h   length of vertical line to be drawn, including first point
   @param    color   Binary (on or off) color to fill with
*/
/**************************************************************************/
void GFXcanvas1::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
								  uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	int16_t row_bytes = ((WIDTH + 7) / 8);
	uint8_t* ptr = &buffer[(x / 8) + y * row_bytes];

	if (color > 0) {
#ifdef __AVR__
		uint8_t bit_mask = pgm_read_byte(&GFXsetBit[x & 7]);
#else
		uint8_t bit_mask = (0x80 >> (x & 7));
#endif
		for (int16_t i = 0; i < h; i++) {
			*ptr |= bit_mask;
			ptr += row_bytes;
		}
	} else {
#ifdef __AVR__
		uint8_t bit_mask = pgm_read_byte(&GFXclrBit[x & 7]);
#else
		uint8_t bit_mask = ~(0x80 >> (x & 7));
#endif
		for (int16_t i = 0; i < h; i++) {
			*ptr &= bit_mask;
			ptr += row_bytes;
		}
	}
}

/**************************************************************************/
/*!
   @brief    Speed optimized horizontal line drawing into the raw canvas buffer
   @param    x   Line horizontal start point
   @param    y   Line vertical start point
   @param    w   length of horizontal line to be drawn, including first point
   @param    color   Binary (on or off) color to fill with
*/
/**************************************************************************/
void GFXcanvas1::drawFastRawHLine(int16_t x, int16_t y, int16_t w,
								  uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	int16_t rowBytes = ((WIDTH + 7) / 8);
	uint8_t* ptr = &buffer[(x / 8) + y * rowBytes];
	size_t remainingWidthBits = w;

	// check to see if first byte needs to be partially filled
	if ((x & 7) > 0) {
		// create bit mask for first byte
		uint8_t startByteBitMask = 0x00;
		for (int8_t i = (x & 7); ((i < 8) && (remainingWidthBits > 0)); i++) {
#ifdef __AVR__
			startByteBitMask |= pgm_read_byte(&GFXsetBit[i]);
#else
			startByteBitMask |= (0x80 >> i);
#endif
			remainingWidthBits--;
		}
		if (color > 0) {
			*ptr |= startByteBitMask;
		} else {
			*ptr &= ~startByteBitMask;
		}

		ptr++;
	}

	// do the next remainingWidthBits bits
	if (remainingWidthBits > 0) {
		size_t remainingWholeBytes = remainingWidthBits / 8;
		size_t lastByteBits = remainingWidthBits % 8;
		uint8_t wholeByteColor = color > 0 ? 0xFF : 0x00;

		memset(ptr, wholeByteColor, remainingWholeBytes);

		if (lastByteBits > 0) {
			uint8_t lastByteBitMask = 0x00;
			for (size_t i = 0; i < lastByteBits; i++) {
#ifdef __AVR__
				lastByteBitMask |= pgm_read_byte(&GFXsetBit[i]);
#else
				lastByteBitMask |= (0x80 >> i);
#endif
			}
			ptr += remainingWholeBytes;

			if (color > 0) {
				*ptr |= lastByteBitMask;
			} else {
				*ptr &= ~lastByteBitMask;
			}
		}
	}
}
#pragma endregion

#pragma region ８ビット色キャンバス機能 -  GFXcanvas8
/**************************************************************************/
/*!
   @brief    Instatiate a GFX 8-bit canvas context for graphics
   @param    w   Display width, in pixels
   @param    h   Display height, in pixels
   @param    allocate_buffer If true, a buffer is allocated with malloc. If
   false, the subclass must initialize the buffer before any drawing operation,
   and free it in the destructor. If false (the default), the buffer is
   allocated and freed by the library.
*/
/**************************************************************************/
GFXcanvas8::GFXcanvas8(uint16_t w, uint16_t h, bool allocate_buffer) :
	Adafruit_GFX(w, h),
	buffer_owned(allocate_buffer)
{
	isBackground = false;
	bckColor = 0;
	if (allocate_buffer) {
		uint32_t bytes = w * h;
		if ((buffer = (uint8_t*)malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
	} else
		buffer = nullptr;
}

/**************************************************************************/
/*!
   @brief    Delete the canvas, free memory
*/
/**************************************************************************/
GFXcanvas8::~GFXcanvas8(void)
{
	if (buffer && buffer_owned)
		free(buffer);
}

/**************************************************************************/
/*!
   @brief    コピーコンストラクタ
   @param    pSrc  複製元のキャンバスへのポインタ
   @param    allocate_buffer Trueの場合、画像のバッファはこのクラス内でmallocされ、
   デストラクタでfreeされる。
   Falseの場合、バッファはコピーコンストラクタのバッファを共有する。
*/
/**************************************************************************/
GFXcanvas8::GFXcanvas8(const GFXcanvas8* pSrc, bool allocate_buffer) :
	Adafruit_GFX(pSrc->width(), pSrc->height()),
	buffer_owned(allocate_buffer)
{
	isBackground = false;
	bckColor = pSrc->bckColor;

	if (allocate_buffer) {
		uint32_t bytes = WIDTH * HEIGHT;
		if ((buffer = (uint8_t*)malloc(bytes))) {
			memcpy(buffer, pSrc->buffer, bytes);
		}
	} else {
		buffer = pSrc->buffer;
	}
}

/**
 * @brief  背景色を設定する。この色はビットマップ描画の際に透明となる。
 */
void GFXcanvas8::useTransparentColor(uint8_t color)
{
	isBackground = true;
	bckColor = color;
}
/**
 * @brief  背景色を無効にする。
 */
void GFXcanvas8::unUseTransparentColor()
{
	isBackground = false;
	bckColor = 0;
}

GFXcanvas8* GFXcanvas8::deepCopy(const GFXcanvas8* src)
{
	if (WIDTH != src->width() || HEIGHT != src->height()) {
		return NULL;
	}
	memcpy(buffer, src->buffer, WIDTH * HEIGHT);
	return this;
}

/**************************************************************************/
/*!
	@brief  Draw a pixel to the canvas framebuffer
	@param  x   x coordinate
	@param  y   y coordinate
	@param  color 8-bit Color to fill with. Only lower byte of uint16_t is used.
*/
/**************************************************************************/
void GFXcanvas8::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if (buffer) {
		if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
			return;

		int16_t t;
		switch (rotation) {
			case 1:
				t = x;
				x = WIDTH - 1 - y;
				y = t;
				break;
			case 2:
				x = WIDTH - 1 - x;
				y = HEIGHT - 1 - y;
				break;
			case 3:
				t = x;
				x = y;
				y = HEIGHT - 1 - t;
				break;
		}

		buffer[x + y * WIDTH] = color;
	}
}

/**********************************************************************/
/*!
		@brief    Get the pixel color value at a given coordinate
		@param    x   x coordinate
		@param    y   y coordinate
		@returns  The desired pixel's 8-bit color value
*/
/**********************************************************************/
uint8_t GFXcanvas8::getPixel(int16_t x, int16_t y) const
{
	int16_t t;
	switch (rotation) {
		case 1:
			t = x;
			x = WIDTH - 1 - y;
			y = t;
			break;
		case 2:
			x = WIDTH - 1 - x;
			y = HEIGHT - 1 - y;
			break;
		case 3:
			t = x;
			x = y;
			y = HEIGHT - 1 - t;
			break;
	}
	return getRawPixel(x, y);
}

/**********************************************************************/
/*!
		@brief    Get the pixel color value at a given, unrotated coordinate.
			  This method is intended for hardware drivers to get pixel value
			  in physical coordinates.
		@param    x   x coordinate
		@param    y   y coordinate
		@returns  The desired pixel's 8-bit color value
*/
/**********************************************************************/
uint8_t GFXcanvas8::getRawPixel(int16_t x, int16_t y) const
{
	if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
		return 0;
	if (buffer) {
		return buffer[x + y * WIDTH];
	}
	return 0;
}

/**************************************************************************/
/*!
	@brief  Fill the framebuffer completely with one color
	@param  color 8-bit Color to fill with. Only lower byte of uint16_t is used.
*/
/**************************************************************************/
void GFXcanvas8::fillScreen(uint16_t color)
{
	if (buffer) {
		memset(buffer, color, WIDTH * HEIGHT);
	}
}

/**************************************************************************/
/*!
   @brief  Speed optimized vertical line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  h      Length of vertical line to be drawn, including first point
   @param  color  8-bit Color to fill with. Only lower byte of uint16_t is
				  used.
*/
/**************************************************************************/
void GFXcanvas8::drawFastVLine(int16_t x, int16_t y, int16_t h,
							   uint16_t color)
{
	if (h < 0) { // Convert negative heights to positive equivalent
		h *= -1;
		y -= h - 1;
		if (y < 0) {
			h += y;
			y = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
		return;
	}

	if (y < 0) { // Clip top
		h += y;
		y = 0;
	}
	if (y + h > height()) { // Clip bottom
		h = height() - y;
	}

	if (getRotation() == 0) {
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		x -= h - 1;
		drawFastRawHLine(x, y, h, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		y -= h - 1;
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		drawFastRawHLine(x, y, h, color);
	}
}

/**************************************************************************/
/*!
   @brief  Speed optimized horizontal line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  w      Length of horizontal line to be drawn, including 1st point
   @param  color  8-bit Color to fill with. Only lower byte of uint16_t is
				  used.
*/
/**************************************************************************/
void GFXcanvas8::drawFastHLine(int16_t x, int16_t y, int16_t w,
							   uint16_t color)
{
	if (w < 0) { // Convert negative widths to positive equivalent
		w *= -1;
		x -= w - 1;
		if (x < 0) {
			w += x;
			x = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
		return;
	}

	if (x < 0) { // Clip left
		w += x;
		x = 0;
	}
	if (x + w >= width()) { // Clip right
		w = width() - x;
	}

	if (getRotation() == 0) {
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		drawFastRawVLine(x, y, w, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		x -= w - 1;
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		y -= w - 1;
		drawFastRawVLine(x, y, w, color);
	}
}

/**************************************************************************/
/*!
   @brief    Speed optimized vertical line drawing into the raw canvas buffer
   @param    x   Line horizontal start point
   @param    y   Line vertical start point
   @param    h   length of vertical line to be drawn, including first point
   @param    color   8-bit Color to fill with. Only lower byte of uint16_t is
   used.
*/
/**************************************************************************/
void GFXcanvas8::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
								  uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	uint8_t* buffer_ptr = buffer + y * WIDTH + x;
	for (int16_t i = 0; i < h; i++) {
		(*buffer_ptr) = color;
		buffer_ptr += WIDTH;
	}
}

void GFXcanvas8::drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	memset(buffer + y * WIDTH + x, color, w);
}
#pragma endregion

#pragma region １６ビット色キャンバス機能 -  GFXcanvas16

GFXcanvas16::GFXcanvas16() : // デフォルトコンストラクタ
	Adafruit_GFX(0, 0),
	buffer_owned(false)
{

	isBackground = false;
	bckColor = 0;
	buffer = nullptr;
}

/*!
  @brief  16ビットカラーキャンバスを生成するコンストラクタ。
  @param  w   キャンバスの幅（ピクセル単位）
  @param  h   キャンバスの高さ（ピクセル単位）
  @param  allocate_buffer  trueの場合は内部バッファを確保、falseの場合はサブクラスでバッファ管理
  @details allocate_bufferがtrueなら内部でバッファをmallocし、falseならバッファは外部で用意・解放する必要があります。
*/
GFXcanvas16::GFXcanvas16(uint16_t w, uint16_t h, bool allocate_buffer) :
	Adafruit_GFX(w, h),
	buffer_owned(allocate_buffer)
{
	isBackground = false;
	bckColor = 0;
	if (allocate_buffer) {
		uint32_t bytes = w * h * 2;
#ifdef MICROPY_BUILD_TYPE
		if ((buffer = (uint16_t*)m_malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
#else
		if ((buffer = (uint16_t*)malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
#endif
	} else {
		buffer = nullptr;
	}
}

/*!
  @brief  16ビットカラーキャンバスのメモリを解放するデストラクタ。
  @details
	インスタンス生成時に内部バッファを確保していた場合（buffer_ownedがtrue）、
	デストラクタでバッファのメモリを解放します。
	外部管理バッファの場合は解放処理を行いません。
*/
GFXcanvas16::~GFXcanvas16(void)
{
	if (buffer && buffer_owned)
		free(buffer);
}

/*!
  @brief    コピーコンストラクタ（GFXcanvas16）。
  @param    pSrc  複製元のキャンバスへのポインタ
  @param    allocate_buffer trueの場合は新たにバッファを確保して内容をコピー、falseの場合はバッファを共有
  @details
	allocate_bufferがtrueの場合、pSrcの内容を新しいバッファにコピーし、デストラクタで解放します。
	falseの場合はpSrcのバッファを共有し、解放は行いません。
	背景色や透過色の設定も複製されます。
*/
GFXcanvas16::GFXcanvas16(const GFXcanvas16* pSrc, bool allocate_buffer) :
	Adafruit_GFX(pSrc->width(), pSrc->height()),
	buffer_owned(allocate_buffer)
{
	isBackground = false;
	bckColor = pSrc->bckColor;

	if (allocate_buffer) {
		uint32_t bytes = WIDTH * HEIGHT * 2;
#ifdef MICROPY_BUILD_TYPE
		if ((buffer = (uint16_t*)m_malloc(bytes))) {
			memcpy(buffer, pSrc->buffer, bytes);
		}
#else
		if ((buffer = (uint16_t*)malloc(bytes))) {
			memcpy(buffer, 0, bytes);
		}
#endif
	} else {
		buffer = pSrc->buffer;
	}
}
/*!
  @brief  オブジェクト初期化し、バッファを確保または設定する。デフォルトコンストラクタで作成した場合にはこれを使って初期化する。
  @param  w   キャンバスの幅（ピクセル単位）
  @param  h   キャンバスの高さ（ピクセル単位）
  @param  allocate_buffer  trueの場合は内部バッファを確保、falseの場合は外部でバッファを管理
  @details
	Adafruit_GFXのメンバも再初期化します。allocate_bufferがtrueなら内部でバッファを確保しゼロクリアします。
	falseの場合はbufferをnullptrに設定し、外部でバッファの用意・解放が必要です。
*/
void GFXcanvas16::constructObject(uint16_t w, uint16_t h, bool allocate_buffer)
{
	Adafruit_GFX::constructObject(w, h);
	buffer_owned = allocate_buffer;
	isBackground = false;
	bckColor = 0;
	if (allocate_buffer) {
		uint32_t bytes = w * h * 2;
#ifdef MICROPY_BUILD_TYPE
		if ((buffer = (uint16_t*)m_malloc(bytes))) {
			memset(buffer, 0, bytes);
		}
#else
		if ((buffer = (uint16_t*)malloc(bytes))) {
			memcpy(buffer, 0, bytes);
		}
#endif
	} else {
		buffer = nullptr;
	}
}
/*!
  @brief  オブジェクトを破棄し、バッファを解放する。
  @details
	buffer_ownedがtrueの場合は内部バッファを解放します。
	falseの場合はバッファの解放は行いません。
	バッファ解放後、ポインタをnullptrにリセットします。
*/
void GFXcanvas16::destructObject()
{
	if (buffer && buffer_owned) {
#ifdef MICROPY_BUILD_TYPE
		m_free(buffer);
		buffer = nullptr;
#else
		free(buffer);
		buffer = nullptr;
#endif
	}
}
/*!
  @brief  背景色を設定する。この色はビットマップ描画の際に透明となる。
  @param  color  透過色として扱う16ビットカラー値（RGB565）
  @details
	isBackgroundフラグを有効化し、bckColorに指定色を設定します。
	以降の描画でこの色が背景色（透明色）として扱われます。
*/
void GFXcanvas16::useTransparentColor(uint16_t color)
{
	isBackground = true;
	bckColor = color;
}
/*!
  @brief  背景色を無効にする。
  @details
	isBackgroundフラグを無効化し、bckColorを0にリセットします。
	以降の描画で背景色（透明色）は適用されません。
*/
void GFXcanvas16::unUseTransparentColor()
{
	isBackground = false;
	bckColor = 0;
}
/*!
  @brief  他のGFXcanvas16インスタンスの内容をこのキャンバスにコピーする。
  @param  src  コピー元のGFXcanvas16インスタンス
  @return コピーに成功した場合は自身のポインタ、サイズ不一致時はNULL
  @details
	キャンバスサイズ（幅・高さ）が一致している場合のみ、srcのバッファ内容を
	このインスタンスのバッファにmemcpyでコピーします。
	サイズが異なる場合は何もせずNULLを返します。
*/
GFXcanvas16* GFXcanvas16::deepCopy(const GFXcanvas16* src)
{
	if (WIDTH != src->width() || HEIGHT != src->height()) {
		return NULL;
	}
	memcpy(buffer, src->buffer, WIDTH * HEIGHT * 2);
	return this;
}

/*!
  @brief  キャンバスの指定座標にピクセルを描画する。
  @param  x     描画するX座標
  @param  y     描画するY座標
  @param  color 描画色（16ビットRGB565）
  @details
	座標がキャンバス範囲外の場合は何も行いません。
	キャンバスの回転設定に応じて座標変換を行い、バッファにピクセルを書き込みます。
*/
void GFXcanvas16::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if (buffer) {
		if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
			return;

		int16_t t;
		switch (rotation) {
			case 1:
				t = x;
				x = WIDTH - 1 - y;
				y = t;
				break;
			case 2:
				x = WIDTH - 1 - x;
				y = HEIGHT - 1 - y;
				break;
			case 3:
				t = x;
				x = y;
				y = HEIGHT - 1 - t;
				break;
		}

		buffer[x + y * WIDTH] = color;
	}
}

/*!
  @brief  指定座標のピクセル色を取得する。
  @param  x   取得するX座標
  @param  y   取得するY座標
  @return 指定座標の16ビットRGB565カラー値
  @details
	キャンバスの回転設定に応じて座標変換を行い、バッファからピクセル値を取得します。
	範囲外の場合は0を返します。
*/
uint16_t GFXcanvas16::getPixel(int16_t x, int16_t y) const
{
	int16_t t;
	switch (rotation) {
		case 1:
			t = x;
			x = WIDTH - 1 - y;
			y = t;
			break;
		case 2:
			x = WIDTH - 1 - x;
			y = HEIGHT - 1 - y;
			break;
		case 3:
			t = x;
			x = y;
			y = HEIGHT - 1 - t;
			break;
	}
	return getRawPixel(x, y);
}

/*!
  @brief  指定座標（回転なし）にあるピクセルの色を取得する。
  @param  x   取得するX座標（バッファ座標系、回転なし）
  @param  y   取得するY座標（バッファ座標系、回転なし）
  @return 指定座標の16ビットRGB565カラー値
  @details
	座標がキャンバス範囲外の場合は0を返します。
	バッファが有効な場合はbuffer[x + y * WIDTH]の値を返します。
	この関数は回転変換を行わず、物理バッファ上の座標を直接参照します。
*/
uint16_t GFXcanvas16::getRawPixel(int16_t x, int16_t y) const
{
	if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
		return 0;
	if (buffer) {
		return buffer[x + y * WIDTH];
	}
	return 0;
}

/*!
  @brief  フレームバッファ全体を指定色で塗りつぶす。
  @param  color 塗りつぶし色（16ビットRGB565）
  @details
	バッファが有効な場合、全ピクセルをcolorで塗りつぶします。
	上位バイトと下位バイトが同じ場合はmemsetによる高速化を行います。
*/
void GFXcanvas16::fillScreen(uint16_t color)
{
	if (buffer) {
		uint8_t hi = color >> 8, lo = color & 0xFF;
		if (hi == lo) {
			memset(buffer, lo, WIDTH * HEIGHT * 2);
		} else {
			uint32_t i, pixels = WIDTH * HEIGHT;
			for (i = 0; i < pixels; i++) {
				buffer[i] = color;
			}
		}
	}
}

/*!
  @brief  キャンバス内の各16ビットピクセルのエンディアンを反転する。
  @details
	キャンバス内の全ピクセルについて、リトルエンディアンとビッグエンディアンを相互に変換します。
	ほとんどのマイコン（例：SAMD）はリトルエンディアンですが、多くのディスプレイはビッグエンディアンです。
	通常の描画処理（RGBビットマップ描画など）では自動的にエンディアン変換が行われますが、
	DMA転送などでディスプレイのネイティブな順序に合わせたい場合に有効です。
	この関数は特定のエンディアンに変換するのではなく、各16ビット値のバイト順を単純に入れ替えます。
*/
void GFXcanvas16::byteSwap(void)
{
	if (buffer) {
		uint32_t i, pixels = WIDTH * HEIGHT;
		for (i = 0; i < pixels; i++)
			buffer[i] = __builtin_bswap16(buffer[i]);
	}
}

/*!
  @brief  垂直線を高速に描画する。
  @param  x     線の開始x座標
  @param  y     線の開始y座標
  @param  h     線の長さ（ピクセル数）
  @param  color 描画色（16ビットRGB565）
  @details
	回転やクリッピング処理を行い、キャンバスバッファに垂直線を高速に描画します。
	座標や長さが範囲外の場合は自動的にクリッピングされます。
*/
void GFXcanvas16::drawFastVLine(int16_t x, int16_t y, int16_t h,
								uint16_t color)
{
	if (h < 0) { // Convert negative heights to positive equivalent
		h *= -1;
		y -= h - 1;
		if (y < 0) {
			h += y;
			y = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
		return;
	}

	if (y < 0) { // Clip top
		h += y;
		y = 0;
	}
	if (y + h > height()) { // Clip bottom
		h = height() - y;
	}

	if (getRotation() == 0) {
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		x -= h - 1;
		drawFastRawHLine(x, y, h, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		y -= h - 1;
		drawFastRawVLine(x, y, h, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		drawFastRawHLine(x, y, h, color);
	}
}

/*!
  @brief  水平線を高速に描画する。
  @param  x     線の開始x座標
  @param  y     線のy座標
  @param  w     線の長さ（ピクセル数）
  @param  color 描画色（16ビットRGB565）
  @details
	回転やクリッピング処理を行い、キャンバスバッファに水平線を高速に描画します。
	座標や長さが範囲外の場合は自動的にクリッピングされます。
*/
void GFXcanvas16::drawFastHLine(int16_t x, int16_t y, int16_t w,
								uint16_t color)
{
	if (w < 0) { // Convert negative widths to positive equivalent
		w *= -1;
		x -= w - 1;
		if (x < 0) {
			w += x;
			x = 0;
		}
	}

	// Edge rejection (no-draw if totally off canvas)
	if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
		return;
	}

	if (x < 0) { // Clip left
		w += x;
		x = 0;
	}
	if (x + w >= width()) { // Clip right
		w = width() - x;
	}

	if (getRotation() == 0) {
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 1) {
		int16_t t = x;
		x = WIDTH - 1 - y;
		y = t;
		drawFastRawVLine(x, y, w, color);
	} else if (getRotation() == 2) {
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;

		x -= w - 1;
		drawFastRawHLine(x, y, w, color);
	} else if (getRotation() == 3) {
		int16_t t = x;
		x = y;
		y = HEIGHT - 1 - t;
		y -= w - 1;
		drawFastRawVLine(x, y, w, color);
	}
}

/*!
  @brief  キャンバスバッファに高速に垂直線を描画する。
  @param  x     線の開始x座標（バッファ座標系、回転なし）
  @param  y     線のy座標（バッファ座標系、回転なし）
  @param  h     描画する線の長さ（ピクセル数）
  @param  color 描画色（16ビットRGB565）
  @details
	バッファ上の指定位置からhピクセル分、colorで塗りつぶします。
	x, yは回転変換前のバッファ座標で、クリッピングや座標変換は呼び出し元で行う必要があります。
*/
void GFXcanvas16::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
								   uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	uint16_t* buffer_ptr = buffer + y * WIDTH + x;
	for (int16_t i = 0; i < h; i++) {
		(*buffer_ptr) = color;
		buffer_ptr += WIDTH;
	}
}

/*!
  @brief  キャンバスバッファに高速に水平線を描画する。
  @param  x     線の開始x座標（バッファ座標系、回転なし）
  @param  y     線のy座標（バッファ座標系、回転なし）
  @param  w     描画する線の長さ（ピクセル数）
  @param  color 描画色（16ビットRGB565）
  @details
	バッファ上の指定位置からwピクセル分、colorで塗りつぶします。
	座標変換やクリッピングは呼び出し元で行う必要があります。
*/
void GFXcanvas16::drawFastRawHLine(int16_t x, int16_t y, int16_t w,
								   uint16_t color)
{
	// x & y already in raw (rotation 0) coordinates, no need to transform.
	uint32_t buffer_index = y * WIDTH + x;
	for (uint32_t i = buffer_index; i < buffer_index + w; i++) {
		buffer[i] = color;
	}
}
/*!
  @brief  16ビットRGB565ビットマップをキャンバスの指定位置にコピーする。
  @param  x        コピー先キャンバスのX座標
  @param  y        コピー先キャンバスのY座標
  @param  w        コピーするビットマップの幅
  @param  h        コピーするビットマップの高さ
  @param  picBuf   コピー元ビットマップデータのポインタ（RGB565フォーマット）
  @param  bufWidth  コピー元ビットマップ全体の幅
  @param  bufHeight コピー元ビットマップ全体の高さ
  @details
	指定した座標(x, y)からw×hサイズ分、picBufからキャンバスのバッファへ転送します。
	コピー元ビットマップはbufWidth×bufHeightサイズで、picBuf[(y + i) * bufWidth + x + j]の値を
	buffer[i * w + j]にコピーします。
*/
void GFXcanvas16::copyRGBBitmap(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t* picBuf, uint16_t bufWidth, uint16_t bufHeight)
{
	uint16_t* ptr = &buffer[x + y * WIDTH];
	for (int16_t i = 0; i < h; i++) {
		for (int16_t j = 0; j < w; j++) {
			buffer[i * w + j] = picBuf[(y + i) * bufWidth + x + j];
		}
	}
}

#pragma endregion

#pragma endregion