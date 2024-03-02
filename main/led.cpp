#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"

#include "led.hpp"

#define TAG "LED"

const gpio_num_t ledConf[] = { (gpio_num_t)CONFIG_LED1_PIN, (gpio_num_t)CONFIG_LED2_PIN, (gpio_num_t)CONFIG_LED3_PIN, (gpio_num_t)CONFIG_LED4_PIN };

Led::Led() {
    for(int i=0; i<4; i++)
        m_led[i] = false;
}

void Led::init() {
    // LED初期化
    for(int i=0; i<4; i++) {
        gpio_reset_pin(ledConf[i]);
        gpio_set_direction(ledConf[i], GPIO_MODE_OUTPUT);
    }
    ledRef();
}

void Led::setLed(bool led[4]) {
    for(int i=0; i<4; i++)
        m_led[i] = led[i];
    ledRef();
}

void Led::ledRef() {
    for(int i=0; i<4; i++)
        gpio_set_level(ledConf[i], m_led[i] ? 1 : 0);
}