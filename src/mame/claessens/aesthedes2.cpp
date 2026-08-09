#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "bus/vme/pme6822.h"
#include "bus/rs232/rs232.h"
#include "bus/vme/aes2_crtc.h"
#include "bus/vme/aes2_microsys.h"
#include "bus/vme/aes2_io.h"

#include "machine/aesthedes_bitpad.h"

#include "aesthedes2.lh"

// Selectively enable parts of the system. Good to troubleshoot/investigate/debug.
// Crate 3 and 5 will require each an hard disk. You may need to adjust the command line
// (-harddisk / -harddisk1 / -harddisk2) to make it work.
#define ENABLE_CRATE_3 1
#define ENABLE_CRATE_5 1

namespace {


static DEVICE_INPUT_DEFAULTS_START(terminal_302)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END


static DEVICE_INPUT_DEFAULTS_START(terminal_504)
    DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_19200 )
    DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
    DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
    DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
DEVICE_INPUT_DEFAULTS_END


class aesthedes2_state : public driver_device
{
public:
    aesthedes2_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
        , m_rs232_302(*this, "rs232_302")
        , m_rs232_504(*this, "rs232_504")
        , m_bitpad(*this, "bitpad")
    {
    }

    void aesthedes2(machine_config &config)
    {
        RS232_PORT(config, m_rs232_302, default_rs232_devices, "terminal");
        RS232_PORT(config, m_rs232_504, default_rs232_devices, "terminal");

#if ENABLE_CRATE_3

        VME(config, "crate3");
        VME_SLOT(config, "crate3:02").option_set("pme6822", VME_PME6822).machine_config([this](device_t *dev) {
            auto &card = downcast<vme_pme6822_card_device &>(*dev);
            card.set_eprom_regions(":os9kernel", ":ae_config_302");
            card.rs232_tx_cb().set(m_rs232_302, FUNC(rs232_port_device::write_txd));
        });

        // TODO: crate name
        VME_SLOT(config, "crate3:03").option_set("aesthedes2_microsys",VME_AES2_MICROSYS ).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_microsys_device &>(*dev);
            card.set_base_address(0x04f11000);
        });
        VME_SLOT(config, "crate3:20").option_set("aesthedes2_io", VME_AESTHEDES2_IO).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_io_device &>(*dev);
            card.set_base_address(0x04fc9000);
#if ENABLE_CRATE_5
            // Connect to crate 5. Note: flat cable is reversed!
            card.connector_a().portb_cb().set(":crate5:16:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_b_pa_w));
            card.connector_a().x1_cb().set(":crate5:16:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_b_x2_w));
            card.connector_a().x2_cb().set(":crate5:16:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_b_x1_w));
            card.connector_a().x19_cb().set(":crate5:16:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_b_x20_w));
            card.connector_a().x20_cb().set(":crate5:16:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_b_x19_w));
#endif
        });

        m_rs232_302->rxd_handler().set("crate3:02:pme6822", FUNC(vme_pme6822_card_device::rs232_rxd_w));
        m_rs232_302->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_302));
#endif


#if ENABLE_CRATE_5
        VME(config, "crate5");
        VME_SLOT(config, "crate5:04").option_set("pme6822", VME_PME6822).machine_config([this](device_t *dev) {
            auto &card = downcast<vme_pme6822_card_device &>(*dev);
            card.set_eprom_regions(":os9kernel", ":ae_config_504");
            card.rs232_tx_cb().set(m_rs232_504, FUNC(rs232_port_device::write_txd));
        });
        VME_SLOT(config, "crate5:16").option_set("aesthedes2_io", VME_AESTHEDES2_IO).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_io_device &>(*dev);
            card.set_base_address(0x04fc9000);
#if ENABLE_CRATE_3
            // Connect to crate 3. Note: flat cable is reversed!
            card.connector_b().portb_cb().set(":crate3:20:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_pa_w));
            card.connector_b().x1_cb().set(":crate3:20:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_x2_w));
            card.connector_b().x2_cb().set(":crate3:20:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_x1_w));
            card.connector_b().x19_cb().set(":crate3:20:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_x20_w));
            card.connector_b().x20_cb().set(":crate3:20:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_x19_w));
#endif
        });
        VME_SLOT(config, "crate5:17").option_set("aesthedes2_io", VME_AESTHEDES2_IO).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_io_device &>(*dev);
            card.set_base_address(0x04fc9020);
        });
        VME_SLOT(config, "crate5:18").option_set("aesthedes2_crtc", VME_AESTHEDES2_CRTC).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_crtc_device &>(*dev);
            card.set_base_address(0x04ffa000);
        });
        VME_SLOT(config, "crate5:19").option_set("aesthedes2_crtc", VME_AESTHEDES2_CRTC).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_crtc_device &>(*dev);
            card.set_base_address(0x04ffa020);
        });
        VME_SLOT(config, "crate5:20").option_set("aesthedes2_crtc", VME_AESTHEDES2_CRTC).machine_config([this](device_t *dev) {
            (this);
            auto &card = downcast<aesthedes2_vme_crtc_device &>(*dev);
            card.set_base_address(0x04ffa040);
        });

        m_rs232_504->rxd_handler().set("crate5:04:pme6822", FUNC(vme_pme6822_card_device::rs232_rxd_w));
        m_rs232_504->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_504));

        AES2_BITPAD(config, m_bitpad);
        m_bitpad->porta_cb().set("crate5:17:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_pa_w));
        m_bitpad->porta_strobe_cb().set("crate5:17:aesthedes2_io", FUNC(aesthedes2_vme_io_device::connector_a_x2_w));
#endif

        config.set_default_layout(layout_aesthedes2);
    }

private:
    required_device<rs232_port_device> m_rs232_302;
    required_device<rs232_port_device> m_rs232_504;

    required_device<aesthedes_bitpad_device> m_bitpad;
};

static INPUT_PORTS_START(aesthedes2)
INPUT_PORTS_END

}

ROM_START(aesthedes2)
    ROM_REGION32_BE(0x10000, "os9kernel", 0)
    ROM_LOAD("os9kernel.bin", 0x0000, 0x10000, CRC(1eb71799))

    ROM_REGION32_BE(0x2000, "ae_config_504", 0)
    ROM_LOAD("ae_config_504.bin", 0x0000, 0x2000, CRC(66823c33))

    ROM_REGION32_BE(0x2000, "ae_config_302", 0)
    ROM_LOAD("ae_config_302.bin", 0x0000, 0x2000, CRC(4c618d47))
ROM_END

SYST(1985+, aesthedes2, 0, 0, aesthedes2, aesthedes2, aesthedes2_state, empty_init, "Claessens Product Consultants", "Aesthedes 2", MACHINE_NO_SOUND);
