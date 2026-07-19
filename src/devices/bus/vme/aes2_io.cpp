#include "aes2_io.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL|LOG_REGS)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_IO, aesthedes2_vme_io_device, "aesthedes2_io", "Aesthedes2 VME I/O");

void aesthedes2_vme_io_device::device_start()
{
    if (m_base_addr == 0)
        fatalerror("base address not set");

    vme_space(vme::AM_09).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_io_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_io_device::write32)));

    vme_space(vme::AM_0d).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_io_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_io_device::write32)));
}

void aesthedes2_vme_io_device::device_add_mconfig(machine_config &config)
{
}

u32 aesthedes2_vme_io_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
        LOGFAIL("io: unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
        return 0;
    }

    int card_offset = (offset << 1);
    // int shift;
    if (ACCESSING_BITS_16_23) {
        // shift = 16;
    }
    if (ACCESSING_BITS_0_7) {
        card_offset += 1;
        // shift = 0;
    }

    LOGREGS("%s reg READ  @%02x\n", machine().describe_context(), card_offset);

    return 0;
}

void aesthedes2_vme_io_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    // LOGREGS("io: write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);

    if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
        LOGFAIL("io: unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
        return;
    }

    int card_offset = (offset << 1);
    int shift;
    if (ACCESSING_BITS_16_23) {
        shift = 16;
    }
    if (ACCESSING_BITS_0_7) {
        card_offset += 1;
        shift = 0;
    }

    uint8_t reg_data = data >> shift;

    LOGREGS("%s reg WRITE @%02x data=%02x\n", machine().describe_context(), card_offset, reg_data);
}