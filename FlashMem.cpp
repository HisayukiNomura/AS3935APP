/**
 * @file FlashMem.cpp
 * @brief フラッシュメモリ管理クラスの実装
 * @details
 * Raspberry Pi Picoのオンボードフラッシュメモリに対し、
 * セクタ消去・書き込み・読み出しを安全に行うためのクラス実装。
 * 割り込み制御を用いて排他制御も行う。
 */
#include "FlashMem.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include <cstring>
#include "hardware/regs/addressmap.h"

/**
 * @brief コンストラクタ
 * @param block 書き込み開始ブロック番号
 * @param blockCount 管理するブロック数
 */
FlashMem::FlashMem(uint32_t block, uint32_t blockCount)
    : m_block(block), m_blockCount(blockCount) {}

/**
 * @brief デストラクタ
 */
FlashMem::~FlashMem() {}

/**
 * @brief フラッシュメモリにデータを書き込む
 * @details
 * 指定オフセット・長さ分のデータを、セクタ消去後に書き込む。
 * 書き込み中は割り込みを禁止し、終了後に元の状態へ復帰する。
 * @param offset 書き込み先オフセット（バイト）
 * @param data 書き込むデータへのポインタ
 * @param len 書き込むバイト数
 * @return 成功時true、範囲外や失敗時false
 */
bool FlashMem::write(uint32_t offset, const void* data, uint32_t len)
{
    if (offset + len > m_blockCount * FLASH_BLOCK_SIZE) return false;
	uint32_t flash_target_offset = m_block * FLASH_BLOCK_SIZE + offset;
	uint32_t erase_size = (len + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE * FLASH_SECTOR_SIZE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, erase_size);
	flash_range_program(flash_target_offset, static_cast<const uint8_t*>(data), len);
	restore_interrupts(ints);
    return true;
}

/**
 * @brief フラッシュメモリからデータを読み出す
 * @details
 * 指定オフセット・長さ分のデータをバッファにコピーする。
 * @param offset 読み出し元オフセット（バイト）
 * @param data 読み出し先バッファへのポインタ
 * @param len 読み出すバイト数
 * @return 成功時true、範囲外や失敗時false
 */
bool FlashMem::read(uint32_t offset, void* data, uint32_t len) const
{
	if (offset + len > m_blockCount * FLASH_BLOCK_SIZE) return false;
	uint32_t flash_target_offset = m_block * FLASH_BLOCK_SIZE + offset;
	const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + flash_target_offset);
    std::memcpy(data, flash_ptr, len);
    return true;
}
