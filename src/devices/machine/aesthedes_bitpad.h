#ifndef MAME_MACHINE_AESTHEDES_BITPAD_H
#define MAME_MACHINE_AESTHEDES_BITPAD_H

#pragma once

#include "emu.h"


class aesthedes_bitpad_device : public device_t
{
public:
	aesthedes_bitpad_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock = 0);

    auto porta_strobe_cb() { return m_out_a_strobe_func.bind(); }
    auto porta_cb() { return m_out_a_port_func.bind(); }

	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

private:
    devcb_write_line m_out_a_strobe_func;
    devcb_write8 m_out_a_port_func;

    TIMER_CALLBACK_MEMBER(fake_movement);
    emu_timer *m_fake_movement_timer;
    int m_fake_movement_count;
};

DECLARE_DEVICE_TYPE(AES2_BITPAD, aesthedes_bitpad_device)

#endif