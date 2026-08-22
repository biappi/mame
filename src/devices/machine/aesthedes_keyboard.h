#ifndef MAME_MACHINE_AESTHEDES_KEYBOARD_H
#define MAME_MACHINE_AESTHEDES_KEYBOARD_H

#pragma once

#include "emu.h"

#include <memory>

class aesthedes_keyboard_server;

class aesthedes_keyboard_device : public device_t
{
public:
	aesthedes_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);
    ~aesthedes_keyboard_device();

    auto porta_strobe_cb() { return m_out_a_strobe_func.bind(); }
    auto porta_cb() { return m_out_a_port_func.bind(); }
    auto portb_cb() { return m_out_b_port_func.bind(); }

    void leds_pa_w(u8 data);
    void leds_pb_w(u8 data);
    void leds_x1_w(int state);
    void inject_keycode(int keycode);

	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;

private:
    devcb_write_line m_out_a_strobe_func;
    devcb_write8 m_out_a_port_func;
    devcb_write8 m_out_b_port_func;

    std::unique_ptr<aesthedes_keyboard_server> m_server;
    u16 m_leds_latch;
};

DECLARE_DEVICE_TYPE(AES2_KEYBOARD, aesthedes_keyboard_device)

#endif