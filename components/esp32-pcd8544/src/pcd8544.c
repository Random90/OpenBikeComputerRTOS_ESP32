#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "pcd8544.h"
#include "pcd8544_pin.h"
#include "font5x7.h"

static const char *TAG = "PCD8544";

static spi_device_handle_t spi;

static pcd8544_config_t config;

static uint8_t *frame_buf;

static uint8_t trans_queue_size = 0;

static void** data_ptrs;
static uint8_t data_count = 0;

static uint8_t last_col = 0, last_row = 0;

static uint32_t last_warned_at = 0;

static void pcd8544_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user;
    gpio_set_level(config.control_pin->dc_io_num, dc);
}

static void pcd8544_spi_init() {
    if (config.spi_pin == NULL) {
        config.spi_pin = (void *)pcd8544_native_spi_pin_config(config.spi_host);
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = config.spi_pin->mosi_io_num,
        .miso_io_num = config.spi_pin->miso_io_num,
        .sclk_io_num = config.spi_pin->sclk_io_num,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    spi_device_interface_config_t devcfg={
        .clock_speed_hz = 4*1000*1000,
        .mode = 0,
        .spics_io_num = config.spi_pin->spics_io_num,
        .queue_size = PCD8544_TRANS_QUEUE_SIZE,
        .pre_cb = pcd8544_pre_transfer_callback,
    };

    ESP_LOGI(TAG, "SPI pins: Din=IO%d, Clk=IO%d, CE=IO%d",
            config.spi_pin->mosi_io_num, config.spi_pin->sclk_io_num, config.spi_pin->spics_io_num);

    ESP_ERROR_CHECK( spi_bus_initialize(config.spi_host, &buscfg, config.dma_chan) );
    ESP_ERROR_CHECK( spi_bus_add_device(config.spi_host, &devcfg, &spi) );
}

static void pcd8544_reset() {
    gpio_set_level(config.control_pin->reset_io_num, 1);
    gpio_set_level(config.control_pin->reset_io_num, 0);
    gpio_set_level(config.control_pin->reset_io_num, 1);
}

static void* pcd8544_malloc_for_queue_trans(size_t size) {
    return config.dma_chan > 0 ? heap_caps_malloc(size, MALLOC_CAP_DMA) : malloc(size);
}

static void pcd8544_register_buf_for_gc(void* buf) {
    data_ptrs[data_count++] = buf;
}

static void pcd8544_gc_finished_trans_data() {
    while(data_count) {
        if (config.dma_chan > 0) {
            heap_caps_free(data_ptrs[--data_count]);
        } else {
            free(data_ptrs[--data_count]);
        }
    }
}

void pcd8544_init(pcd8544_config_t* pcd8544_config) {
    config = *pcd8544_config;
    if (config.control_pin == NULL) {
        config.control_pin = (void *)pcd8544_default_control_pin_config(config.spi_host);
    }
    gpio_set_direction(config.control_pin->reset_io_num, GPIO_MODE_OUTPUT);
    gpio_set_direction(config.control_pin->dc_io_num, GPIO_MODE_OUTPUT);
    gpio_set_direction(config.control_pin->backlight_io_num, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "Control pins: RST=IO%d, DC=IO%d, BL=IO%d",
            config.control_pin->reset_io_num, config.control_pin->dc_io_num, config.control_pin->backlight_io_num);

    pcd8544_spi_init();
    pcd8544_reset();
    pcd8544_set_backlight(true);

    data_ptrs = pcd8544_malloc_for_queue_trans(PCD8544_TRANS_QUEUE_SIZE * sizeof(void*));

    pcd8544_set_display_mode(PCD8544_DISPLAY_MODE_NORMAL);
    pcd8544_set_contrast(20);

    frame_buf = config.dma_chan > 0 ? heap_caps_malloc(PCD8544_FRAME_BUF_SIZE, MALLOC_CAP_DMA) : malloc(PCD8544_FRAME_BUF_SIZE);
}

static void pcd8544_check_queue_size() {
    if (trans_queue_size == PCD8544_TRANS_QUEUE_SIZE) {
        uint32_t now = esp_log_timestamp();
        if (now - last_warned_at >= PCD8544_WARN_SUPPRESS_INTERVAL_MS) {
            ESP_LOGW(TAG, "trans_queue_size reaches PCD8544_TRANS_QUEUE_SIZE(%d). Force syncing (may get some latency). Consider increasing PCD8544_TRANS_QUEUE_SIZE from 'make menuconfig' for less CPU usage", PCD8544_TRANS_QUEUE_SIZE);
            last_warned_at = now;
        }
        pcd8544_sync_and_gc();
    }
}

static void pcd8544_queue_trans(const uint8_t *cmds_or_data, int len, bool dc) {
    size_t length = 8 * sizeof(uint8_t) * len;
    // On large data without DMA-enabled SPI, We have to queue data for each 32 bytes packet.
    if (config.dma_chan == 0 && length > PCD8544_MAX_TRANS_LENGTH_WITHOUT_DMA) {
        int length_left = length;
        void* p = (void *)cmds_or_data;
        while (length_left) {
            spi_transaction_t *t = malloc(sizeof(spi_transaction_t));
            memset(t, 0, sizeof(spi_transaction_t));
            length = length_left > PCD8544_MAX_TRANS_LENGTH_WITHOUT_DMA
                    ? PCD8544_MAX_TRANS_LENGTH_WITHOUT_DMA : length_left;
            t->length = length;
            t->tx_buffer = p;
            t->user = (void*)(dc ? 1 : 0);
            t->flags = 0;
            pcd8544_check_queue_size();
            ESP_ERROR_CHECK( spi_device_queue_trans(spi, t, portMAX_DELAY) );
            trans_queue_size++;
            p += length / 8;
            length_left -= length;
        }
    } else {
        spi_transaction_t *t = malloc(sizeof(spi_transaction_t));
        memset(t, 0, sizeof(spi_transaction_t));
        t->length = length;
        t->tx_buffer = cmds_or_data;
        t->user = (void*)(dc ? 1 : 0);
        t->flags = 0;
        pcd8544_check_queue_size();
        ESP_ERROR_CHECK( spi_device_queue_trans(spi, t, portMAX_DELAY) );
        trans_queue_size++;
    }
};

static void pcd8544_cmds(const uint8_t *cmds, int len) {
    pcd8544_queue_trans(cmds, len, false);
}

static void pcd8544_data(const uint8_t *data, int len) {
    pcd8544_queue_trans(data, len, true);
}

void pcd8544_set_backlight(bool on) {
    bool flag = config.is_backlight_common_anode ? !on : on;
    gpio_set_level(config.control_pin->backlight_io_num, flag);
}

void pcd8544_set_powerdown_mode(bool powerdown) {
    pcd8544_set_backlight(!powerdown);
    if (powerdown) {
        pcd8544_clear_display();
        pcd8544_finalize_frame_buf();
    }
    uint8_t *cmd = pcd8544_malloc_for_queue_trans(1);
    *cmd = 0b00100000 | powerdown ? (1 << 2) : 0; // power-down mode / chip is active
    pcd8544_cmds(cmd, 1);
}

void pcd8544_set_contrast(uint8_t vop) {
    uint8_t *cmds = pcd8544_malloc_for_queue_trans(5);
    pcd8544_register_buf_for_gc(cmds);
    cmds[0] = 0b00100001; // extended instruction set
    cmds[1] = 0b00000100 | PCD8544_TEMPERATURE_COEFFICIENT; // templerature coefficient
    cmds[2] = 0b00010000 | PCD8544_BIAS_SYSTEM; // bias system
    cmds[3] = 0b10000000 | vop; // Vop (contrast)
    cmds[4] = 0b00100000; // basic instruction set
    pcd8544_cmds(cmds, 5);
}

void pcd8544_set_display_mode(pcd8544_display_mode mode) {
    uint8_t *cmd = pcd8544_malloc_for_queue_trans(1);
    pcd8544_register_buf_for_gc(cmd);
    *cmd = 0b00001000 | ((mode & 2) << 1) | (mode & 1); // display_mode
    pcd8544_cmds(cmd, 1);
}

void pcd8544_clear_display() {
    memset(frame_buf, 0, PCD8544_FRAME_BUF_SIZE);
}

static void pcd8544_set_addressing_mode(pcd8544_addressing_mode mode) {
    uint8_t *cmd = pcd8544_malloc_for_queue_trans(1);
    pcd8544_register_buf_for_gc(cmd);
    *cmd = 0b00100000 | (mode << 1);
    pcd8544_cmds(cmd, 1);
}

void pcd8544_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    float m = (float)(y1-y0)/(x1-x0);
    if (m >= 0 && m < 1) {
        int8_t dx = x1 > x0 ? 1 : -1;
        for (uint8_t x = x0; x != x1; x+=dx) {
            uint8_t y = m * (x - x0) + y0;
            uint8_t y_bank = y / 8;
            uint8_t y_bit = y % 8;
            frame_buf[x * 6 + y_bank] |= 1 << y_bit;
        }
    } else {
        int8_t dy = y1 > y0 ? 1 : -1;
        for (uint8_t y = y0; y != y1; y+=dy) {
            uint8_t x = (1/m) * (y - y0) + x0;
            uint8_t y_bank = y / 8;
            uint8_t y_bit = y % 8;
            frame_buf[x * 6 + y_bank] |= 1 << y_bit;
        }
    }
}

