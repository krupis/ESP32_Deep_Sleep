/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "nvs_flash.h"
#include "nvs.h"

#define GPIO_OUTPUT_BUILTIN_LED 2

#define GPIO_BUILTIN_LED_PIN_SEL   ((1ULL<<GPIO_OUTPUT_BUILTIN_LED) )



static void configure_gpio();
static void get_wake_up_reason();


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Open NVS done\n");
    }

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }


    const int wakeup_time_sec = 20;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
  
    get_wake_up_reason();
    printf("Going to sleep in 5 second \n");


  // Turn off and keep off the built-in led during deep sleep
    configure_gpio();
    gpio_set_level(GPIO_OUTPUT_BUILTIN_LED, 0);
    gpio_hold_en(GPIO_OUTPUT_BUILTIN_LED);
    gpio_deep_sleep_hold_en();




    vTaskDelay(5000/portTICK_PERIOD_MS);
    esp_deep_sleep_start();


}



static void configure_gpio(){
    gpio_config_t io_conf;

// OUTPUTS
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_BUILTIN_LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}


static void get_wake_up_reason(){
    uint8_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0: {
            printf("Wakeup caused by external signal using RTC_IO \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_EXT1 : {
            printf("Wakeup caused by external signal using RTC_CNTL \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER : {
            printf("Wakeup caused by timer \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_TOUCHPAD : {
            printf("Wakeup caused by touchpad \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_ULP : {
            printf("Wakeup caused by ULP program \n"); 
            break;
        }
        default : {
            printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
            break;
        }
    }
}