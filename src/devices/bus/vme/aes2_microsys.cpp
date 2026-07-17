#include "aes2_microsys.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL|LOG_REGS)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AES2_MICROSYS, aesthedes2_vme_microsys_device, "aesthedes2_microsys", "Aesthedes2 VME Microsys");

void aesthedes2_vme_microsys_device::device_start()
{
    if (m_base_addr == 0)
        fatalerror("base address not set");

    vme_space(vme::AM_09).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_microsys_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_microsys_device::write32)));
    
    vme_space(vme::AM_0d).install_readwrite_handler(
        m_base_addr, m_base_addr + 0x1f,
        read32_delegate(*this, FUNC(aesthedes2_vme_microsys_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_microsys_device::write32)));
}

void aesthedes2_vme_microsys_device::device_add_mconfig(machine_config &config)
{
    WD37C65C(config, m_fdc, 16'000'000);
}

u32 aesthedes2_vme_microsys_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    switch (offset) {
        case 0x00:
            if (ACCESSING_BITS_24_31) {
                LOGFAIL("1 bits 24-31 unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
            } else if (ACCESSING_BITS_16_23) {
                auto msr = m_fdc->msr_r();
                LOGFAIL("2 MSR @%08x mask=%08x msr=%08x\n", m_base_addr + (offset << 2), mem_mask, msr);
                return msr << 16;
            } else if (ACCESSING_BITS_8_15) {
                LOGFAIL("3 unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
            } else if (ACCESSING_BITS_0_7) {
                LOGFAIL("4 unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
            }
            break;

        default:
            LOGFAIL("unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
            break;
    }
    return 0;
}

void aesthedes2_vme_microsys_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    LOGFAIL("unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
}
