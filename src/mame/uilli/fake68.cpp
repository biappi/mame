// license:BSD-3-Clause

// based on https://www.retrobrewcomputers.org/doku.php?id=builderpages:plasmo:cb030:cb030_rev1:cb030r1_memmap
// in order to boot a build of https://github.com/John-Titor/os9-m68k-ports/tree/main/ports/CB030

/*

debugger snippets:

prints init module ptr
print d@(d@0 + 0x20) 

module dir start
print d@(d@(d@0 + 0x20) + $03C)

module dir end:
print d@(d@(d@0 + 0x20) + $03C)

*/

#include "emu.h"
#include "cpu/m68000/m68010.h"
#include "cpu/m68000/m68020.h"
#include "cpu/m68000/m68030.h"
#include "machine/mc68681.h"
#include "machine/nvram.h"
#include "machine/terminal.h"
#include "bus/rs232/rs232.h"
#include "bus/ata/ataintf.h"
#include "bus/ata/hdd.h"
#include "video/pwm.h"

#include "fake68.lh"

#include <stdio.h>
#include <signal.h>

#define RAMDISK_BASE 0xa0000000
#define RAMDISK_SIZE 0x01900000

#define RAMDISK_END  (RAMDISK_BASE + RAMDISK_SIZE - 1)

namespace {

class fake68_state : public driver_device
{
public:
    fake68_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
        , m_maincpu(*this, "maincpu")
        , m_ramdisk(*this, "nvram")
        , m_duart(*this, "duart")
        , m_duart1(*this, "duart1")
        , m_duart2(*this, "duart2")
        , m_duart3(*this, "duart3")
        , m_rs232_a(*this, "rs232_a")
        , m_rs232_b(*this, "rs232_b")
        , m_rs232_1(*this, "rs232_1")
        , m_rs232_2(*this, "rs232_2")
        , m_rs232_3(*this, "rs232_3")
        , m_ata(*this, "ata")
        , m_display(*this, "display")
    {
    }

    void fake68(machine_config &config);

protected:
    virtual void machine_reset() override;
    virtual void machine_start() override;

private:
    required_device<cpu_device> m_maincpu;
    required_device<nvram_device> m_ramdisk;
    required_device<mc68681_device> m_duart;
    required_device<mc68681_device> m_duart1;
    required_device<mc68681_device> m_duart2;
    required_device<mc68681_device> m_duart3;
    required_device<rs232_port_device> m_rs232_a;
    required_device<rs232_port_device> m_rs232_b;
    required_device<rs232_port_device> m_rs232_1;
    required_device<rs232_port_device> m_rs232_2;
    required_device<rs232_port_device> m_rs232_3;
    required_device<ata_interface_device> m_ata;
    required_device<pwm_display_device> m_display;

    void main_map(address_map &map);
    void cpu_map(address_map &map);

    void duart_output(uint8_t data);

    emu_timer *m_tick_timer;
    TIMER_CALLBACK_MEMBER(tick_timer);

    uint32_t bootvect_r(offs_t offset);
    void bootvect_w(offs_t offset, uint32_t data, uint32_t mem_mask);

    uint32_t remap_r(offs_t offset);
    void remap_w(offs_t offset, uint32_t data, uint32_t mem_mask);

    uint32_t timer_on_r(offs_t offset);
    void timer_on_w(offs_t offset, uint32_t data, uint32_t mem_mask);

    uint32_t timer_off_r(offs_t offset);
    void timer_off_w(offs_t offset, uint32_t data, uint32_t mem_mask);

    bool m_did_bootvect_hack;
    uint32_t m_bootvect_hack[2];

    uint8_t cf_r(offs_t offset);
    void cf_w(offs_t offset, uint8_t data);

    std::unique_ptr<uint8_t[]> m_ramdisk_data;

