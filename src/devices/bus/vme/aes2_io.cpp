#include "aes2_io.h"

#define LOG_FAIL    (1U << 1)
#define LOG_REGS    (1U << 2)
#define LOG_IO      (1U << 3)

#define VERBOSE (LOG_FAIL|LOG_REGS|LOG_IO)

#include "logmacro.h"

#define LOGFAIL(...)    LOGMASKED(LOG_FAIL,  __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS,  __VA_ARGS__)
#define LOGIO(...)      LOGMASKED(LOG_IO, __VA_ARGS__)

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
    PIA6821(config, m_pia_a);
    PIA6821(config, m_pia_b);
    PIA6821(config, m_pia_c);

    m_pia_a->ca2_handler().set([this](int state){
        LOGIO("PIA A write CA2: %d\n", state);
    });
    m_pia_a->cb2_handler().set([this](int state){
        LOGIO("PIA A write CB2: %d\n", state);
    });
    m_pia_a->writepb_handler().set([this](u8 data){
        LOGIO("PIA A write PB: 0x%02x\n", data);
    });
    m_pia_b->ca2_handler().set([this](int state){
        LOGIO("PIA B write CA2: %d\n", state);
    });
    m_pia_b->cb2_handler().set([this](int state){
        LOGIO("PIA B write CB2: %d\n", state);
    });
    m_pia_b->writepb_handler().set([this](u8 data){
        LOGIO("PIA B write PB: 0x%02x\n", data);
    });
}

u32 aesthedes2_vme_io_device::read32(address_space &space, offs_t offset, u32 mem_mask)
{
    if (!(ACCESSING_BITS_16_23) && !(ACCESSING_BITS_0_7)) {
        LOGFAIL("io: unknown read @%08x mask=%08x\n", m_base_addr + (offset << 2), mem_mask);
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

    int pia_index = (card_offset >> 2) & 0x3;
    offs_t pia_reg = card_offset & 0x3;
    u8 data;
    switch (pia_index) {
        case 0: data = m_pia_a->read_alt(pia_reg); break;
        case 1: data = m_pia_b->read_alt(pia_reg); break;
        case 2: data = m_pia_c->read_alt(pia_reg); break;
        default:
            LOGFAIL("invalid PIA index 3\n");
            return 0;
    }

    LOGREGS("%s reg READ  @%02x data=%02x\n", machine().describe_context(), card_offset, data);
    return data << shift;
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

    int pia_index = (card_offset >> 2) & 0x3;
    offs_t pia_reg = card_offset & 0x3;
    switch (pia_index) {
        case 0: m_pia_a->write_alt(pia_reg, reg_data); return;
        case 1: m_pia_b->write_alt(pia_reg, reg_data); return;
        case 2: m_pia_c->write_alt(pia_reg, reg_data); return;
        default:
            LOGFAIL("invalid PIA index 3\n");
            return;
    }
}