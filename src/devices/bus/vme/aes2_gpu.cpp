#include "aes2_gpu.h"

#define LOG_REGS    (1U << 1)

#define VERBOSE (LOG_REGS)

#include "logmacro.h"

#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_GPU, aesthedes2_vme_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME GPU");

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

u32 aesthedes2_vme_gpu_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
	LOGREGS("%s read @%08x mask=%08x\n", machine().describe_context(), m_base_addr + (offset << 2), mem_mask);
    // this should make m2dispsys routines happy. May emulate an EF9365 always ready for commands.
	return 0xffffffff;
}

void aesthedes2_vme_gpu_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
	LOGREGS("%s write @%08x mask=%08x data=%08x\n", machine().describe_context(), m_base_addr + (offset << 2), mem_mask, data);
}