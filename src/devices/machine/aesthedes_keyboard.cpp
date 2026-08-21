#include "aesthedes_keyboard.h"
#include "aesthedes_keycodes.h"

#define VERBOSE (1)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(AES2_KEYBOARD, aesthedes_keyboard_device, "aes2_keyboard", "Aesthedes 2 Keyboard")

#define DO_FAKE_KEYSTROKES 1

const size_t table_size = sizeof(keycode_table) / sizeof(uint16_t);

aesthedes_keyboard_device::aesthedes_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_KEYBOARD, tag, owner, clock)
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
    , m_out_b_port_func(*this)
{
}

void aesthedes_keyboard_device::device_start()
{
    m_fake_keystrokes_timer = timer_alloc(FUNC(aesthedes_keyboard_device::fake_keystrokes), this);
}

void aesthedes_keyboard_device::device_reset()
{
    m_out_a_port_func(0x0f);
    m_out_b_port_func(0xff);
    m_out_a_strobe_func(1);
#if DO_FAKE_KEYSTROKES
    m_fake_keystrokes_timer->adjust(attotime::from_msec(29000), 0, attotime::from_msec(1500));
#endif
    m_fake_keystrokes_count = 0;
}

TIMER_CALLBACK_MEMBER(aesthedes_keyboard_device::fake_keystrokes)
{
    u16 keycode = keycode_table[m_fake_keystrokes_count];
    LOG("sending keycode %d (%d/%d)\n", keycode, m_fake_keystrokes_count, table_size);

    m_out_a_port_func((u8)(keycode >> 8));
    m_out_b_port_func((u8)(keycode & 0xff));
    m_out_a_strobe_func(0);
    m_out_a_strobe_func(1);

    m_fake_keystrokes_count++;
    if (m_fake_keystrokes_count >= table_size) {
        m_fake_keystrokes_timer->enable(false);
    }
}

void aesthedes_keyboard_device::leds_pa_w(u8 data) {
    m_leds_latch = (m_leds_latch & 0x00ff) | (data << 8);
}

void aesthedes_keyboard_device::leds_pb_w(u8 data) {
    m_leds_latch = (m_leds_latch & 0xff00) | (data);
}

void aesthedes_keyboard_device::leds_x1_w(int state) {
    if (state == 0) {
        LOG("LEDs: %04x\n", m_leds_latch);
    }
}