    uint8_t ramdisk_r(offs_t offset);
    void ramdisk_w(offs_t offset, uint8_t data);
};

void fake68_state::machine_reset()
{
    printf("%s\n", __PRETTY_FUNCTION__);
    // raise(SIGTRAP);

    m_duart->ip0_w(1);
    m_duart->ip1_w(1);
    m_duart->ip2_w(1);
    m_duart->ip3_w(1);
    m_duart->ip4_w(1);
    m_duart->ip5_w(1);
    m_duart->ip6_w(1);
}

void fake68_state::machine_start()
{
    printf("%s\n", __PRETTY_FUNCTION__);

    m_did_bootvect_hack = false;

    m_tick_timer = timer_alloc(FUNC(fake68_state::tick_timer), this);
    m_tick_timer->adjust(attotime::from_hz(100), 0, attotime::from_hz(100));
    m_tick_timer->enable(false);

    m_ramdisk_data = make_unique_clear<uint8_t[]>(RAMDISK_SIZE);
    m_ramdisk->set_base(&m_ramdisk_data[0], RAMDISK_SIZE);

    save_pointer(NAME(m_ramdisk_data), RAMDISK_SIZE);
}

void fake68_state::main_map(address_map &map)
{
    map(0x00000000, 0x00000007).rw(FUNC(fake68_state::bootvect_r), FUNC(fake68_state::bootvect_w));
    map(0x00000008, 0x07ffffff).ram();
    map(RAMDISK_BASE, RAMDISK_END).rw(FUNC(fake68_state::ramdisk_r), FUNC(fake68_state::ramdisk_w));
    map(0xfe000000, 0xfeffffff).rom().region("eprom", 0);
    map(0xffff8000, 0xffff8fff).rw(FUNC(fake68_state::remap_r), FUNC(fake68_state::remap_w));
    map(0xffff9800, 0xffff9fff).rw(FUNC(fake68_state::timer_on_r), FUNC(fake68_state::timer_on_w));
    map(0xffff9000, 0xffff97ff).rw(FUNC(fake68_state::timer_off_r), FUNC(fake68_state::timer_off_w));
    map(0xffffe000, 0xffffefff).rw(FUNC(fake68_state::cf_r), FUNC(fake68_state::cf_w));

    map(0xfffff000, 0xfffff03f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0xff00);
    map(0xfffff040, 0xfffff07f).rw(m_duart1, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0xff00);
    map(0xfffff080, 0xfffff0bf).rw(m_duart2, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0xff00);
    map(0xfffff0c0, 0xfffff0ef).rw(m_duart3, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0xff00);
}


void fake68_state::cpu_map(address_map &map)
{
    map(0xfffffff3, 0xfffffff3).lr8(NAME([]() { printf("avec 1"); return m68000_base_device::autovector(1); }));
    map(0xfffffff5, 0xfffffff5).lr8(NAME([]() { printf("avec 2"); return m68000_base_device::autovector(2); }));
    map(0xfffffff7, 0xfffffff7).lr8(NAME([]() { printf("avec 3"); return m68000_base_device::autovector(3); }));
    map(0xfffffff9, 0xfffffff9).lr8(NAME([]() { printf("avec 4"); return m68000_base_device::autovector(4); }));
    map(0xfffffffb, 0xfffffffb).lr8(NAME([]() { printf("avec 5"); return m68000_base_device::autovector(5); }));
    map(0xfffffffd, 0xfffffffd).lr8(NAME([]() { printf("avec 6"); return m68000_base_device::autovector(6); }));
    map(0xffffffff, 0xffffffff).lr8(NAME([]() { printf("avec 7"); return m68000_base_device::autovector(7); }));
}

uint32_t fake68_state::bootvect_r(offs_t offset)
{
    static const uint32_t bootvect_pc[] = {
        0x00000000,
        0xfe000494,
    };

    if (!m_did_bootvect_hack)
        return bootvect_pc[offset];
    else
        return m_bootvect_hack[offset];
}

void fake68_state::bootvect_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
    m_bootvect_hack[offset % sizeof(m_bootvect_hack)] &= ~mem_mask;
    m_bootvect_hack[offset % sizeof(m_bootvect_hack)] |= (data & mem_mask);
}

uint32_t fake68_state::remap_r(offs_t offset)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_did_bootvect_hack = true;
    return 0;
}

void fake68_state::remap_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_did_bootvect_hack = true;
}

uint32_t fake68_state::timer_on_r(offs_t offset)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_tick_timer->enable(true);
    return 0;
}

void fake68_state::timer_on_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_tick_timer->enable(true);
}

uint32_t fake68_state::timer_off_r(offs_t offset)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_tick_timer->enable(false);
    return 0;
}

void fake68_state::timer_off_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    m_tick_timer->enable(false);
}

uint8_t fake68_state::cf_r(offs_t offset)
{
    return m_ata->cs0_r(offset, 0xff);
}

void fake68_state::cf_w(offs_t offset, uint8_t data)
{
    m_ata->cs0_w(offset, data, 0xff);
}

void fake68_state::duart_output(uint8_t data)
{
    printf("%s got %02x %c\n", __PRETTY_FUNCTION__, data, data);
    m_display->write_mx(data);
    m_display->write_my(0x01);
}

uint8_t fake68_state::ramdisk_r(offs_t offset)
{
    return m_ramdisk_data[offset];
}

void fake68_state::ramdisk_w(offs_t offset, uint8_t data)
{
    m_ramdisk_data[offset] = data;
}

