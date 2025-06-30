/**
 * @file DispClock.h
 * @brief TFTディスプレイ用時計描画ユーティリティクラス定義
 * @details
 * - pictData.hのビットマップを利用し、時・分・秒・AM/PMをグラフィカルに描画。
 * - 24時間/12時間表示切替、描画最適化のための前回値キャッシュ、再描画フラグ制御などを提供。
 * - インスタンス化不要のstaticユーティリティ設計。
 */
#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "pictData.h"
#include <ctime>

using namespace ardPort;
using namespace ardPort::spi;
/**
 * @brief TFTディスプレイに時計を描画するユーティリティクラス
 * @details
 * - インスタンス化不要、全てstaticメンバ。
 * - 前回描画値をキャッシュし、必要な部分のみ再描画。
 * - 24時間/12時間表示切替、AM/PM表示、再描画フラグ制御をサポート。
 */
class DispClock {
public:
  static Adafruit_ILI9341* pTFT;           ///< TFTディスプレイへのポインタ
  static bool isClock24Hour;               ///< 24時間表示かどうかのフラグ
  static uint8_t prevHour;                 ///< 前回表示した時（12h用）
  static uint8_t prevHourU;                ///< 前回表示した時の十の位（24h用）
  static uint8_t prevHourL;                ///< 前回表示した時の一の位（24h用）
  static uint8_t prevMinutesU;             ///< 前回表示した分の十の位
  static uint8_t prevMinutesL;             ///< 前回表示した分の一の位
  static uint8_t prevSeconds;              ///< 前回表示した秒
  static uint8_t prevAmPm;                 ///< 前回表示したAM/PM

  static const uint16_t* numTable[13];     ///< 0～12の数字ビットマップテーブル
  /**
   * @brief 時計描画用の初期化
   * @details
   * TFTディスプレイのポインタと24時間表示フラグを保存する。
   * @param a_ptft TFTディスプレイへのポインタ
   * @param a_isClock24Hour 24時間表示かどうか
   * @retval なし
   */
  static void init(Adafruit_ILI9341* a_ptft,bool a_isClock24Hour);
  /**
   * @brief 現在時刻をTFT画面に描画する
   * @details
   * pictData.hのビットマップをdrawRGBBitmapで表示し、時刻をグラフィカルに描画。
   * 24時間/12時間表示切替、AM/PM表示、前回値キャッシュによる描画最適化。
   * 秒表示は15秒ごとに異なるビットマップでアニメーション風に表示。
   * @param x ビットマップ表示のX座標
   * @param y ビットマップ表示のY座標
   * @retval なし
   */
  static void show(int x = 0, int y = 0);
  /**
   * @brief 時計再描画フラグを立てる
   * @details
   * 前回値キャッシュを全てリセットし、次回show()で全描画を強制する。
   * @retval なし
   */
  static void setRedrawFlag() {
    prevHour = 0xFF;      ///< 再描画フラグを立てる
    prevHourU = 0xFF;     ///< 十の位をリセット
    prevHourL = 0xFF;     ///< 一の位をリセット
    prevMinutesU = 0xFF;  ///< 分の十の位をリセット
    prevMinutesL = 0xFF;  ///< 分の一の位をリセット
    prevSeconds = 0xFF;   ///< 秒をリセット
    prevAmPm = 0xFF;      ///< AM/PMをリセット
 }
};
