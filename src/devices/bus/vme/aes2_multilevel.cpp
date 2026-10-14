#include "aes2_multilevel.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_GENERAL | LOG_FAIL)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL, __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)

DEFINE_DEVICE_TYPE(VME_AESTHEDES2_MULTILEVEL_GPU, aesthedes2_multilevel_gpu_device, "aesthedes2_gpu", "Aesthedes2 VME GPU");

void aesthedes2_multilevel_gpu_device::device_add_mconfig(machine_config &config)
{

}

void aesthedes2_multilevel_gpu_device::device_start()
{
	vme_space(vme::AM_29).install_readwrite_handler(
		0x0000, 0xffff,
		read16_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::read16)),
		write16_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::write16)));

	vme_space(vme::AM_2d).install_readwrite_handler(
		0x0000, 0xffff,
		read16_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::read16)),
		write16_delegate(*this, FUNC(aesthedes2_multilevel_gpu_device::write16)));
}

u16 aesthedes2_multilevel_gpu_device::read16(address_space &space, offs_t offset, u16 mem_mask)
{
    if (offset >= 0x8000) {
        LOGFAIL("unexpected read offset %08x\n", offset);
        return 0;
    }

    int card_offset = (offset << 1);

	if (!(ACCESSING_BITS_0_7)) {
		LOGFAIL("unknown read @%04x mask=%04x\n", card_offset, mem_mask);
        return 0;
    }

    int shift;
    if (ACCESSING_BITS_0_7) {
		card_offset += 1;
        shift = 0;
    }
	
    if ((card_offset & 0xff) != 0x01) {
        LOGFAIL("cannot intepret %04x as 8-bit register offset\n", card_offset);
        return 0;
    }

    u8 reg8_offset = card_offset >> 8;
    
    u8 data = reg8_read(reg8_offset);
	return data << shift;
}

void aesthedes2_multilevel_gpu_device::write16(address_space &space, offs_t offset, u16 data, u16 mem_mask)
{
    if (offset >= 0x8000) {
        LOGFAIL("unexpected write offset %08x\n", offset);
        return;
    }

    int card_offset = (offset << 1);
    
    if (!(ACCESSING_BITS_0_7)) {
        LOGFAIL("unknown write @%04x mask=%04x data=%04x\n", card_offset, mem_mask, data);
        return;
    }

    int shift;
    if (ACCESSING_BITS_0_7) {
        card_offset += 1;
        shift = 0;
    }

    if ((card_offset & 0xff) != 0x01) {
        LOGFAIL("cannot intepret %04x as 8-bit register offset\n", card_offset);
        return;
    }
    
    u8 reg8_offset = card_offset >> 8;
    uint8_t reg8_data = data >> shift;
    reg8_write(reg8_offset, reg8_data);
}

u8 aesthedes2_multilevel_gpu_device::reg8_read(offs_t offset)
{
    u8 data = 0;
    LOGREGS("%s reg READ  @%02x data=%02x\n", machine().describe_context(), offset, data);
    return data;
}

void aesthedes2_multilevel_gpu_device::reg8_write(offs_t offset, u8 data)
{
    LOGREGS("%s reg WRITE @%02x data=%02x\n", machine().describe_context(), offset, data);

    switch (offset) {
        case 0xB0:
            // level command register
            if ((data & 0xf0) == 0) {
                // we can ignore "begin write" commands, the channel can be figured out
                // with the "end write" command.
            } else {
                switch (data & 0x0f) {
                    case 1: 
                        m_level_colors[m_current_level].set_r(m_data_register);
                        LOG("%s: write r channel of level %u to %u\n", machine().describe_context(), m_current_level, m_data_register);
                        break;
                    case 5: 
                        m_level_colors[m_current_level].set_g(m_data_register);
                        LOG("%s: write g channel of level %u to %u\n", machine().describe_context(), m_current_level, m_data_register);
                        break;
                    case 9: 
                        m_level_colors[m_current_level].set_b(m_data_register);
                        LOG("%s: write b channel of level %u to %u\n", machine().describe_context(), m_current_level, m_data_register);
                        break;
                    default:
                        LOGFAIL("%s: level register %02X: unknown command %02X\n", machine().describe_context(), offset, data);
                }
            }
            break;
        case 0xB1:
            // level select register
            if (data >= 64) {
                LOGFAIL("%s: level register %02X: invalid level %u\n", machine().describe_context(), offset, data);
            } else {
                m_current_level = data;
            }
            break;
        case 0xB3:
            // level data register
            m_data_register = data;
            break;
        default:
            LOGFAIL("%s: write to unknown register %02X (data %02X)\n", machine().describe_context(), offset, data);
    }
}