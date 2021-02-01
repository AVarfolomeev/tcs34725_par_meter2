/*
 * board.cpp
 *
 *  Created on: Dec 25, 2020
 *      Author: andrey
 */
#include <ctype.h>
#include <stdarg.h>
#include "board.h"
#include "usbd_cdc_if.h"
#include "spi1106.h"
#include "settings.h"

extern I2C_HandleTypeDef hi2c1;
extern USBD_HandleTypeDef hUsbDeviceFS;

static volatile bool i2c_tx_completed = false;
static volatile bool i2c_rx_completed = false;

const uint32_t flash_settings_page = 0x0800F7C2;
static settings current_settings;

const auto app_title = "TCS34725 PAR meter 1.1";

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

const uint32_t log_buf_size = 512;
char log_buf[log_buf_size];

void log(const char* format, ...)
{
	auto hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
	if (hcdc == nullptr)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	int nn = vsnprintf(log_buf, log_buf_size, format, args);
	va_end(args);

	while (hcdc->TxState != 0)
	{
		HAL_Delay(1);
	}
	CDC_Transmit_FS((uint8_t*)log_buf, nn);
}

void read_par()
{
	auto white = i2c_read_word(0x14);
	auto red = i2c_read_word(0x16);
	auto green = i2c_read_word(0x18);
	auto blue = i2c_read_word(0x1a);

	//log("w:%d r:%d g:%d b:%d\n\r", white, red, green, blue);

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

	double par = current_settings.m_total_factor * current_settings.calc_normalized_from_raw(white, red, green, blue);
	//double par = (double)white * (double)0.045067;
	int ipar = par;
	snprintf(buf, buf_size, "%d        ", ipar);
	sh1106MediumPrint(0, y++, (uint8_t *)buf);
}

void send_status()
{
	log("\n\r\n\r%s build %s %s\n\r", app_title, __DATE__, __TIME__);

	auto chip_id = i2c_read_register(0x12);
	log("chip id:%X\n\r", chip_id);

	log("Integration time %x %lfms\n\r", current_settings.m_integration_time, current_settings.get_integration_time());
	log("Wait time %x %lfms\n\r", current_settings.m_wait_time, current_settings.get_wait_time());
	log("Gain %x x%lf\n\r", current_settings.m_gain, current_settings.get_gain());
	log("PAR = K[%lf] * W[%lf] * R[%lf] * G[%lf] * B[%lf]\n\r",
			current_settings.m_total_factor,
			current_settings.m_white,
			current_settings.m_red,
			current_settings.m_green,
			current_settings.m_blue);
	log("Usage:\n\r");
	log("k <value> - set K factor\n\r");
	log("w <value> - set W factor, 0..1\n\r");
	log("r <value> - set R factor, 0..1\n\r");
	log("g <value> - set G factor, 0..1\n\r");
	log("b <value> - set B factor, 0..1\n\r");
	log("int <value> - set integration time, 0..255\n\r");
	log("wait <value> - set wait time, 0..255\n\r");
	log("gain <value> - set gain, 0..3\n\r");
	log("def - reset to defaults\n\r");
	log("flash - write changes to flash\n\r");
	log("par <par value> <raw w> <raw r> <raw g> <raw b>\n\r");
	log("    - calculate K factor from measured PAR\n\r");
}

const uint32_t cmd_buf_size = 512;
char cmd_buf[cmd_buf_size];
char current_cmd_buf[cmd_buf_size];
uint32_t cmd_input_size = 0;
char* current_cmd = nullptr;

void process_cdc_input(char c)
{
	c = tolower(c);

	if (c == '\r')
	{
		cmd_buf[cmd_input_size] = 0;

		memcpy(current_cmd_buf, cmd_buf, cmd_input_size + 1);
		current_cmd = current_cmd_buf;

		cmd_input_size = 0;
	}
	else if (c == '\b')
	{
		if (cmd_input_size > 0) cmd_input_size--;
	}
	else
	{
		cmd_buf[cmd_input_size] = c;
		cmd_input_size++;
	}

	if (cmd_input_size >= cmd_buf_size)
	{
		cmd_input_size = 0;
	}
}

void CDC_OnDataReceived(uint8_t* pbuf, uint32_t *Len)
{
	CDC_Transmit_FS(pbuf, *Len);

	for (uint32_t i = 0; i < *Len; i++)
	{
		process_cdc_input((char)pbuf[i]);
	}
}

bool starts(const char* buf, const char* probe)
{
	auto probe_len = strlen(probe);
	return strncmp(buf, probe, probe_len) == 0;
}

