#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <nvs_flash.h>

#include <string.h>
#include <math.h>

#include <driver/gpio.h>

#include <wifi_credentials.h>
#include <app/attitude_controller.h>

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

extern "C" void app_main(void)
{
    int                  level = 0;
    AttitudeController * controller;

    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    
    wifi_config_t sta_config;

    strcpy((char *)sta_config.sta.ssid, WIFI_SSID);
    strcpy((char *)sta_config.sta.password, WIFI_PASSWORD);
    sta_config.sta.bssid_set = false;

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    controller = new AttitudeController(0.01f);

    controller->set_height_target(Controller::Mode::SPEED, 0.0);
    controller->set_roll_target(Controller::Mode::POSITION, 0.0);
    controller->set_pitch_target(Controller::Mode::POSITION, 0.0);
    controller->set_yaw_target(Controller::Mode::SPEED, 0.5);

    while (true)
    {
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
        vTaskDelay(6 / portTICK_PERIOD_MS);
        controller->update();
    }
}

