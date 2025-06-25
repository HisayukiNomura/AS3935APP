#pragma once
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
#include "lib-9341/XPT2046_Touchscreen/XPT2046_Touchscreen.h"

using namespace ardPort;
using namespace ardPort::spi;
/**
 * @brief TFT画面にダイアログボックスを表示するGUIクラス
 */
class GUIMsgBox {
public:
    GUIMsgBox(Adafruit_ILI9341* tft, XPT2046_Touchscreen* ts);

    /**
     * @brief OK/Cancelダイアログを表示し、ユーザーの選択を返す
     * @param x ダイアログ左上X座標
     * @param y ダイアログ左上Y座標
     * @param caption キャプション
     * @param message 本文メッセージ
     * @param okMsg OKボタンのラベル
     * @param cancelMsg Cancelボタンのラベル
     * @return OKならtrue、Cancelならfalse
     */
    bool showOKCancel(int x, int y, const char* caption, const char* message, const char* okMsg, const char* cancelMsg);

    /**
     * @brief OKのみのダイアログを表示し、タッチで閉じる
     * @param x ダイアログ左上X座標
     * @param y ダイアログ左上Y座標
     * @param caption キャプション
     * @param message 本文メッセージ
     * @param okMsg OKボタンのラベル
     */
    void showOK(int x, int y, const char* caption, const char* message, const char* okMsg);

private:
    Adafruit_ILI9341* m_tft;
    XPT2046_Touchscreen* m_ts;
};
