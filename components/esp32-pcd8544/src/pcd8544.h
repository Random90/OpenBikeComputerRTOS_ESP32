#ifndef _PCD8544_H_
#define _PCD8544_H_

#include "driver/spi_common.h"
#include "pcd8544_pin.h"
#include "sdkconfig.h"

/**
 * @brief Internal frame buffer size
 */
#define PCD8544_FRAME_BUF_SIZE 504 // 84 rows * 6 cols
/**
 * @brief SPI Tranfer queue size for ESP32-PCD8544
 */
#define PCD8544_TRANS_QUEUE_SIZE 64
/**
 * @brief Bias System parameter for PCD8544
 *
 * It affects results of the contrast setting via `pcd8544_set_contrast(uint8_t vop);`
 *
 * You may configure this parameter via `make menuconfig` if you have contrast range problem.
 *
 */
#define PCD8544_BIAS_SYSTEM 7
/**
 * @brief Temperature coefficient parameter for PCD8544.
 *
 * It affects results of the contrast setting via `pcd8544_set_contrast(uint8_t vop);`, in low tempearature
 * environment.
 *
 * You may configure this parmeter via `make menuconfig` if you have contrast range problem.
 *
 */
#define PCD8544_TEMPERATURE_COEFFICIENT 2

/**
 * @brief Maximum buffer size (in bits) for non-DMA SPI transfer.
 *
 *  If you initialized pcd8544_init(config) with `config.dma_chan = 0`, SPI data transfer via DMA is disabled.
 *  In that case, Maximum transfer data size is limited to this size in bits.
 *  ESP32-PCD8544 decompose large size of data transfer requrest into multiple 256 bits of request to fit
 *  this limitation.
 *
 *  @attention Full frame buffer transfer cause 84x6=504 bytes = 4032 bits of request. In non-DMA environment,
 *             the request is decomposed into each 256 bits i.e. 16 data transfer request occurs.
 *             YOu may want to increase PCD8544_TRANS_QUEUE_SIZE from kconfig.
 */
#define PCD8544_MAX_TRANS_LENGTH_WITHOUT_DMA 256 // 32 bytes * 8 bits = 256 bits

/**
 * @brief Interval for surpressing repeatedly displaying warnings in milliseconds.
 *
 *  To surpress queue size related warning, certain interval in milliseconds is used.
 *
 */
#define PCD8544_WARN_SUPPRESS_INTERVAL_MS 5000

typedef enum {
    /**
     * @brief Make DDRAM address pointer to move from (row,col) like (0,0), (1,0), (2,0) ... (83,0), (0,1), (1,1)
     *        order for each data transfer.
     */
    PCD8544_ADDRESSING_MODE_HORIZONTAL,
    /**
     * @brief Make DDRAM address pointer to move from (row,col) like (0,0), (0,1), (0,2) ... (0,5), (1,0), (1,1)
     *        order for each data transfer.
     */
    PCD8544_ADDRESSING_MODE_VERTICAL
} pcd8544_addressing_mode;

typedef enum {
    /**
     * @brief Make LCD content to blank
     */
    PCD8544_DISPLAY_MODE_BLANK = 0b00,
    /**
     * @brief Make all LCD segments on (for pin connection check)
     */
    PCD8544_DISPLAY_MODE_ALL_SEGMENTS_ON = 0b01,
    /**
     * @brief Make LCD segments correspond to DDRAM data
     */
    PCD8544_DISPLAY_MODE_NORMAL = 0b10,
    /**
     * @brief Make LCD segments opposite to DDRAM data
     */
    PCD8544_DISPLAY_MODE_INVERSE = 0b11,
} pcd8544_display_mode;

/**
 * @brief Confirugration for PCD8544
 *
 *  Configurations for PCD8544 and Nokida 5110 LCD board.
 *
 */
