// license:BSD-3-Clause
// copyright-holders:David Haywood

#ifndef MAME_MACHINE_VT1682_IO_H
#define MAME_MACHINE_VT1682_IO_H

#pragma once

DECLARE_DEVICE_TYPE(VT_VT1682_IO, vrt_vt1682_io_device)

class vrt_vt1682_io_device : public device_t
{
public:
	// construction/destruction
	vrt_vt1682_io_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto porta_out() { return m_porta_out.bind(); }
	auto portb_out() { return m_portb_out.bind(); }
	auto portc_out() { return m_portc_out.bind(); }
	auto portd_out() { return m_portd_out.bind(); }
	auto porta_in() { return m_porta_in.bind(); }
	auto portb_in() { return m_portb_in.bind(); }
	auto portc_in() { return m_portc_in.bind(); }
	auto portd_in() { return m_portd_in.bind(); }


	DECLARE_READ8_MEMBER(vt1682_210e_io_ab_r);
	DECLARE_READ8_MEMBER(vt1682_210f_io_cd_r);

	DECLARE_WRITE8_MEMBER(vt1682_210e_io_ab_w);
	DECLARE_WRITE8_MEMBER(vt1682_210f_io_cd_w);

	DECLARE_READ8_MEMBER(vt1682_210d_ioconfig_r);
	DECLARE_WRITE8_MEMBER(vt1682_210d_ioconfig_w);


protected:
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	uint8_t m_210d_ioconfig;

	// actually 4-bit ports
	devcb_write8 m_porta_out;
	devcb_write8 m_portb_out;
	devcb_write8 m_portc_out;
	devcb_write8 m_portd_out;
	devcb_read8 m_porta_in;
	devcb_read8 m_portb_in;
	devcb_read8 m_portc_in;
	devcb_read8 m_portd_in;
};

#endif // MAME_MACHINE_VT1682_IO_H
