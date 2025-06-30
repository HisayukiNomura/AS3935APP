/**
 * @file printfDebug.h
 * @brief デバッグ用printfラッパー関数の宣言・実装ヘッダ
 * @details
 * 本ヘッダは、Raspberry Pi Pico等の組込み環境向けに、
 * シリアルデバッグ出力を制御するためのラッパー関数 `dbgprintf` を提供します。
 * `Settings` クラスのシリアルデバッグ設定に応じて、デバッグ出力の有効/無効を切り替えます。
 * バッファオーバーフロー防止のため、vsnprintfを用いて安全にフォーマット処理を行います。
 * 
 * - DEBUG_STDOUT: デバッグ出力有効化用マクロ
 * - dbgprintf: シリアルデバッグ出力用ラッパー関数
 *
 * @author (your name)
 * @date 2025-06-30
 */
#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define DEBUG_STDOUT ///< デバッグ出力を有効化するマクロ
extern Settings settings; ///< グローバル設定インスタンス

/**
 * @brief シリアルデバッグ出力用printfラッパー関数
 * @details
 * Settings::isSerialDebug()がtrueの場合のみ、可変長引数で与えられたフォーマット文字列を
 * シリアル出力(printf)します。バッファサイズは1024バイトで、vsnprintfにより安全にフォーマットします。
 * フォーマット済み文字列は一時バッファに格納され、printfで出力されます。
 *
 * @param format フォーマット文字列 (printf互換)
 * @param ...    可変長引数 (printf互換)
 * @retval なし
 *
 * @note バッファサイズ(1024)を超える出力は切り捨てられます。
 * @note Settings::isSerialDebug()がfalseの場合は何も出力しません。
 * @warning フォーマット文字列にユーザ入力を直接渡すと脆弱性の原因となるため注意。
 */
static void dbgprintf(const char* format, ...)
{
	if (settings.isSerialDebug()) { ///< シリアルデバッグ有効時のみ出力
		char buffer[1024]; ///< 出力用の一時バッファ(1024バイト)
		va_list args;      ///< 可変引数リスト

		va_start(args, format); ///< 可変引数リスト初期化

		// snprintfでフォーマット文字列と可変引数を安全に処理
		int written = vsnprintf(buffer, sizeof(buffer), format, args); ///< フォーマット済み文字列長

		va_end(args); ///< 可変引数リスト終了

		printf("%s", buffer); ///< フォーマット済み文字列を出力(脆弱性防止のためformatではなくbufferを明示)
	}
}