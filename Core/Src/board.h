/*
 * board.h
 *
 *  Created on: Dec 25, 2020
 *      Author: andrey
 */

#pragma once
#include "main.h"
#include "usb_device.h"

#ifdef __cplusplus
 extern "C" {
#endif

void main_loop(void);
void CDC_OnDataReceived(uint8_t* pbuf, uint32_t *Len);

#ifdef __cplusplus
}
#endif
