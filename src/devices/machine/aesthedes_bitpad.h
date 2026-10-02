#ifndef MAME_MACHINE_AESTHEDES_BITPAD_H
#define MAME_MACHINE_AESTHEDES_BITPAD_H

#pragma once

#include "emu.h"

#include <utility>


class aesthedes_bitpad_device : public device_t
{
public:
	aesthedes_bitpad_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

    auto porta_strobe_cb() { return m_out_a_strobe_func.bind(); }
    auto porta_cb() { return m_out_a_port_func.bind(); }

    template <typename T> void set_screen(T &&tag) { m_screen.set_tag(std::forward<T>(tag)); }

	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
    virtual ioport_constructor device_input_ports() const override;

    DECLARE_INPUT_CHANGED_MEMBER(mouse_changed);

private:
    required_device<screen_device> m_screen;
    required_ioport m_x;
    required_ioport m_y;
    required_ioport m_buttons;

    devcb_write_line m_out_a_strobe_func;
    devcb_write8 m_out_a_port_func;

    TIMER_CALLBACK_MEMBER(send_coordinate_byte);
    emu_timer *m_coordinate_timer;
    u16 m_pending_x;
    u16 m_pending_y;
    u8 m_pending_status;
    u16 m_report_x;
    u16 m_report_y;
    u8 m_report_status;
    u8 m_packet_byte;
    bool m_packet_active;

    void begin_coordinate_packet();
    std::pair<u16, u16> screen_coordinates(u16 x, u16 y);
};

DECLARE_DEVICE_TYPE(AES2_BITPAD, aesthedes_bitpad_device)

#endif
