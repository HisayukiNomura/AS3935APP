#pragma once
#include <stdint.h>
#include "lib-9341/Adafruit_ILI9341/Adafruit_ILI9341.h"
using namespace ardPort;

class AS3935 {
private:
    uint8_t m_u8I2cPort;
    uint8_t m_u8SdaPin;
    uint8_t m_u8SclPin;
    uint8_t m_u8IrqPin;
    Adafruit_ILI9341* m_pTft;
    // 必要に応じてI2Cアドレスや内部状態も追加
public:
    // コンストラクタ: I2Cポート番号、SDA/SCLピン、IRQピン、TFTポインタを受け取る
    AS3935(Adafruit_ILI9341* a_pTft);
	bool InitI2C(uint8_t PortNo, uint8_t a_SDA, uint8_t a_SCL, uint8_t a_IRQ);

    // デストラクタ
    ~AS3935() {
        // 必要なクリーンアップ処理をここに追加
    }

    // AS3935の初期化メソッド
    bool begin();

    // キャリブレーションのためのメソッド
	// キャリブレーション実行
    void calibrate();

    // その他、AS3935の操作メソッドをここに追加
};
