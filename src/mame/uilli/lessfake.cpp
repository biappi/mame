// SPDX-License-Identifier: BSD-3-Clause
#include "emu.h"
#include "cpu/m68000/m68020.h"
#include "machine/msm6242.h"
#include "machine/mc68681.h"
#include "bus/rs232/rs232.h"

namespace {

class lessfake_state : public driver_device
{
public:
    lessfake_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
        , m_cpu(*this, "cpu")
        , m_ram(*this, "ram")
        , m_rtc(*this, "rtc")
        , m_duart(*this, "duart")
        , m_rs232_a(*this, "rs232_a")
        , m_rs232_b(*this, "rs232_b")
    {}

    void lessfake(machine_config &config);

protected:    
    virtual void machine_start() override;
    
private:
    required_device<m68020_device> m_cpu;
    required_shared_ptr<u32> m_ram;
    required_device<rtc62421_device> m_rtc;
    required_device<mc68681_device> m_duart;
    required_device<rs232_port_device> m_rs232_a;
    required_device<rs232_port_device> m_rs232_b;

    bool m_did_bootvect_hack;
    uint32_t m_bootvect_hack[2];

    uint32_t bootvect_r(offs_t offset);
    void bootvect_w(offs_t offset, uint32_t data, uint32_t mem_mask);

    uint32_t unk1_r(offs_t offset);
    void unk1_w(offs_t offset, uint32_t data, uint32_t mem_mask);
    
    void mem_map(address_map &map);

    u8 ctrl_r(offs_t offset);
    void ctrl_w(offs_t offset, u8 data);
};


void lessfake_state::machine_start()
{
    printf("%s\n", __PRETTY_FUNCTION__);

    m_did_bootvect_hack = false;
}

/* -------------------------------------------------
 * Board control registers (FFFE0000)
 * ------------------------------------------------- */

u8 lessfake_state::ctrl_r(offs_t offset)
{
    return 0xff;
}

void lessfake_state::ctrl_w(offs_t offset, u8 data)
{
    logerror("CTRL W @ FFFE%04X = %02X\n", offset * 2, data);
}

/* -------------------------------------------------
 * Memory map
 * ------------------------------------------------- */

 uint32_t lessfake_state::bootvect_r(offs_t offset)
 {
    // copied from the ROM because i don't know how to properly implement mirroring
    static const uint32_t bootvect_pc[] = {
        0x00002000,
        0xfff004ba,
    };
 
    if (!m_did_bootvect_hack)
        return bootvect_pc[offset];
    else
        return m_bootvect_hack[offset];
 }
 
void lessfake_state::bootvect_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{   
    m_bootvect_hack[offset % sizeof(m_bootvect_hack)] &= ~mem_mask;
    m_bootvect_hack[offset % sizeof(m_bootvect_hack)] |= (data & mem_mask);
}

uint32_t lessfake_state::unk1_r(offs_t offset)
{
    return 0;
}

void lessfake_state::unk1_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
    // the first writes are to this dervice... maybe the boot remap?
    printf("%s write: %08X %08X %08X\n", __PRETTY_FUNCTION__, offset, data, mem_mask);
    if (offset == 0x000000f >> 3)
        m_did_bootvect_hack = true;
}


void lessfake_state::mem_map(address_map &map)
{
    map(0x00000000, 0x00000007).rw(FUNC(lessfake_state::bootvect_r), FUNC(lessfake_state::bootvect_w));
    map(0x00000008, 0x07ffffff).ram().share("ram");

    map(0xfff80000, 0xfff8ffff).rw(FUNC(lessfake_state::unk1_r), FUNC(lessfake_state::unk1_w));
    map(0xfffe0000, 0xfffe00ff).rw(FUNC(lessfake_state::ctrl_r), FUNC(lessfake_state::ctrl_w));

    map(0xfff00000, 0xfff7ffff).rom().region("rom", 0);
    map(0xfff80000, 0xfff8003f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));
    map(0xfff90000, 0xfff9000f).rw(m_rtc, FUNC(rtc62421_device::read), FUNC(rtc62421_device::write));
}

static DEVICE_INPUT_DEFAULTS_START(terminal_a)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_9600 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_9600 )
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

/* -------------------------------------------------
 * Machine configuration
 * ------------------------------------------------- */

void lessfake_state::lessfake(machine_config &config)
{
    M68020(config, m_cpu, 16_MHz_XTAL);
    m_cpu->set_addrmap(AS_PROGRAM, &lessfake_state::mem_map);
    
    RTC62421(config, m_rtc, 32.768_kHz_XTAL);
    // m_rtc->out_int_handler().set_inputline(m_cpu, INPUT_LINE_IRQ6);

    MC68681(config, m_duart, 8_MHz_XTAL / 2);
    m_duart->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart->irq_cb().set_inputline(m_cpu, M68K_IRQ_5);

    RS232_PORT(config, m_rs232_a, default_rs232_devices, "terminal");
    m_duart->a_tx_cb().set(m_rs232_a, FUNC(rs232_port_device::write_txd));
    m_rs232_a->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_a_w));

    RS232_PORT(config, m_rs232_b, default_rs232_devices, "terminal");
    m_duart->b_tx_cb().set(m_rs232_b, FUNC(rs232_port_device::write_txd));
    m_rs232_b->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_b_w));

    m_rs232_a->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_a));
    m_rs232_b->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_b));
}

/* -------------------------------------------------
 * ROM definition
 * ------------------------------------------------- */

ROM_START(lessfake)
    ROM_REGION32_BE(0xfff00000, "rom", ROMREGION_ERASEFF)
    ROM_LOAD("lessfake.bin", 0x000000, 0x00020000, BAD_DUMP CRC(0eef4a25) SHA1(0d63281839eac90dd15b107c33443cb05bf84ba0))
ROM_END

} // anonymous namespace

/* -------------------------------------------------
 * System definition
 * ------------------------------------------------- */

COMP(1989, lessfake, 0, 0, lessfake, 0, lessfake_state,
     empty_init, "Uilli", "lessfake", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)

