#include "aesthedes_keyboard.h"

DEFINE_DEVICE_TYPE(AES2_KEYBOARD, aesthedes_keyboard_device, "aes2_keyboard", "Aesthedes 2 Keyboard")

aesthedes_keyboard_device::aesthedes_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_KEYBOARD, tag, owner, clock)
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
{
}

void aesthedes_keyboard_device::device_start()
{
}