bool factor_cmd(const char* buf, const char* probe, double* factor)
{
	auto probe_len = strlen(probe);

	if (strncmp(buf, probe, probe_len) != 0)
	{
		return false;
	}

	double k = 0;
	if (sscanf(buf + probe_len, "%lf", &k) < 1)
	{
		log("\n\rerror\n\r");
		send_status();
		return true;
	}
	*factor = k;
	log("\n\rok\n\r");
	return true;
}

void init_tcs()
{
	i2c_write_register(0x00, 0x03);
	i2c_write_register(0x01, current_settings.m_integration_time);
	i2c_write_register(0x03, current_settings.m_wait_time);
	i2c_write_register(0x0f, current_settings.m_gain);
}

bool reg_cmd(const char* buf, const char* probe, uint8_t* reg)
{
	auto probe_len = strlen(probe);

	if (strncmp(buf, probe, probe_len) != 0)
	{
		return false;
	}

	unsigned int x = 0;
	if (sscanf(buf + probe_len, "%x", &x) < 1)
	{
		log("\n\rerror\n\r");
		send_status();
		return true;
	}
	*reg = (uint8_t)x;
	init_tcs();
	log("\n\rok\n\r");
	return true;
}

bool flash_current_settings()
{
	current_settings.make_valid();

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase_init;
	erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
	erase_init.Banks = 0;
	erase_init.PageAddress = flash_settings_page;
	erase_init.NbPages = 1;
	uint32_t error = 0;
	if (HAL_FLASHEx_Erase(&erase_init, &error) != HAL_OK)
	{
		while (HAL_FLASH_Lock() != HAL_OK) ;
		return false;
	}

	FLASH_WaitForLastOperation(500);

	auto data = reinterpret_cast<uint32_t*>(&current_settings);
	uint32_t flash_addr = flash_settings_page;
	uint32_t i = 0;
	while (i < sizeof(settings))
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_addr, *data) != HAL_OK)
		{
			while (HAL_FLASH_Lock() != HAL_OK) ;
			return false;
		}
		data++;
		flash_addr += 4;
		i += 4;
	}
	HAL_FLASH_Lock();
	return true;
}

void try_exec_received_command()
{
	if (current_cmd == nullptr)
	{
		return;
	}

	auto cmd = current_cmd;
	current_cmd = nullptr;

	if (factor_cmd(cmd, "k ", &current_settings.m_total_factor)) return;
	if (factor_cmd(cmd, "w ", &current_settings.m_white)) return;
	if (factor_cmd(cmd, "r ", &current_settings.m_red)) return;
	if (factor_cmd(cmd, "g ", &current_settings.m_green)) return;
	if (factor_cmd(cmd, "b ", &current_settings.m_blue)) return;
	if (reg_cmd(cmd, "int ", &current_settings.m_integration_time)) return;
	if (reg_cmd(cmd, "wait ", &current_settings.m_wait_time)) return;
	if (reg_cmd(cmd, "gain ", &current_settings.m_gain)) return;

	if (starts(cmd, "def"))
	{
		current_settings.set_default();
		init_tcs();
		log("\n\rok\n\r");
		return;
	}

	if (starts(cmd, "flash"))
	{
		if (!flash_current_settings())
		{
			log("\n\rerror\n\r");
		}
		else
		{
			auto fl_settings = reinterpret_cast<settings*>(flash_settings_page);
			if (fl_settings->is_valid())
			{
				log("\n\rok\n\r");
			}
			else
			{
				log("\n\rerror\n\r");
			}
		}
		return;
	}

	if (starts(cmd, "par "))
	{
		double par = 0;
		double w = 0;
		double r = 0;
		double g = 0;
		double b = 0;
		if (sscanf(cmd + 4, "%lf %lf %lf %lf %lf", &par, &w, &r, &g, &b) < 5)
		{
			log("\n\rerror\n\r");
			send_status();
			return;
		}

		auto raw = current_settings.calc_normalized_from_raw((uint32_t)w, (uint32_t)r, (uint32_t)g, (uint32_t)b);
		if (raw == 0)
		{
			log("\n\rerror: bad raw values\n\r");
			return;
		}

		current_settings.m_total_factor = par / raw;

		log("\n\rok\n\r");
		return;
	}

	send_status();
}

void main_loop(void)
{
	current_settings.set_default();

	auto fl_settings = reinterpret_cast<settings*>(flash_settings_page);
	if (fl_settings->is_valid())
	{
		memcpy(&current_settings, fl_settings, sizeof(settings));
	}

	sh1106Init (40,0x22,1);
	sh1106Clear(0,7);
	sh1106SmallPrint(0, 0, (uint8_t*)app_title);

	init_tcs();

	while (true)
	{
		try_exec_received_command();

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		read_par();

		//HAL_Delay(250);

	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		//HAL_Delay(250);

		read_par();
	}
}