void pcd8544_draw_rectangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    pcd8544_draw_line(x0, y0, x1, y0);
    pcd8544_draw_line(x1, y0, x1, y1);
    pcd8544_draw_line(x1, y1, x0, y1);
    pcd8544_draw_line(x0, y0, x0, y1);
}

void pcd8544_set_pos(uint8_t row, uint8_t col) {
    uint8_t *cmds = pcd8544_malloc_for_queue_trans(2);
    pcd8544_register_buf_for_gc(cmds);
    cmds[0] = 0b10000000 | row,
    cmds[1] = 0b01000000 | col;
    pcd8544_cmds(cmds, 2);
    last_row = row;
    last_col = col;
}

void pcd8544_draw_bitmap(const uint8_t *bitmap, uint8_t rows, uint8_t cols, bool transparent) {
    for (uint8_t col = last_col; col < last_col + cols; col++) {
        for (uint8_t row = last_row; row < last_row + rows; row++) {
            if (transparent) {
                frame_buf[6*row + col] |= bitmap[rows*(col-last_col) + (row-last_row)];
            } else {
                frame_buf[6*row + col] = bitmap[rows*(col-last_col) + (row-last_row)];
            }
        }
    }
}

void pcd8544_finalize_frame_buf() {
    pcd8544_set_addressing_mode(PCD8544_ADDRESSING_MODE_VERTICAL);
    pcd8544_set_pos(0, 0);
    pcd8544_data(frame_buf, PCD8544_FRAME_BUF_SIZE);
    pcd8544_set_addressing_mode(PCD8544_ADDRESSING_MODE_HORIZONTAL);
}


