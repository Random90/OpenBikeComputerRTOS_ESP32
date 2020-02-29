#ifndef _PCD8544_PIN_H_
#define _PCD8544_PIN_H_

#include "driver/spi_common.h"

/**
 * @brief custom configuration for PCD8544 control pins
 *
 *  ESP32-PCD8544 offers automatic configuration for native SPI pins according to SPI host, to allow IOMUX fast and
 *  low latency SPI transfer. But you can specify them manually too. In that case initialize this structure and 
 *  assign to the `spi_pin member of pcd8544_config_t, passed to `pcd8544_init(); `
 *
 *  For more detail, see "Chapter.4 IO_MUX and GPIO Matrix" in the ESP32 Technical Reference Manual.
 *
 * @attention You have to specify IO pins that can be configured as output pin. See ESP32 datasheet for more detail.
 */
typedef struct pcd8544_spi_pin_config_t {
    /**
     * @brief GPIO pin number for SPI MISO pin.
     *J
     *  This pin is actually not used in PCD8544, but have to be specified for compatibility.
     *
     */
    uint8_t miso_io_num;
    /**
     * @brief GPIO pin number for SPI MOSI pin
     *
     *  GPIO pin number connected to the `Din` pin on Nokia 5110 LCD Module.
     *
     */
    uint8_t mosi_io_num;
    /**
     * @brief GPIO pin number for SPI CLK pin 
     *
     *  GPIO pin number connected to the `Clk` pin on Nokia 5110 LCD Module.
     *
     */
    uint8_t sclk_io_num;
    /**
     * @brief GPIO pin number for SPI CS pin
     *
     *  GPIO pin number connected to the `CE` pin on Nokia 5110 LCD Module.
     *
     */
    uint8_t spics_io_num;
} pcd8544_spi_pin_config_t;

/**
 * @brief custom configuration for PCD8544 control pins
 *
 *  ESP32-PCD8544 offers recommended assignments for control pins. But you can specify them manually too.
 *
 * @attention You have to specify IO pins that can be configured as output pin. See ESP32 datasheet for more detail.
 */
typedef struct pcd8544_control_pin_config_t {
    /**
     * @brief GPIO Pin number for Reset control pin
     *
     *  GPIO pin number connected to the `RST` pin on Nokia 5110 LCD Module
     *
     */
    uint8_t reset_io_num;
    /**
     * @brief GPIO Pin number for D/C control pin
     *
     *  GPIO pin number connected to the `DC` pin on Nokia 5110 LCD Module
     *
     */
    uint8_t dc_io_num;
    /**
     * @brief GPIO Pin number for backlight control pin 
     *
     *  GPIO pin number connected to the `BL` pin on Nokia 5110 LCD Module
     *
     */
    uint8_t backlight_io_num;
} pcd8544_control_pin_config_t;

/**
 * @brief Native pin configuration for SPI host. 
 *
 *  In ESP32 Datasheet, native pin configuration that enables IOMUX -- faster and low latency
 *  data transfer method comparing to GPIO matrix -- is recommended. This function returns Native 
 *  SPI pin assignnments for SPI host, that are defined in driver/spi_common.h of ESP-IDF.
 *
 * @param host  SPI host to request native pin configuration
 *
 * @return pcd8544_spi_pin_config_t  Native pin configuraion for SPI host
 */
const pcd8544_spi_pin_config_t* pcd8544_native_spi_pin_config(spi_host_device_t host);

/**
 * @brief (internal use) Get recommended control pin assignments for PCD8544 controller.
 *
 *  Like other SPI display controllers, PCD8544 requires some additional control pins against traditional SPI
 *  -- RST(Reset), BL(Backlight) and DC(Data/Control).
 *  These pins must be able to configures as output pins. Recommended pins (i.e. not reserved for other important 
 *  functionality) are retunred for SPI host.
 *
 * @param host  SPI host to request control pin configuration
 *
 * @return pcd8544_control_pin_config_t  Recommended control pin configuraion for SPI host
 */
const pcd8544_control_pin_config_t* pcd8544_default_control_pin_config(spi_host_device_t host);

#endif /* _PCD8544_PIN_H_ */
