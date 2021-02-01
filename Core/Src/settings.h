/*
 * settings.h
 *
 *  Created on: 1 февр. 2021 г.
 *      Author: andrey
 */

#pragma once

#include <stdint.h>

#pragma pack(push)
class settings
{
public:
	settings();

	uint16_t m_version;
	uint16_t m_crc;

	uint8_t m_integration_time;
	uint8_t m_wait_time;
	uint8_t m_gain;
	double m_white;
	double m_red;
	double m_green;
	double m_blue;
	double m_total_factor;

	void set_default();
	void make_valid();
	bool is_valid();

	uint32_t max_count();
	double get_integration_time();
	double get_wait_time();
	double get_gain();

	double calc_normalized_from_raw(uint32_t w, uint32_t r, uint32_t g, uint32_t b);
};
#pragma pack(pop)
