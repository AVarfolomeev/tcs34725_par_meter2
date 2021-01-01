/*
 * board.cpp
 *
 *  Created on: Dec 25, 2020
 *      Author: andrey
 */
#include <stdarg.h>
#include "board.h"
#include "usbd_cdc_if.h"
#include "spi1106.h"

extern I2C_HandleTypeDef hi2c1;

static volatile bool i2c_tx_completed = false;
static volatile bool i2c_rx_completed = false;

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	i2c_tx_completed = true;
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	i2c_rx_completed = true;
}

void i2c_write(uint8_t *pData, uint16_t Size)
{
	//i2c_tx_completed = false;
	//if (HAL_I2C_Master_Transmit_DMA(&hi2c1, 0x29 << 1, pData, Size) != HAL_OK)
	if (HAL_I2C_Master_Transmit(&hi2c1, 0x29 << 1, pData, Size, 1000) != HAL_OK)
	{
		return;
	}

	//while (!i2c_tx_completed);
}

void i2c_read(uint8_t *pData, uint16_t Size)
{
	//i2c_rx_completed = false;
	//if (HAL_I2C_Master_Receive_DMA(&hi2c1, 0x29 << 1, pData, Size) != HAL_OK)
	if (HAL_I2C_Master_Receive(&hi2c1, 0x29 << 1, pData, Size, 1000) != HAL_OK)
	{
		return;
	}

	//while (!i2c_rx_completed);
}

void i2c_write_register(uint8_t reg, uint8_t data)
{
	uint8_t buf[2];
	buf[0] = reg |= 0x80;
	buf[1] = data;

	i2c_write(buf, 2);
}

uint8_t i2c_read_register(uint8_t reg)
{
	reg |= 0x80;

	i2c_write(&reg, 1);

	uint8_t res = 0;
	i2c_read(&res, 1);

	return res;
}

uint32_t i2c_read_word(uint8_t reg)
{
	reg |= 0xA0;

	i2c_write(&reg, 1);

	uint8_t res[2];
	i2c_read(res, 2);

	return (uint32_t)(res[1] << 8) + (uint32_t)res[0];
}

char log_buf[256];

void log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int nn = vsnprintf(log_buf, 256, format, args);
	va_end(args);
	CDC_Transmit_FS((uint8_t*)log_buf, nn);
}

void read_par()
{
	auto white = i2c_read_word(0x14);
	auto red = i2c_read_word(0x16);
	auto green = i2c_read_word(0x18);
	auto blue = i2c_read_word(0x1a);

	log("w:%d r:%d g:%d b:%d\n\r", white, red, green, blue);

	const uint32_t buf_size = 64;
	uint8_t y = 2;
	char buf[buf_size];

	snprintf(buf, buf_size, "W %d        ", (int)white);
	sh1106SmallPrint(0, y++, (uint8_t *)buf);

	snprintf(buf, buf_size, "R %d        ", (int)red);
	sh1106SmallPrint(0, y++, (uint8_t *)buf);

	snprintf(buf, buf_size, "G %d        ", (int)green);
	sh1106SmallPrint(0, y++, (uint8_t *)buf);

	snprintf(buf, buf_size, "B %d        ", (int)blue);
	sh1106SmallPrint(0, y++, (uint8_t *)buf);

	double par = (double)white * (double)0.045067;
	int ipar = par;
	snprintf(buf, buf_size, "%d        ", ipar);
	sh1106MediumPrint(0, y++, (uint8_t *)buf);
}

void main_loop(void)
{
	sh1106Init (40,0x22,1);
	sh1106Clear(0,7);
	sh1106SmallPrint(0, 0, (uint8_t *) "TCS34725 PAR meter 1.0");

	//sh1106MediumPrint(0,1,(uint8_t *) "Hi SH1106");
	//sh1106MediumPrint(0,3,(uint8_t *) "Hello SH1106");

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	HAL_Delay(1000);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_Delay(1000);

	CDC_Transmit_FS((uint8_t*)"TCS34725 PAR meter 1.0\n\r", 39);

	auto chip_id = i2c_read_register(0x12);
	log("chip id:%X\n\r", chip_id);

	i2c_write_register(0x00, 0x03);
	i2c_write_register(0x01, 0xf6);
	i2c_write_register(0x03, 0xf6);
	i2c_write_register(0x0f, 0x00);


	while (true)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		read_par();

		//HAL_Delay(250);

	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		//HAL_Delay(250);

		read_par();
	}
}



