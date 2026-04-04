// SPDX-License-Identifier: BSD-3-Clause
#include "emu.h"
#include "cpu/m68000/m68020.h"
#include "machine/msm6242.h"
#include "machine/mc68681.h"
#include "machine/68230pit.h"
#include "bus/rs232/rs232.h"

namespace {

class another_state : public driver_device
{
public:
    another_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
        , m_cpu(*this, "cpu")
        , m_rtc(*this, "rtc")
        , m_duart(*this, "duart")
        , m_rs232_a(*this, "rs232_a")
        // , m_rs232_b(*this, "rs232_b")
    {}

    void another(machine_config &config);

protected:    
    virtual void machine_start() override;
    
private:
    required_device<m68020_device> m_cpu;
    required_device<rtc62421_device> m_rtc;
    required_device<mc68681_device> m_duart;
    required_device<rs232_port_device> m_rs232_a;
    // required_device<rs232_port_device> m_rs232_b;

    void mem_map(address_map &map);
};

void another_state::mem_map(address_map &map)
{
    map(0x00000000, 0x0000ffff).rom().region("rom", 0);
    map(0x00060000, 0x0006003f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));
    map(0x00070000, 0x0007003f).rw(m_rtc, FUNC(rtc62421_device::read), FUNC(rtc62421_device::write));
    map(0x08000000, 0x087fffff).ram().share("ram");
    map(0x02000000, 0x02001fff).rom().region("ae_config", 0);
    map(0x02200000, 0x029FFFFF).ram().share("ram2");
}

static DEVICE_INPUT_DEFAULTS_START(terminal_a)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END

void another_state::machine_start()
{
    printf("%s\n", __PRETTY_FUNCTION__);
}

void another_state::another(machine_config &config)
{
    M68020(config, m_cpu, 16_MHz_XTAL);
    m_cpu->set_addrmap(AS_PROGRAM, &another_state::mem_map);
    
    RTC62421(config, m_rtc, 32.768_kHz_XTAL);
    // m_rtc->out_int_handler().set_inputline(m_cpu, INPUT_LINE_IRQ6);

    MC68681(config, m_duart, 8_MHz_XTAL / 2);
    m_duart->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart->irq_cb().set_inputline(m_cpu, M68K_IRQ_5);

    RS232_PORT(config, m_rs232_a, default_rs232_devices, "terminal");
    m_duart->a_tx_cb().set(m_rs232_a, FUNC(rs232_port_device::write_txd));
    m_rs232_a->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_a_w));

    // RS232_PORT(config, m_rs232_b, default_rs232_devices, "terminal");
    // m_duart->b_tx_cb().set(m_rs232_b, FUNC(rs232_port_device::write_txd));
    // m_rs232_b->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_b_w));

    m_rs232_a->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_a));
    // m_rs232_b->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_b));

}

ROM_START(another)
    ROM_REGION32_BE(0x00010000, "rom", ROMREGION_ERASEFF)
    ROM_LOAD("another.bin", 0x000000, 0x00010000, CRC(1eb71799) SHA1(d07b181ff96bd282e0e4ec2558cc1c8a85364d6e))

    ROM_REGION32_BE(0x00002000, "ae_config", ROMREGION_ERASEFF)
    ROM_LOAD("ae_config", 0x00000000, 0x00002000, CRC(66823c33) SHA1(fbf489bb5c5a6921d7e000826262f8406b18ba52))
ROM_END

}

COMP(1989, another, 0, 0, another, 0, another_state,
     empty_init, "Uilli", "another", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