static INPUT_PORTS_START(fake68)
INPUT_PORTS_END

static DEVICE_INPUT_DEFAULTS_START(terminal_a)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

static DEVICE_INPUT_DEFAULTS_START(terminal_b)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

static DEVICE_INPUT_DEFAULTS_START(terminal_1)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

static DEVICE_INPUT_DEFAULTS_START(terminal_2)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

static DEVICE_INPUT_DEFAULTS_START(terminal_3)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

static void cfcard_option(device_slot_interface &device)
{
    device.option_add("cfcard", ATA_CF);
}

void fake68_state::fake68(machine_config &config)
{
    M68030(config, m_maincpu, 16_MHz_XTAL);
    m_maincpu->set_addrmap(AS_PROGRAM, &fake68_state::main_map);
    // m_maincpu->set_addrmap(m68030_device::AS_CPU_SPACE, &fake68_state::cpu_map);

    MC68681(config, m_duart, 8_MHz_XTAL / 2);
    m_duart->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart->outport_cb().set(FUNC(fake68_state::duart_output));
    m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_3);

    MC68681(config, m_duart1, 8_MHz_XTAL / 2);
    m_duart1->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart1->irq_cb().set_inputline(m_maincpu, M68K_IRQ_4);

    MC68681(config, m_duart2, 8_MHz_XTAL / 2);
    m_duart2->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart2->irq_cb().set_inputline(m_maincpu, M68K_IRQ_5);

    MC68681(config, m_duart3, 8_MHz_XTAL / 2);
    m_duart3->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart3->irq_cb().set_inputline(m_maincpu, M68K_IRQ_6);

    NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

    /* - */

    RS232_PORT(config, m_rs232_a, default_rs232_devices, "terminal");
    m_duart->a_tx_cb().set(m_rs232_a, FUNC(rs232_port_device::write_txd));
    m_rs232_a->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_a_w));

    RS232_PORT(config, m_rs232_b, default_rs232_devices, "terminal");
    m_duart->b_tx_cb().set(m_rs232_b, FUNC(rs232_port_device::write_txd));
    m_rs232_b->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_b_w));

    /* - */

    RS232_PORT(config, m_rs232_1, default_rs232_devices, "terminal");
    m_duart1->a_tx_cb().set(m_rs232_1, FUNC(rs232_port_device::write_txd));
    m_rs232_1->rxd_handler().set(m_duart1, FUNC(mc68681_device::rx_a_w));

    RS232_PORT(config, m_rs232_2, default_rs232_devices, "terminal");
    m_duart2->a_tx_cb().set(m_rs232_2, FUNC(rs232_port_device::write_txd));
    m_rs232_2->rxd_handler().set(m_duart2, FUNC(mc68681_device::rx_a_w));

    RS232_PORT(config, m_rs232_3, default_rs232_devices, "terminal");
    m_duart3->a_tx_cb().set(m_rs232_3, FUNC(rs232_port_device::write_txd));
    m_rs232_3->rxd_handler().set(m_duart3, FUNC(mc68681_device::rx_a_w));

    /* - */

    m_rs232_a->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_a));
    m_rs232_b->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_b));

    /* - */

    m_rs232_1->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_1));
    m_rs232_2->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_2));
    m_rs232_3->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_3));

    ATA_INTERFACE(config, m_ata).options(cfcard_option, "cfcard");

    PWM_DISPLAY(config, m_display).set_size(1, 7);
    m_display->set_segmask(0x3f, 0x7f);

    config.set_default_layout(layout_fake68);
}


TIMER_CALLBACK_MEMBER(fake68_state::tick_timer)
{
    m_maincpu->set_input_line(M68K_IRQ_6, ASSERT_LINE);
}

ROM_START(fake68)
    ROM_REGION32_BE(0xfe000000, "eprom", 0)
    // ROM_LOAD("romboot", 0x000000, 0x000158ac, CRC(3de4486c) SHA1(d7e86da86d1ab5b11eb856113d62c99a5e40d36c))
    // ROM_LOAD("romimage.dev", 0x000000, 0x00061a2e, CRC(0eef4a25) SHA1(0d63281839eac90dd15b107c33443cb05bf84ba0))
    ROM_LOAD("romimage.dev.patched-debugger", 0x00000000, 0x00070000, BAD_DUMP CRC(0eef4a25) SHA1(0d63281839eac90dd15b107c33443cb05bf84ba0))
ROM_END

}

SYST(2024, fake68, 0, 0, fake68, fake68, fake68_state, empty_init, "Uilli", "Fake 68k machine", MACHINE_IS_SKELETON);

