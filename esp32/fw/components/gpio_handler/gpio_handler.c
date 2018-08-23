#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "gpio_handler.h"

//static TaskHandle_t thandle_1;
static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t) arg;
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

bool gpio_get_yesno()
{
	//TODO 2min timeout
	uint32_t io_num;
	for (;;) {
		if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			//printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
			if (io_num == GPIO_BUTTON_0) {
				//printf("yesbutton\n");
				return true;
			} else if (io_num == GPIO_BUTTON_1) {
				//printf("nobutton\n");
				return false;
			} else {
				//printf("unknown button?\n");
			}
		}
	}
}

/*
void gpio_idle_blink_task()
{
	for (;;) {
		gpio_blink_one(GPIO_BLUE);
		vTaskDelay(100 / portTICK_RATE_MS);
		gpio_blink_one(GPIO_BLUE);

		vTaskDelay(10000 / portTICK_RATE_MS);
	}
}
*/

void short_beep()
{
	gpio_set_level(GPIO_OUTPUT_IO_0, 1);
	vTaskDelay(300 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_IO_0, 0);
}

void gpio_white_on()
{
	gpio_set_level(GPIO_RED, 0);
	gpio_set_level(GPIO_GREEN, 0);
	gpio_set_level(GPIO_BLUE, 0);
}

void gpio_white_off()
{
	gpio_set_level(GPIO_RED, 1);
	gpio_set_level(GPIO_GREEN, 1);
	gpio_set_level(GPIO_BLUE, 1);
}

void gpio_blink_white()
{
	gpio_set_level(GPIO_RED, 0);
	gpio_set_level(GPIO_GREEN, 0);
	gpio_set_level(GPIO_BLUE, 0);
	vTaskDelay(250 / portTICK_RATE_MS);
	gpio_set_level(GPIO_RED, 1);
	gpio_set_level(GPIO_GREEN, 1);
	gpio_set_level(GPIO_BLUE, 1);
	vTaskDelay(250 / portTICK_RATE_MS);
}

void gpio_blink_one(int gpio_num)
{
	gpio_set_level(gpio_num, 0);
	vTaskDelay(250 / portTICK_RATE_MS);
	gpio_set_level(gpio_num, 1);
	vTaskDelay(250 / portTICK_RATE_MS);
}

int gpio_sample_adc()
{
	int num_samples = 1000;
	int i = num_samples;
	double avg = 0.0;
	while (i > 0) {
		avg += adc1_get_raw(ADC1_EXAMPLE_CHANNEL);
		i--;
	}
	return (avg / num_samples);
}

void gpio_handler_init()
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	//start gpio task
	//xTaskCreate(gpio_task_handle, "gpio_task_handle", 2048, NULL, 10, NULL);

	//install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_BUTTON_0, gpio_isr_handler,
			     (void *)GPIO_BUTTON_0);
	//hook isr handler for specific gpio pin
	gpio_isr_handler_add(GPIO_BUTTON_1, gpio_isr_handler,
			     (void *)GPIO_BUTTON_1);

	gpio_set_level(GPIO_OUTPUT_IO_0, 0);
	gpio_set_level(GPIO_RED, 1);
	gpio_set_level(GPIO_GREEN, 1);
	gpio_set_level(GPIO_BLUE, 1);

	vTaskDelay(1500 / portTICK_RATE_MS);
	gpio_blink_one(GPIO_RED);
	gpio_blink_one(GPIO_GREEN);
	gpio_blink_one(GPIO_BLUE);

	gpio_blink_one(GPIO_RED);
	gpio_blink_one(GPIO_GREEN);
	gpio_blink_one(GPIO_BLUE);
	//xTaskCreate(gpio_idle_blink_task, "gpio_idle_blink_task",
	//	    10000, NULL, 1, &thandle_1);

	adc1_config_channel_atten(ADC1_EXAMPLE_CHANNEL, ADC_ATTEN_0db);
	vTaskDelay(2 * portTICK_PERIOD_MS);

	return;
}