void pcd8544_puts(const char *text) {
    uint8_t text_len = strlen(text);
    uint8_t char_width = sizeof(font5x7[0]) + 1;
    uint16_t data_len = char_width * text_len;
    uint8_t *data = pcd8544_malloc_for_queue_trans(data_len);
    pcd8544_register_buf_for_gc(data);
    for (uint8_t i = 0; i < text_len; i++) {
        for (uint8_t j = 0; j < char_width-1; j++) {
            data[i * char_width + j] = font5x7[text[i]-FONT5X7_CHAR_CODE_OFFSET][j];
        }
	data[i * char_width + char_width-1] = 0;
    }
    pcd8544_data(data, data_len);
}

void pcd8544_printf(const char *format, ...) {
    char s[64];
    va_list ap;
    va_start(ap, format);
    vsprintf(s, format, ap);
    va_end(ap);
    pcd8544_puts(s);
}

void pcd8544_sync_and_gc() {
    spi_transaction_t *rtrans;
    while(trans_queue_size) {
        ESP_ERROR_CHECK( spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY) );
        free(rtrans);
        trans_queue_size--;
    }
    pcd8544_gc_finished_trans_data();
}

void pcd8544_free() {
    spi_bus_remove_device(spi);
    spi_bus_free(config.spi_host);
    heap_caps_free(frame_buf);
    heap_caps_free(data_ptrs);
}
