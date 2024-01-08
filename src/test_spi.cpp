#include "esp_crc.h"
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EXT_SPI_SS     -1 //or 11?
static const char *TAG = "ext_test";

spi_device_handle_t spi;

void init_spi() {
    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t dev_cfg = {
        
        .mode = 0,
        .clock_speed_hz = 1000000, 
        .spics_io_num = -1, 
        .queue_size = 8
    };

    dev_cfg.clock_source = spi_clock_source_t::SPI_CLK_SRC_DEFAULT;

        gpio_config_t config = {
        .pin_bit_mask = (1UL << EXT_SPI_SS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = gpio_pullup_t::GPIO_PULLUP_DISABLE,
        .pull_down_en = gpio_pulldown_t::GPIO_PULLDOWN_DISABLE,
        .intr_type = gpio_int_type_t::GPIO_INTR_DISABLE,
    };

    esp_err_t result;
    result = gpio_config(&config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Error during SSn gpio configuration!");
    }

    ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, 1);
    assert(ret == ESP_OK);

    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi);
    assert(ret == ESP_OK);
}

esp_err_t ext_deinit(){
    esp_err_t result;
    
    result = spi_bus_remove_device(spi);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Error during SPI device clean up...!");
        return result;
    }

    result = spi_bus_free(VSPI_HOST);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Error during SPI bus clean up...!");
        return result;
    }

    ESP_LOGI(TAG, "Extension interface set down...");

    return ESP_OK;
}


uint8_t spi_command(uint8_t addr, uint8_t instr, uint8_t reg) {

    uint8_t tx_data[] = {0x01, 0x11, 0x00, 0x29};
    uint8_t rx_data[8];
    uint8_t chk_data[8];

    while (1) {

        spi_transaction_t t;

        memset(&t, 0, sizeof(t));

        t.length = 32; 
        t.tx_buffer = tx_data;
        t.rx_buffer = rx_data;

        gpio_set_level(static_cast<gpio_num_t>(EXT_SPI_SS), 0);

        esp_err_t ret = spi_device_transmit(spi, &t);
        
        assert(ret == ESP_OK);

        printf("Sent: ");
        for (int i = 0; i < 4; i++) {
            printf("%02X ", tx_data[i]);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "\nReceived: ");

        for (int i = 0; i < 8; i++) {
            ESP_LOGI(TAG, "Received data: %02X ", rx_data[i]);
            chk_data[i] = rx_data[i];
        }

        printf("\n");
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(static_cast<gpio_num_t>(EXT_SPI_SS), 1);

    }
}
