#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "pictData.h"
#include <ctime>

using namespace ardPort;
using namespace ardPort::spi;
/**
 * @brief 時計表示用ユーティリティ
 */
class DispClock {
public:
  static Adafruit_ILI9341* pTFT;

  static uint8_t prevHour;      // 前回表示した時
  static uint8_t prevMinutesU;   // 前回表示した分
  static uint8_t prevMinutesL;   // 前回表示した分の下位
  static uint8_t prevSeconds;   // 前回表示した秒
  static uint8_t prevAmPm;     // 前回表示したAM/PM

  static const uint16_t* numTable[13];
  /**
   * @brief 現在時刻をTFT画面に表示する（インスタンス化不要）
   * @param tft TFTディスプレイへのポインタ
   * @param x   ビットマップ表示のX座標
   * @param y   ビットマップ表示のY座標
   */
  static void init(Adafruit_ILI9341* a_ptft);
 
  static void show(int x = 0, int y = 0);
};
