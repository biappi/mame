// license:BSD-3-Clause
// copyright-holders:Enrico Gueli
#include "emu.h"
#include "pme6822.h"

#include "bus/nscsi/hd.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define VERBOSE (LOG_GENERAL)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(VME_PME6822,   vme_pme6822_card_device,   "pme6822",   "Radstone PME 68-22")

namespace {

static void scsi_devices(device_slot_interface &device)
{
	device.option_add("harddisk", NSCSI_HARDDISK);
}

} // anonymous namespace

vme_pme6822_card_device::vme_pme6822_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
    : device_t(mconfig, VME_PME6822, tag, owner, clock)
    , device_vme_card_interface(mconfig, *this)
    , m_maincpu(*this, "maincpu")
    , m_duart(*this, "duart")
    , m_rtc(*this, "rtc")
    , m_ncr(*this, "scsi:7:ncr5385")
    , m_duart_a_tx(*this)
    , m_eprom0_region("eprom0")
    , m_eprom1_region("eprom1")
    , m_ncr_int_cached_at(attotime::zero)
    , m_ncr_int_state(false)
    , m_ncr_dma_waiting(false)
    , m_ncr_transfer_counter(0)
    , m_ncr_transfer_counter_captured(0)
{
}

void vme_pme6822_card_device::set_eprom_regions(const char *eprom0, const char *eprom1)
{
    m_eprom0_region = eprom0;
    m_eprom1_region = eprom1;
}

void vme_pme6822_card_device::device_add_mconfig(machine_config &config)
{
	M68020(config, m_maincpu, 16670000);
    m_maincpu->set_addrmap(AS_PROGRAM, &vme_pme6822_card_device::main_map);

    MC68681(config, m_duart, 8_MHz_XTAL / 2);
    m_duart->set_clocks(500000, 500000, 1000000, 1000000);
    m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_5);
    m_duart->outport_cb().set(FUNC(vme_pme6822_card_device::duart_output));
    m_duart->a_tx_cb().set(FUNC(vme_pme6822_card_device::duart_a_tx));

    DS1216E(config, m_rtc);

    NSCSI_BUS(config, "scsi");
    NSCSI_CONNECTOR(config, "scsi:0", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:1", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:2", scsi_devices, "harddisk");
    NSCSI_CONNECTOR(config, "scsi:3", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:4", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:5", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:6", scsi_devices, nullptr);
    NSCSI_CONNECTOR(config, "scsi:7").option_set("ncr5385", NCR5385).clock(10'000'000).machine_config(
        [this](device_t *device)
        {
            ncr5385_device &adapter = downcast<ncr5385_device &>(*device);
            adapter.set_own_id(7);
            adapter.irq().set(*this, FUNC(vme_pme6822_card_device::ncr_irq_w));
            adapter.dreq().set(*this, FUNC(vme_pme6822_card_device::ncr_dreq));
        });
}

void vme_pme6822_card_device::main_map(address_map &map)
{
    // 64KB for OS-9 kernel ROM
    map(0x00000000, 0x0000ffff).rom().region(m_eprom0_region, 0);

    // 8KB for AE_CONFIG module
    map(0x00040000, 0x00041fff).rom().region(m_eprom1_region, 0);

    // 8MB RAM, according to system info printed by the "mfree" command
    map(0x08000000, 0x087fffff).ram();

    // more alleged RAM accessed by the ROM code
    map(0x02200000, 0x029fffff).ram();

    // MC2681P DUART
    map(0x00060000, 0x0006001f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write));

    // NCR 5385 SCSI (register file @ byte offsets 0x0-0xf; OS-9 touches e.g. 0x00020009)
    map(0x00020000, 0x0002000f).rw(FUNC(vme_pme6822_card_device::ncr_port_r), FUNC(vme_pme6822_card_device::ncr_port_w));

    // RAM for DMA transfers with the SCSI controller
    map(0x00030000, 0x00030003).rw(FUNC(vme_pme6822_card_device::ncr_dma_scratchpad_r), FUNC(vme_pme6822_card_device::ncr_dma_scratchpad_w));
}

void vme_pme6822_card_device::device_start()
{
	LOG("%s\n", FUNCNAME);

    save_item(NAME(m_ncr_int_cached_at));
    save_item(NAME(m_ncr_dma_waiting));
    // FIXME either change to vector or add support to save std::queues
    // save_item(NAME(m_ncr_dma_w_queue));
    // save_item(NAME(m_ncr_dma_r_queue));
    save_item(NAME(m_ncr_transfer_counter));
    save_item(NAME(m_ncr_transfer_counter_captured));

    // memory tap offers a tidy solution for the "phantom" rtc
	m_maincpu->space(AS_PROGRAM).install_read_tap(0x00041000, 0x00041fff, "rtc",
		[this](offs_t offset, u32 &data, u32 mem_mask)
		{
			if (ACCESSING_BITS_24_31)
			{
				if (m_rtc->ceo_r())
					data = (data & 0x00ffffffU) | u32(m_rtc->read(offset >> 2)) << 24;
				else
					m_rtc->read(offset >> 2);
			}
		});
}

void vme_pme6822_card_device::device_reset()
{
    LOG("%s\n", FUNCNAME);

    m_ncr_int_cached_at = attotime::zero;
    m_ncr_dma_waiting = false;
    m_ncr_dma_w_queue = std::queue<u8>();
    m_ncr_dma_r_queue = std::queue<u8>();
    m_ncr_transfer_counter = 0;
    m_duart->ip2_w(true);
}

