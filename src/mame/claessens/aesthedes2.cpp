#include "emu.h"

#include "bus/vme/vme.h"
#include "bus/vme/vme_cards.h"
#include "bus/vme/pme6822.h"
#include "bus/rs232/rs232.h"
#include "bus/vme/aes2_crtc.h"
#include "bus/vme/aes2_microsys.h"

#include "aesthedes2.lh"

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
    {
    }

    void aesthedes2(machine_config &config)
    {
        RS232_PORT(config, m_rs232_302, default_rs232_devices, "terminal");
        RS232_PORT(config, m_rs232_504, default_rs232_devices, "terminal");

        // Crate 3

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

        m_rs232_302->rxd_handler().set("crate3:02:pme6822", FUNC(vme_pme6822_card_device::rs232_rxd_w));
        m_rs232_302->set_option_device_input_defaults("terminal", DEVICE_INPUT_DEFAULTS_NAME(terminal_302));

        // Crate 5

        VME(config, "crate5");
        VME_SLOT(config, "crate5:04").option_set("pme6822", VME_PME6822).machine_config([this](device_t *dev) {
            auto &card = downcast<vme_pme6822_card_device &>(*dev);
            card.set_eprom_regions(":os9kernel", ":ae_config_504");
            card.rs232_tx_cb().set(m_rs232_504, FUNC(rs232_port_device::write_txd));
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

        config.set_default_layout(layout_aesthedes2);
    }

private:
    required_device<rs232_port_device> m_rs232_302;
    required_device<rs232_port_device> m_rs232_504;
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
