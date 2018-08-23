/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __GPIO_HANDLER_H__
#define __GPIO_HANDLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_adc_cal.h"

#define GPIO_OUTPUT_IO_0	4
#define GPIO_RED			16
#define GPIO_GREEN			17
#define GPIO_BLUE			18
#define GPIO_OUTPUT_PIN_SEL	((1ULL<<GPIO_RED) | (1ULL<<GPIO_GREEN) | \
                          (1ULL<<GPIO_BLUE) | (1ULL<<GPIO_OUTPUT_IO_0) )

#define GPIO_BUTTON_0		19
#define GPIO_BUTTON_1		21
#define GPIO_INPUT_PIN_SEL	((1ULL<<GPIO_BUTTON_0) | \
                             (1ULL<<GPIO_BUTTON_1))

#define ESP_INTR_FLAG_DEFAULT 0

#define ADC1_EXAMPLE_CHANNEL ADC1_CHANNEL_7

bool gpio_get_yesno();
void gpio_handler_init();
void gpio_idle_blink_task();
void gpio_white_on();
void gpio_white_off();
void gpio_blink_white();
void gpio_blink_one(int gpio_num);
int gpio_sample_adc();
void short_beep();

#endif