typedef struct pcd8544_config_t {
    /**
     * @brief configure installed PCD8544 board PCBs LED are common-anode or common-cathode
     *
     * There are two type of PCB is selling store for Nokia5100 LCD module. They can be determained from its color;
     * red one and green one.
     *
     * Red one comes with common-anode LED so it this member should be `true`.
     *
     * Blue one comes with common-cathode LED so it this member should be `false`.
     */
    bool is_backlight_common_anode;
    /**
     * @brief SPI host to be used for PCD8544.
     *
     * You can choose from ESP32' logical 3 SPI channels `SPI_HOST`, `HSPI_HOST` and `VSPI_HOST`.
     *
     * @attention Differnt SPI host have different native pin assignemnt for fast and low latency IO_MUX.
     * So ESP23-PCD8544 offers different pin assignment according to `.spi_host` member.  You can confirm
     * which pins are assigned via UART serial output log in INFO lebel.
     *
     * @attention If you want to use DMA channel for low latency and less flicker, choose HSPI_HOST or VSPI_HOST
     *            that is currently available for DMA transfer.
     */
    spi_host_device_t spi_host;
    /**
     * @brief DMA channel to use (1 or 2) or no use DMA channel (0).
     *
     * ESP32 offers two DMA channel; 1 and 2. You can optionally use a DMA channel for less CPU load, low latency
     * and high throughput SPI transfer. If you already using DMA channels for other use (like I2S-connected speaker,
     * mic or image sensor), you can specify `.dma_chan = 0` to tell ESP32-PCD8544 not to use any DMA channel.
     */
    uint8_t dma_chan;
    /**
     * @brief Manual SPI pin assignment configuration.
     *
     * Althrough native SPI pin assignment for SPI host is recommended as it enables low latency SPI transfer by IOMMU,
     * you can still specify any IO pin numbers for each SPI pins  one by one, via setting pointer to `*spi_pin`
     * member.
     *
     * @attention All specified GPIO pins must be able to configured output pins. (see "2.2 Pin Description" in
     *            the ESP32 Datasheet)
     *
     * @attention Keeping this member `NULL` allows ESP32-PCD8544 to auto-configure native SPI pins corresponding to
     *            selected SPI host. You can confirm which pins are assigned to control pins from  UART serial output
     *            log (In INFO log level).
     *
     */
    pcd8544_spi_pin_config_t *spi_pin;
    /**
     * @brief Manual control pin assignment configuration.
     *
     * To drive PCD8544, it requires some additoinal control pins from tradisonal SPI pins; `DC`(Data/Command) pin,
     * `RST`(Reset) pin and `BL`(Backlight) pin. You can specify them manually alternative to automatic configuration
     * by ESP32-PCD8544.
     *
     * @attention All specified IO pins should be able to configured output pins.
     *
     * @attention Keeping this member `NULL` allows auto configuration of recommended control pin assignment by
     *            ESP32-PCD8544. You can confirm which pins are assigned to control pins from  UART serial output log
     *            (In INFO log lavel).
     */
    pcd8544_control_pin_config_t *control_pin;
} pcd8544_config_t;

/**
 * @brief Initialize PCD8544 device
 *
 *  Initialize PCD8544 as SPI slave. You can make some operations via APIs offered from ESP32-PCD8544
 *  until pcd8544_free() is called.
 *
 * @param config Configuration of PCD8544 SPI
 */
void pcd8544_init(pcd8544_config_t *config);

/**
 * @brief Set LCD backlight status
 *
 *  Set LCD backlight enabled / disabled.
 *
 * @param on  boolean parameter to activate or deactivate LED for LCD.
 *
 * @attention To use this function correctly you have to initialize `.is_backlight_common_anode` member in
 *            pcd8544_config_t according to the model of your Nokia 5100 LCD Module (red one or blue one,
 *            or you will get opposite result.
 */
void pcd8544_set_backlight(bool on);

