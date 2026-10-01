#include "aes2_68k.h"

#define LOG_IO      (1U << 1)
#define LOG_G64     (1U << 6)

#define VERBOSE (0)

#include "logmacro.h"

#define LOGIO(...)      LOGMASKED(LOG_IO, __VA_ARGS__)


DEFINE_DEVICE_TYPE(VME_AESTHEDES2_68K, aesthedes2_vme_68k_device, "aesthedes2_68k", "Aesthedes2 68k (AES C100/0038)");

void aesthedes2_vme_68k_device::device_start()
{
    m_fake_irq6_timer = timer_alloc(FUNC(aesthedes2_vme_68k_device::fake_irq6_timer), this);
    m_clear_irq6_timer = timer_alloc(FUNC(aesthedes2_vme_68k_device::clear_irq6_timer), this);
}

void aesthedes2_vme_68k_device::device_reset()
{
    m_cpu_irq6(CLEAR_LINE);
    m_clear_irq6_timer->adjust(attotime::never);
    m_fake_irq6_timer->adjust(attotime::zero, 0, attotime::from_hz(50));
}

void aesthedes2_vme_68k_device::device_add_mconfig(machine_config &config)
{
	M68000(config, m_cpu, 8_MHz_XTAL);
    m_cpu->set_addrmap(AS_PROGRAM, &aesthedes2_vme_68k_device::main_map);

    PIA6821(config, m_pia_a);
    m_pia_a->ca2_handler().set([this](int state){
        LOGIO("PIA A write CA2: %d\n", state);
        m_connector_a_x20(state);
    });
    m_pia_a->cb2_handler().set([this](int state){
        LOGIO("PIA A write CB2: %d\n", state);
        m_connector_a_x1(state);
    });
    m_pia_a->writepa_handler().set([this](u8 data){
        LOGIO("PIA A write PA: 0x%02x\n", data);
        m_connector_a_porta(data);
    });
    m_pia_a->writepb_handler().set([this](u8 data){
        LOGIO("PIA A write PB: 0x%02x\n", data);
        m_connector_a_portb(data);
    });

    PIA6821(config, m_pia_b);
    PIA6821(config, m_pia_c);
    PIA6821(config, m_pia_d);

    m_cpu_irq6.bind().set_inputline(m_cpu, M68K_IRQ_6);
}

void aesthedes2_vme_68k_device::set_rom(const char *rom_name)
{
    m_rom_name = rom_name;
}

void aesthedes2_vme_68k_device::main_map(address_map &map)
{
    map(0x000000, 0x000007).rom().region(m_rom_name, 0);
    map(0x000008, 0x0fffff).ram();
    map(0x800000, 0x80ffff).rom().region(m_rom_name, 0);

    map(0xff0000, 0xff0003).rw(m_pia_a, FUNC(pia6821_device::read_alt), FUNC(pia6821_device::write_alt));
    map(0xff0004, 0xff0007).rw(m_pia_b, FUNC(pia6821_device::read_alt), FUNC(pia6821_device::write_alt));
    map(0xff0008, 0xff000b).rw(m_pia_c, FUNC(pia6821_device::read_alt), FUNC(pia6821_device::write_alt));
    map(0xff000c, 0xff000f).rw(m_pia_d, FUNC(pia6821_device::read_alt), FUNC(pia6821_device::write_alt));

    // window to other cards in backplane
    map(0xffb000, 0xffefff).rw(FUNC(aesthedes2_vme_68k_device::g64_ext_r), FUNC(aesthedes2_vme_68k_device::g64_ext_w));
}

u16 aesthedes2_vme_68k_device::g64_ext_r(offs_t offset, u16 mem_mask)
{
    u16 read = device_vme_card_interface::vme_read16<vme::AM_0d, 0xffb000>(offset, mem_mask);
    LOGMASKED(LOG_G64, "G64: read %08x mask %08x -> %08x\n", offset, mem_mask, read);
    return read;
}

void aesthedes2_vme_68k_device::g64_ext_w(offs_t offset, u16 data, u16 mem_mask)
{
    device_vme_card_interface::vme_write32<vme::AM_0d, 0xffb000>(offset, data, mem_mask);
}


TIMER_CALLBACK_MEMBER(aesthedes2_vme_68k_device::fake_irq6_timer)
{
    m_cpu_irq6(ASSERT_LINE);
    m_clear_irq6_timer->adjust(attotime::from_usec(1));
}

TIMER_CALLBACK_MEMBER(aesthedes2_vme_68k_device::clear_irq6_timer)
{
    m_cpu_irq6(CLEAR_LINE);
}