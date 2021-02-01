/*
 * settings.cpp
 *
 *  Created on: 1 февр. 2021 г.
 *      Author: andrey
 */

#include <cmath>
#include "settings.h"

settings::settings()
{
}

void settings::set_default()
{
	m_version = 0xA0B0;

	m_integration_time = 0xF6;
	m_wait_time = 0;
	m_gain = 0;

	m_white = 1;
	m_red = 0;
	m_green = 0;
	m_blue = 0;
	m_total_factor = 461.48608;
}

uint16_t crc16(const uint8_t* data_p, uint32_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--)
	{
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }

    return crc;
}

void settings::make_valid()
{
	auto self = reinterpret_cast<uint8_t*>(this) + sizeof(uint16_t) * 2;
	auto self_size = sizeof(settings) - sizeof(uint16_t) * 2;

	m_crc = crc16(self, self_size);
}

bool settings::is_valid()
{
	if (m_version != 0xA0B0)
	{
		return false;
	}

	auto self = reinterpret_cast<uint8_t*>(this) + sizeof(uint16_t) * 2;
	auto self_size = sizeof(settings) - sizeof(uint16_t) * 2;

	auto crc = crc16(self, self_size);
	return m_crc == crc;
}

uint32_t settings::max_count()
{
	uint32_t count = 256;
	count -= m_integration_time;
	count *= 1024;

	if (count > 65535) count = 65535;

	return count;
}

double settings::get_integration_time()
{
	uint32_t count = 256;
	count -= m_integration_time;

	return 2.4 * (double)count;
}

double settings::get_wait_time()
{
	uint32_t count = 256;
	count -= m_integration_time;

	return 2.4 * (double)count;
}

double settings::get_gain()
{
	if (m_gain == 0) return 1;
	if (m_gain == 1) return 4;
	if (m_gain == 2) return 16;
	if (m_gain == 3) return 60;

	return NAN;
}

double settings::calc_normalized_from_raw(uint32_t w, uint32_t r, uint32_t g, uint32_t b)
{
	double max_raw = max_count();

	double nw = (double)w / max_raw;
	double nr = (double)r / max_raw;
	double ng = (double)g / max_raw;
	double nb = (double)b / max_raw;

	return nw * m_white + nr * m_red + ng * m_green + nb * m_blue;
}