/**
 * @brief Control power-down mode / chip-active mode status of PCD8544.
 *
 *  PCD8544 provides `power-down` mode that consume only very low current on inactive status.
 *  This function control its status. In power-down mode, only 1.5uA current is consumed based on datasheet.
 *
 * @param powerdown  boolean parameter to activate or deactivate power-down mode.
 *
 * @attention On entering  power-down mode, backlight goes off for to reduce current. Similary, when back to
 *              chip-active mode backlight goes on.
 *
 * @attention To enter power-down mode correctly, content of frame buffer and LCD DRAM is cleared before
 *            entering power-down mode. Thus you have to recover LCD contents by yourself after back to
 *            chip-active mode.
 */
void pcd8544_set_powerdown_mode(bool powerdown);

/**
 * @brief Control display mode of the LCD
 *
 *  PCD8544 provides four display modes. `normal`, `all segments on`, `inverted` and `blank`. This function
 *  controls PCD8544 between modes which defined on pcd8544_display_mode.
 *
 *  `normal` mode, DDRAM (Display Data RAM) as-is applied to LCD content.
 *
 *  `inverted` mode makes oppsite on / off state of LCD segments.
 *
 *  In `all segments on` mode, you can confirm  electrical connection by setting and very high contrast seting.
 *
 *  `blank` mode make make all segments off.
 *
 * @param mode  pcd8544_display_mode value for entering other display mode.
 *
 * @attention  After pcd8544_init() called, display mode goes `normal`.
 *
 * @attention  `blank` mode does not erase LCD contents. It is hold in DDRAM (Display Data RAM) and still and can be
 *             recovered by switching to other modes.
 *
 */
void pcd8544_set_display_mode(pcd8544_display_mode mode);

/**
 * @brief Control contrast of the LCD
 *
 *  PCD8544 have contrast control functionality. You can access this by setting contrast value. Optimal parameter
 *  may different from each individuals. pcd8544_init() resets contrast to empilically good default but you can
 *  tune via this function.
 *
 * @param vop  Operation voltage parameter for LCD (0~127). In default configuration options, range between 20 to 24
 *             will be good choice.
 *
 * @attention  pcd8544_init() calls this function internally with empilically good default (vop=20) so you don't
 *             need to tune contrast manually. You can still tune contrast against different indivisual
 *             characteristics.
 */
void pcd8544_set_contrast(uint8_t vop);

/**
 * @brief Clear all LCD content.
 *
 *  This function content in frame buffer of LCD.
 *
 * @attention  To confirm result on the LCD, you have to call pcd8544_finalize_frame_buf() after
 * frame buffer writing operation is finished.
 *
 */
void pcd8544_clear_display();

/**
 * @brief Draw line to the LCD.
 *
 *  This function draw an line on the frame buffer. You can specify start and end points in geometory.
 *
 * @param x0  x of the start point.
 *
 * @param y0  y of the start point.
 *
 * @param x1  x of the end point.
 *
 * @param y1  y of the end point.
 *
 * @attention  To confirm result on the LCD, you have to call pcd8544_finalize_frame_buf() after
 * frame buffer writing operation is finished.
 */
void pcd8544_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/**
 * @brief Draw rectrangle to the LCD.
 *
 *  This function draw rectangle on the frame buffer. You can specify upper-left and bottom-right points in geometory.
 *
 * @param x0  x of the upper-left corner.
 *
 * @param y0  y of the upper-left corner.
 *
 * @param x1  x of the bottom-right corner.
 *
 * @param y1  y of the bottom-right corner.
 *
 * @attention  To write line data to the DDRAM and update segments on the LCD, you have to call
 * pcd8544_finalize_frame_buf() after all frame buffer writing operation is finished.
 */
void pcd8544_draw_rectangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/**
 * @brief Set (row, col) position operation start position.
 *
 *  This function set start position of the next operation.
 *
 * @param row  start row of the next operation (0-83)
 *
 * @param col  start column of the next operation (0-6)
 *
 * @attention Unlike draw_line() and draw rectangle() accepts (x,y) geometory as arguments, pcd8544_set_pos()
 * accepts row(0-83) and column(0-6) dimentions. Thus, you can spcify only per 8 segments for vertical direction.
 */
