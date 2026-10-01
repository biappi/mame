#include "aes2_multilevel.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_GENERAL | LOG_REGS | LOG_FAIL)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL, __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_MULTILEVEL_GPU, aesthedes2_multilevel_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME GPU");

void aesthedes2_multilevel_gpu_device::device_add_mconfig(machine_config &config)
{

}

void aesthedes2_multilevel_gpu_device::device_start()
{
    vme_space(vme::AM_09).install_readwrite_handler(
		0xffb000, 0xffefff,
		read32_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::write32)));

	vme_space(vme::AM_0d).install_readwrite_handler(
		0xffb000, 0xffefff,
		read32_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::read32)),
		write32_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::write32)));
}

u32 aesthedes2_multilevel_gpu_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
	if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
		LOGFAIL("unknown read @%08x mask=%08x\n", 0xffb000 + (offset << 2), mem_mask);
        return 0;
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
	
    u8 data = 0;
    LOGREGS("%s reg READ  @%04x data=%02x\n", machine().describe_context(), card_offset, data);
	return data << shift;
}

void aesthedes2_multilevel_gpu_device::write32(address_space &space, offs_t offset, u32 data, u32 mem_mask)
{
    if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
        LOGFAIL("unknown write @%08x mask=%08x data=%08x\n", 0xffb000 + (offset << 2), mem_mask, data);
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

	LOGREGS("%s reg WRITE @%04x data=%02x\n", machine().describe_context(), card_offset, reg_data);
}