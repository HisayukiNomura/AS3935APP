/**
 * @file GUIMsgBox.h
 * @brief TFT画面用ダイアログボックスGUIクラス定義
 * @details
 * - ILI9341 TFTディスプレイとXPT2046タッチパネルを用いたOK/Cancel, OKのみのダイアログ表示を提供。
 * - マルチライン・左寄せメッセージ、ボタン色・位置カスタマイズ、タッチ判定等をサポート。
 */
#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"

using namespace ardPort;
using namespace ardPort::spi;
/**
 * @brief TFT画面にダイアログボックスを表示するGUIクラス
 * @details
 * - OK/Cancel, OKのみのダイアログ表示、マルチライン・左寄せメッセージ、ボタン色・位置カスタマイズ対応。
 * - タッチパネル入力でユーザー選択を取得。
 */
class GUIMsgBox {
public:
    /**
     * @brief コンストラクタ
     * @details
     * TFTディスプレイとタッチパネルのポインタを受け取り、内部に保存する。
     * @param tft ILI9341 TFTディスプレイへのポインタ
     * @param ts  XPT2046タッチパネルへのポインタ
     */
    GUIMsgBox(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts); ///< コンストラクタ

    /**
     * @brief OK/Cancelダイアログを表示し、ユーザーの選択を返す
     * @details
     * 指定座標・キャプション・メッセージ・ボタンラベルでダイアログを描画し、
     * タッチパネルでOK/Cancelいずれかが押されるまで待機。
     * @param x ダイアログ左上X座標
     * @param y ダイアログ左上Y座標
     * @param caption キャプション
     * @param message 本文メッセージ（\nで複数行可）
     * @param okMsg OKボタンのラベル
     * @param cancelMsg Cancelボタンのラベル
     * @retval true OKが押された
     * @retval false Cancelが押された
     */
    bool showOKCancel(int x, int y, const char* caption, const char* message, const char* okMsg, const char* cancelMsg);

    /**
     * @brief OKのみのダイアログを表示し、タッチで閉じる
     * @details
     * 指定座標・キャプション・メッセージ・ボタンラベルでダイアログを描画し、
     * タッチパネルでOKが押されるまで待機。
     * @param x ダイアログ左上X座標
     * @param y ダイアログ左上Y座標
     * @param caption キャプション
     * @param message 本文メッセージ（\nで複数行可）
     * @param okMsg OKボタンのラベル
     * @retval なし
     */
    void showOK(int x, int y, const char* caption, const char* message, const char* okMsg);

private:
    Adafruit_ILI9341* m_tft; ///< TFTディスプレイへのポインタ
    XPT2046_Touchscreen* m_ts; ///< タッチパネルへのポインタ
};
