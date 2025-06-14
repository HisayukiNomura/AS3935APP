#pragma once
#include "Adafruit_ILI9341.h"
#include "XPT2046_Touchscreen/XPT2046_Touchscreen.h"

using namespace ardPort;
using namespace ardPort::spi;

class TouchCalibration {
public:
    enum Mode {
        MODE_NONE = 0,
        MODE_SELECT,
        MODE_CALIBRATE,
        MODE_TEST
    };
	uint16_t minX;
	uint16_t maxX;
	uint16_t minY;
	uint16_t maxY;

	TouchCalibration(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);
	bool run(); // メインループ


  private:
    Adafruit_ILI9341* m_tft;
    XPT2046_Touchscreen* m_ts;
    Mode m_mode;
	bool isCalibDone = false;

	void showMenu();
    void runTestMode();
    void runCalibrateMode();
    void waitLongTap();
	void drawTouchPoint(int x, int y, uint16_t color);
};
