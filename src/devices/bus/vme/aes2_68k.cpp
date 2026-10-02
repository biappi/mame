#include "aes2_68k.h"

#define LOG_IO      (1U << 1)
#define LOG_REGS    (1U << 2)

#define VERBOSE (LOG_IO | LOG_REGS)

#include "logmacro.h"

#define LOGIO(...)      LOGMASKED(LOG_IO, __VA_ARGS__)
#define LOGREGS(...)    LOGMASKED(LOG_REGS, __VA_ARGS__)

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

    m_framebuffer->clear(0x00ff00);
}

void aesthedes2_vme_68k_device::device_add_mconfig(machine_config &config)
{
	M68000(config, m_cpu, 8_MHz_XTAL);
    m_cpu->set_addrmap(AS_PROGRAM, &aesthedes2_vme_68k_device::main_map);

    AESTHEDES_FRAMEBUFFER(config, m_framebuffer);
    m_framebuffer->set_resolution(512, 512);

#if ENABLE_68K_SCREEN
    SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
    m_screen->set_refresh_hz(50);
    m_screen->set_size(512, 512);
    m_screen->set_visarea(0, 511, 0, 511);
    m_screen->set_screen_update(m_framebuffer, FUNC(aesthedes_framebuffer_device::screen_update));
#endif

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

    map(0xffc000, 0xffcfff).m(m_framebuffer, FUNC(aesthedes_framebuffer_device::map)).umask16(0x00ff);

    map(0xffffb000, 0xffffb6ff).rw(FUNC(aesthedes2_vme_68k_device::mistery_r), FUNC(aesthedes2_vme_68k_device::mistery_w));
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

u16 aesthedes2_vme_68k_device::mistery_r(offs_t offset, u16 mem_mask)
{
    const u32 address = 0xffffb000 + (offset << 1);

    // Known registers are on the low byte lane (...01).
    if (ACCESSING_BITS_0_7)
    {
        switch (address + 1)
        {
        case 0xffffb601:
            LOGREGS("%s: mistery read b6: status / ack register.\n", machine().describe_context());
            return 0x0000;

        default:
            logerror("%s: unknown HW read @ %08x mask=%04x\n",
                machine().describe_context(),
                address + 1,
                mem_mask);
            break;
        }
    }

    return 0xffff;
}

void aesthedes2_vme_68k_device::mistery_w(offs_t offset, u16 data, u16 mem_mask)
{
    const u32 address = 0xffffb000 + (offset << 1);

    if (ACCESSING_BITS_0_7)
    {
        const u8 value = data & 0xff;

        switch (address + 1)
        {
        case 0xffffb001:
            LOGREGS("%s: mistery write b0: command/control register = %02x\n", machine().describe_context(), value);
            m_mistery_device.m_command = value;
            break;

        case 0xffffb101:
            // Channel / entry selector
            LOGREGS("%s: mistery write b1: channel / entry selector = %02x\n", machine().describe_context(), value);
            m_mistery_device.m_selector = value;
            break;

        case 0xffffb201:
            // Unknown / not observed yet
            logerror("%s: HW B201 write = %02x\n",
                machine().describe_context(), value);
            break;

        case 0xffffb301:
            // Parameter data
            LOGREGS("%s: mistery write b3: parameter = %02x (%d)\n", machine().describe_context(), value, value);
            switch (m_mistery_device.m_command) {
                case 0x00:
                    // do nothing? idle?
                    break;

                case 0x01:
                    // set r
                    if (m_mistery_device.m_selector < 0x40) {
                        rgb_t const color = m_palette->pen_color(m_mistery_device.m_selector);
                        m_palette->set_pen_color(m_mistery_device.m_selector, value, color.g(), color.b());
                    } else {
                        logerror("%s: mistery write b3: parameter = %02x (%d) out of range\n", machine().describe_context(), value, value);
                    }
                    break;

                case 0x05:
                    // set g
                    if (m_mistery_device.m_selector < 0x40) {
                        rgb_t const color = m_palette->pen_color(m_mistery_device.m_selector);
                        m_palette->set_pen_color(m_mistery_device.m_selector, color.r(), value, color.b());
                    } else {
                        logerror("%s: mistery write b3: parameter = %02x (%d) out of range\n", machine().describe_context(), value, value);
                    }
                    break;

                case 0x09:
                    // set b
                    if (m_mistery_device.m_selector < 0x40) {
                        rgb_t const color = m_palette->pen_color(m_mistery_device.m_selector);
                        m_palette->set_pen_color(m_mistery_device.m_selector, color.r(), color.g(), value);
                    } else {
                        logerror("%s: mistery write b3: parameter = %02x (%d) out of range\n", machine().describe_context(), value, value);
                    }
                    break;

                case 0x11:
                case 0x15:
                case 0x19:
                    // commit? latch?
                    break;

            }   

            LOGREGS("%s: mistery write b3: parameter = %02x (%d)\n", machine().describe_context(), value, value);
            break;

        case 0xffffb401:
            // hw_data_port_a_w(value);
            LOGREGS("%s: mistery write b4: streaming data port A = %02x\n", machine().describe_context(), value);
            break;

        case 0xffffb501:
            // Streaming data port B
            LOGREGS("%s: mistery write b5: streaming data port B = %02x\n", machine().describe_context(), value);
            break;

        case 0xffffb601:
            // No writes observed yet
            logerror("%s: HW B601 write = %02x\n",
                machine().describe_context(), value);
            break;

        default:
            logerror("%s: unknown HW write @ %08x = %02x mask=%04x\n",
                machine().describe_context(),
                address + 1,
                value,
                mem_mask);
            break;
        }
    }

}