u8 vme_pme6822_card_device::ncr_port_r(offs_t offset)
{
    LOG("NCR5385: reg_r(%x), %s\n", offset, machine().describe_context());
    if (offset == 5)
    {
        // In register 5 of NCR 5386, the lower three bits contain the controller's own ID.
        // In PME 68-22's case, the bit 5 also tells if the IRQ signal is/was asserted, 
        // or if the SEL signal in the SCSI bus is asserted.
        // Don't ask me why, the Aesthedes driver wants that.
        bool irq_was_asserted = ncr_int_cache_valid() || m_ncr_int_state;
        return ((irq_was_asserted ? 1 : 0) << 5) | 7;
    }

    return m_ncr->reg_r(offset);
}

void vme_pme6822_card_device::ncr_port_w(offs_t offset, u8 data)
{
    LOG("NCR5385: reg_w(%x, %02x)\n", offset, data);
    if (offset >= 0xC && offset < 0xF)
    {
        // capture the Transfer Counter value; will be used for DMA.
        offs_t counter_shift = (2 - (offset - 0xC)) * 8;
        m_ncr_transfer_counter_captured &= ~(0xFF << counter_shift);
        m_ncr_transfer_counter_captured |= (u32(data) << counter_shift);
        m_ncr_transfer_counter = m_ncr_transfer_counter_captured;
    }
    m_ncr->reg_w(offset, data);
}

void vme_pme6822_card_device::ncr_irq_w(int state)
{
    m_ncr_int_state = state;
    if (state == 1) {
        m_ncr_int_cached_at = machine().time();
    }

    m_maincpu->set_input_line(M68K_IRQ_3, state ? ASSERT_LINE : CLEAR_LINE);
}


bool vme_pme6822_card_device::ncr_int_cache_valid() const
{
    attotime now = machine().time();
    // Hold the cache valid for 50 microseconds: long enough to do a few reads after
    // an interrupt, short enough to avoid returning stale data for too long.
    bool valid = (now - m_ncr_int_cached_at) < attotime::from_usec(50);
    LOG("NCR5385: ncr_int_cache_valid() cached_at=%s now=%s valid=%d state=%d\n", m_ncr_int_cached_at.as_string(), now.as_string(), valid, m_ncr_int_state);
    return valid; 
}

u8 vme_pme6822_card_device::ncr_dma_scratchpad_r(offs_t offset)
{
    if (m_ncr_dma_r_queue.size() == 0) {
        return 0;
    }

    u8 data = m_ncr_dma_r_queue.front();
    m_ncr_dma_r_queue.pop();
    size_t size = m_ncr_dma_r_queue.size();
    LOG("NCR5385: dma_scratchpad_r(%x) -> %02x queue size=%d\n", offset, data, size);
    if (size == 0) {
        m_duart->ip2_w(true);
    }
    return data;
}

void vme_pme6822_card_device::ncr_dma_scratchpad_w(offs_t offset, u8 data)
{
    LOG("NCR5385: dma_scratchpad_w(%x, %02x)\n", offset, data);
    m_ncr_dma_w_queue.push(data);

    if (m_ncr_dma_waiting)
    {
        // if the NCR was waiting for data, that's the time to make it happy
        u8 data = m_ncr_dma_w_queue.front();
        m_ncr_dma_w_queue.pop();
        m_ncr->dma_w(data);

        m_ncr_dma_waiting = false;
    }
}

void vme_pme6822_card_device::ncr_dreq(int state)
{
    if (!state) {
        return;
    }
    bool dir_is_in = (m_ncr->reg_r(4) & 0x08) != 0;
    LOG("NCR5385: dreq(%d) queue size=%d count=%d dir=%s\n", state, m_ncr_dma_w_queue.size(), m_ncr_transfer_counter, dir_is_in ? "in" : "out");
    if (!dir_is_in) {
        if (m_ncr_dma_w_queue.empty()) {
            // if queue is empty, just note that we're waiting
            m_ncr_dma_waiting = true;
        } else if (state)
        {      
            u8 data = m_ncr_dma_w_queue.front();
            // if queue is not empty, write the next byte and pop the queue
            m_ncr->dma_w(data);
            m_ncr_dma_w_queue.pop();
            m_ncr_dma_waiting = false;
        }
    } else {
        // the CPU may read the entire block in one go (by reading the DMA scratchpad repeatedly),
        // so we must read it all in a FIFO of ours then flush it via the scratchpad.
        // Read is asyncronous i.e. the NCR will assert DREQ for each byte.
        u8 data = m_ncr->dma_r();
        m_ncr_dma_r_queue.push(data);
        m_ncr_transfer_counter--;

        bool transfer_in_progress = (m_ncr_transfer_counter > 0);
        if (!transfer_in_progress) {
            m_duart->ip2_w(false);
        }

        LOG("NCR5385: dreq read byte %02x queue size=%d in progress=%s\n", data, m_ncr_dma_r_queue.size(), transfer_in_progress ? "yes" : "no");
    }
}

void vme_pme6822_card_device::duart_output(uint8_t data)
{
    LOG("DUART_OUTPUT: %02X %c%c%c%c%c%c%c%c\n", data, 
        (data & 0x80) ? '7' : '.',
        (data & 0x40) ? '6' : '.',
        (data & 0x20) ? '5' : '.',
        (data & 0x10) ? '4' : '.',
        (data & 0x08) ? '3' : '.',
        (data & 0x04) ? '2' : '.',
        (data & 0x02) ? '1' : '.',
        (data & 0x01) ? '0' : '.');
}

void vme_pme6822_card_device::duart_a_tx(int state)
{
    m_duart_a_tx(state);
}