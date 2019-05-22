#include "pcd8544.h"

static const pcd8544_spi_pin_config_t spi_native_pin[3] = {
    {
        .sclk_io_num = 6,
        .miso_io_num = 8,
        .mosi_io_num = 7,
        .spics_io_num = 11,
    }, {
        .sclk_io_num = 14,
        .miso_io_num = 13,
        .mosi_io_num = 12,
        .spics_io_num = 15,
    }, {
        .sclk_io_num = 18,
        .miso_io_num = 23,
        .mosi_io_num = 19,
        .spics_io_num = 5,
    }
};

static const pcd8544_control_pin_config_t pcd8544_default_control_pin = {
    .reset_io_num = 4,
    .dc_io_num = 25,
    .backlight_io_num = 16,
};

const pcd8544_spi_pin_config_t* pcd8544_native_spi_pin_config(spi_host_device_t host) {
    return &spi_native_pin[host];
}

const pcd8544_control_pin_config_t* pcd8544_default_control_pin_config(spi_host_device_t host) {
    return &pcd8544_default_control_pin;
}
