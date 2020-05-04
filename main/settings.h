#ifndef SETTINGS
#define SETTINGS

// tire circuference in milimeters
#define CIRCUMFERENCE 2012
/* Time after which OBC enters 'not riding' mode*/
#define RIDE_TIMEOUT_MS 2500
// constant that affects how quickly the average "catches up" to the latest trend. Smaller the number the faster
#define AVG_FACTOR 100

#define ESP_INTR_FLAG_DEFAULT 0
#define BLINK_GPIO 2
#define REED_IO_NUM 18

#endif


