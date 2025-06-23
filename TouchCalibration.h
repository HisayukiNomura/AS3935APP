/**
 * @file TouchCalibration.h
 * @brief タッチパネルキャリブレーション用クラスの宣言
 * @details
 * Adafruit_ILI9341ディスプレイとXPT2046タッチスクリーンを用いた、
 * タッチパネルのキャリブレーション・テスト・座標取得・描画処理を提供するクラス。
 */
#pragma once
#include "Adafruit_ILI9341.h"
#include "XPT2046_Touchscreen/XPT2046_Touchscreen.h"

using namespace ardPort;
using namespace ardPort::spi;

/**
 * @brief タッチパネルのキャリブレーション・テストを行うクラス
 * @details
 * run()でキャリブレーション・テスト・終了メニューを表示し、
 * ユーザー操作に応じて座標補正やテスト描画を行う。
 * キャリブレーション値はminX, maxX, minY, maxYに格納される。
 */
class TouchCalibration {
public:
    /**
     * @brief 動作モード種別
     */
    enum Mode {
        MODE_NONE = 0, ///< 未選択
        MODE_SELECT,   ///< メニュー選択
        MODE_CALIBRATE,///< キャリブレーション
        MODE_TEST      ///< テスト
    };

    uint16_t minX; ///< キャリブレーション後のX最小値
    uint16_t maxX; ///< キャリブレーション後のX最大値
    uint16_t minY; ///< キャリブレーション後のY最小値
    uint16_t maxY; ///< キャリブレーション後のY最大値

    /**
     * @brief TouchCalibrationコンストラクタ
     * @param tft ディスプレイ制御用ポインタ
     * @param ts タッチスクリーン制御用ポインタ
     */
    TouchCalibration(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);

    /**
     * @brief キャリブレーション・テスト・終了メニューを表示し、ユーザー操作を受け付ける
     * @return キャリブレーションが正常に完了した場合true、終了または未完了の場合false
     */
    bool run();

private:
    Adafruit_ILI9341* m_tft; ///< ディスプレイ制御用
    XPT2046_Touchscreen* m_ts; ///< タッチスクリーン制御用
    Mode m_mode; ///< 現在の動作モード
    bool isCalibDone = false; ///< キャリブレーション完了フラグ

    /**
     * @brief メニュー画面を描画する
     */
    void showMenu();
    /**
     * @brief タッチテストモードを実行し、座標を表示・描画する
     */
    void runTestMode();
    /**
     * @brief キャリブレーションモードを実行し、四隅のタッチ座標を取得・補正値を計算する
     */
    void runCalibrateMode();
    /**
     * @brief ロングタップ（1秒以上）を検出するまで待機する
     */
    void waitLongTap();
    /**
     * @brief 指定座標にタッチポイントを描画する
     * @param x X座標
     * @param y Y座標
     * @param color 円の色
     */
    void drawTouchPoint(int x, int y, uint16_t color);
};
