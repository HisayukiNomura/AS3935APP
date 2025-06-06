#include "Flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include <cstring>

// このファイルはFlashMem.cppにリネームされました

Flash::Flash(uint32_t block, uint32_t blockCount)
    : m_block(block), m_blockCount(blockCount) {}

Flash::~Flash() {}

bool Flash::write(uint32_t offset, const void* data, uint32_t len)
{
    if (offset + len > m_blockCount * FLASH_SECTOR_SIZE) return false;
    uint32_t flash_target_offset = m_block * FLASH_SECTOR_SIZE + offset;
    uint32_t erase_size = (len + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE * FLASH_SECTOR_SIZE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, erase_size);
    flash_range_program(flash_target_offset, static_cast<const uint8_t*>(data), len);
    restore_interrupts(ints);
    return true;
}

bool Flash::read(uint32_t offset, void* data, uint32_t len) const
{
    if (offset + len > m_blockCount * FLASH_SECTOR_SIZE) return false;
    uint32_t flash_target_offset = m_block * FLASH_SECTOR_SIZE + offset;
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + flash_target_offset);
    std::memcpy(data, flash_ptr, len);
    return true;
}
