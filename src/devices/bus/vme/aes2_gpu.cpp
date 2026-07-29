#include "aes2_gpu.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_FAIL|LOG_REGS)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AES2_GPU, aesthedes2_vme_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME C100/065 GPU");

void aesthedes2_vme_gpu_device::device_start()
{
    if (m_base_addr == 0)
        fatalerror("base address not set");

    vme_space(vme::AM_09).install_readwrite_handler(
        m_base_addr, m_base_addr + 0xff,
        read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));

    vme_space(vme::AM_0d).install_readwrite_handler(
        m_base_addr, m_base_addr + 0xff,
        read32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::read32)),
        write32_delegate(*this, FUNC(aesthedes2_vme_gpu_device::write32)));
}

void aesthedes2_vme_gpu_device::device_add_mconfig(machine_config &config)
{
    // TODO: add the display system components here
}

u32 aesthedes2_vme_gpu_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    LOGFAIL("GPU: unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
    return 0;
}

void aesthedes2_vme_gpu_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    LOGFAIL("GPU: unknown write @%08x mask=%08x data=%08x\n", m_base_addr + (offset << 2), mem_mask, data);
}