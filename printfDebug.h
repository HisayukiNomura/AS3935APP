#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define DEBUG_STDOUT // デバッグ出力を有効化

static void dbgprintf(const char* format, ...)
{
    #ifdef DEBUG_STDOUT
	char buffer[1024]; // 出力用の一時バッファ
	va_list args;      // 可変引数を処理するリスト

	// 可変引数リストの初期化
	va_start(args, format);

	// snprintfでフォーマット文字列と可変引数を処理
	int written = vsnprintf(buffer, sizeof(buffer), format, args);

	// 可変引数リストの終了
	va_end(args);

	printf(buffer);

    #else
    #endif
}