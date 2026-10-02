#include "aesthedes_bitpad.h"

#include "render.h"
#include "screen.h"

#define VERBOSE (1)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(AES2_BITPAD, aesthedes_bitpad_device, "aes2_bitpad", "Aesthedes 2 Bitpad")

static INPUT_PORTS_START(aesthedes_bitpad)
    PORT_START("X")
    PORT_BIT(0x03ff, 0x0200, IPT_LIGHTGUN_X) PORT_NAME("Bitpad X") PORT_CODE(GUNCODE_X) PORT_MINMAX(0x000, 0x03ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(aesthedes_bitpad_device::mouse_changed), 0)

    PORT_START("Y")
    PORT_BIT(0x03ff, 0x0200, IPT_LIGHTGUN_Y) PORT_NAME("Bitpad Y") PORT_CODE(GUNCODE_Y) PORT_MINMAX(0x000, 0x03ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(aesthedes_bitpad_device::mouse_changed), 0)

    PORT_START("BUTTONS")
    PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Bitpad Button") PORT_CODE(GUNCODE_BUTTON1) PORT_CHANGED_MEMBER(DEVICE_SELF, FUNC(aesthedes_bitpad_device::mouse_changed), 0)
INPUT_PORTS_END

aesthedes_bitpad_device::aesthedes_bitpad_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, AES2_BITPAD, tag, owner, clock)
    , m_screen(*this, finder_base::DUMMY_TAG)
    , m_x(*this, "X")
    , m_y(*this, "Y")
    , m_buttons(*this, "BUTTONS")
    , m_out_a_strobe_func(*this)
    , m_out_a_port_func(*this)
{
}

void aesthedes_bitpad_device::device_start()
{
    m_coordinate_timer = timer_alloc(FUNC(aesthedes_bitpad_device::send_coordinate_byte), this);

    save_item(NAME(m_pending_x));
    save_item(NAME(m_pending_y));
    save_item(NAME(m_pending_status));
    save_item(NAME(m_report_x));
    save_item(NAME(m_report_y));
    save_item(NAME(m_report_status));
    save_item(NAME(m_packet_byte));
    save_item(NAME(m_packet_active));
}

void aesthedes_bitpad_device::device_reset()
{
    m_pending_x = m_x->read();
    m_pending_y = m_y->read();
    m_pending_status = m_buttons->read();
    m_report_x = m_pending_x;
    m_report_y = m_pending_y;
    m_report_status = m_pending_status;

    m_out_a_port_func(0xff);
    m_out_a_strobe_func(1);

    m_coordinate_timer->adjust(attotime::never);
    m_packet_active = false;
    m_packet_byte = 0;
}

ioport_constructor aesthedes_bitpad_device::device_input_ports() const
{
    return INPUT_PORTS_NAME(aesthedes_bitpad);
}

INPUT_CHANGED_MEMBER(aesthedes_bitpad_device::mouse_changed)
{
    m_pending_x = m_x->read();
    m_pending_y = m_y->read();
    m_pending_status = m_buttons->read();

    if (!m_packet_active)
        begin_coordinate_packet();
}

void aesthedes_bitpad_device::begin_coordinate_packet()
{
    m_report_x = m_pending_x;
    m_report_y = m_pending_y;
    m_report_status = m_pending_status;
    m_packet_byte = 0;
    m_packet_active = true;
    m_coordinate_timer->adjust(attotime::zero);
}

std::pair<u16, u16> aesthedes_bitpad_device::screen_coordinates(u16 x, u16 y)
{
    static constexpr u16 input_max = 0x03ff;

    render_target &target = machine().render().ui_target();
    float mapped_x;
    float mapped_y;
    const s32 target_x = (u32(x) * (target.width() - 1)) / input_max;
    const s32 target_y = (u32(y) * (target.height() - 1)) / input_max;
    if (target.map_point_container(target_x, target_y, m_screen->container(), mapped_x, mapped_y)
        || ((mapped_x != -1.0F) && (mapped_y != -1.0F)))
    {
        return std::make_pair(
                u16(std::clamp(mapped_x, 0.0F, 1.0F) * input_max),
                u16(std::clamp(mapped_y, 0.0F, 1.0F) * input_max));
    }

    return std::make_pair(x, y);
}

TIMER_CALLBACK_MEMBER(aesthedes_bitpad_device::send_coordinate_byte)
{
    // The application maps roughly raw 1000..10000 to its usable coordinate
    // range.  Expand MAME's 10-bit absolute input to that interval before
    // splitting it into the Bitpad's 2+7+7-bit coordinate representation.
    static constexpr u16 raw_min = 1000;
    static constexpr u16 raw_max = 10000;
    static constexpr u16 input_max = 0x03ff;
    const auto expand_coordinate = [](u16 coordinate)
    {
        return raw_min + (u32(coordinate) * (raw_max - raw_min) / input_max);
    };

    // Lightgun input is normalized against the whole render target.  Map it
    // through the target's layout so the Bitpad follows its assigned screen.
    const auto [screen_x, screen_y] = screen_coordinates(m_report_x, m_report_y);

    const u16 x = expand_coordinate(screen_x);
    const u16 y = expand_coordinate(input_max - screen_y);
    u8 data;

    switch (m_packet_byte)
    {
    case 0:
        // Bit 7 starts a report.  The primary button is status bit 0,
        // which occupies bit 2 of the packet's first byte.
        data = 0x80 | ((m_report_status & 0x1f) << 2) | ((x >> 14) & 0x03);
        break;
    case 1:
        data = (x >> 7) & 0x7f;
        break;
    case 2:
        data = x & 0x7f;
        break;
    case 3:
        data = (y >> 14) & 0x03;
        break;
    case 4:
        data = (y >> 7) & 0x7f;
        break;
    default:
        data = y & 0x7f;
        break;
    }

    m_out_a_port_func(data);
    m_out_a_strobe_func(0);
    m_out_a_strobe_func(1);

    ++m_packet_byte;
    if (m_packet_byte < 6) {
        m_coordinate_timer->adjust(attotime::from_hz(1000));
    } else if ((m_pending_x != m_report_x) || (m_pending_y != m_report_y) || (m_pending_status != m_report_status)) {
        begin_coordinate_packet();
    } else {
        m_packet_active = false;
    }
}
