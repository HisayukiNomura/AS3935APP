#pragma once
#include <cstdint>

class FlashMem {
public:
    /**
     * @brief フラッシュメモリの指定領域を管理するクラス
     * @param block 開始ブロック番号
     * @param blockCount 管理するブロック数
     */
    FlashMem(uint32_t block, uint32_t blockCount);
    ~FlashMem();
    /**
     * @brief フラッシュにデータを書き込む
     * @param offset ブロック先頭からのオフセット（バイト単位）
     * @param data 書き込むデータへのポインタ
     * @param len  書き込むバイト数（最大: m_blockCount * FLASH_SECTOR_SIZE）
     * @return 書き込み成功でtrue、失敗でfalse
     */
    bool write(uint32_t offset, const void* data, uint32_t len);
    /**
     * @brief フラッシュからデータを読み出す
     * @param offset ブロック先頭からのオフセット（バイト単位）
     * @param data 読み出し先バッファへのポインタ
     * @param len  読み出すバイト数（最大: m_blockCount * FLASH_SECTOR_SIZE）
     * @return 読み出し成功でtrue、失敗でfalse
     */
    bool read(uint32_t offset, void* data, uint32_t len) const;

private:
    uint32_t m_block;      ///< 開始ブロック番号
    uint32_t m_blockCount; ///< 管理するブロック数
};