void pcd8544_set_pos(uint8_t row, uint8_t col);

/**
 * @brief Draw arbitrary bitmap to the frame buffer
 *
 * @param *bitmap  pointer to a uint8_t buffer contains the bitmap data.
 *
 * @param rows  number of rows of the bitmap data.
 *
 * @param cols  number of cols of the bitmap data.
 *
 * @param transparent  boolean value for whether operate '0' bits as transparent or not.
 *
 * @attention  `*bitmap` buffer should be sized (rows * cols) bytes. and decomposed per `rows` bytes for drawing
 *              each columns.
 *
 * @attention  the Top-left corner position of the bitmap is determined by last pcd8544_set_pos() arguments..
 *
 * @attention  To write line data to the DDRAM and update segments on the LCD, you have to call
 * pcd8544_finalize_frame_buf() after all frame buffer writing operation is finished.
 */
void pcd8544_draw_bitmap(const uint8_t *bitmap, uint8_t rows, uint8_t cols, bool transparent);

/**
 * @brief Finish frame buffer operations of current frame and write to DDRAM of the LCD.
 *
 * To avoid redundant data transfer via SPI bus, functions `pcd8544_clear_display()` `pcd8544_draw_line()`,
 * pcd8544_draw_rectangle() and `pcd8544_draw_bitmaap()` theirselves does not transfer to the DDRAM.
 * Alternatively, `pcd8544_finalize_frame_buf()` does this operation to apply frame buffer state to the LCD at once.
 *
 * For each frame, you have to call this function after above functions are called.
 */
void pcd8544_finalize_frame_buf();

/**
 * @brief Print text to the LCD.
 *
 * Like standard `puts()` function, you can write arbitrary text to the LCD via this function.
 * Position of the Text is determined from last pcd8544_set_pos() call.
 *
 * As this library includes 8x5 dots font, you can write up to 14 characters per row.
 *
 * @param text  `const char*` text to print.
 *
 * @attention If writing position of the text reached right-end of the LCD, writing position is automatically
 * moves to the next column. When it was the bottom column (column 6), the next column is column 1.
 */
void pcd8544_puts(const char* text);

/**
 * @brief Print formatted text to the LCD.
 *
 * Like standard `printf()` function, you can write arbitrary text with format.
 * Position of the Text is determined from last pcd8544_set_pos() call.
 *
 * Format string is full compatible with standard `printf()` as this function uses `spritntf()` internally.
 *
 * @param *format  format string to print.
 *
 * @param ...  arbitrary arguments for format string.
 *
 * @attention If writing position of the text reached right-end of the LCD, writing position is automatically.
 * moves to the next column. When it was the bottom column (column 6), the next column is column 1.
 */
void pcd8544_printf(const char *format, ...);

/**
 * @brief Sync background SPI transfer and free memory of buffers used for transfer.
 *
 * ESP32-PCD8544 driver offers asyncronous SPI tranfer to allow users to other operations during SPI transaction.
 * To achieve this, some internal data should be dynamically allocated and have to wait until SPI transaction finished.
 * This function waits all SPI transfer queue finisshed, and free dynamically allocated buffers for them.
 *
 * @attention Developer must call this function in the last of the frame operations, or it cause memory leak.
 */
void pcd8544_sync_and_gc();

/**
 * @brief Free SPI related resource completely.
 *
 * ESP32-PCD8544 utilize single SPI host and optionally DMA channel of ESP32 to drive PCD8544.
 * Developer may want to re-use SPI host after some content has been written to the LCD. In that case,
 * SPI device and bus occupied by `pcd8544_init()` can be freed with this function.
 *
 * @attention Once `pcd8544_free()` is called, you have to call `pcd8544_init()` again to later use.
 */
void pcd8544_free();

#endif /* _PCD8544_H_ */
