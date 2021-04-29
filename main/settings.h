#ifndef SETTINGS
#define SETTINGS

// tire circuference in milimeters
#define CIRCUMFERENCE CONFIG_WHEEL_CIRCUMFERENCE
/* Time after which OBC enters 'not riding' mode*/
#define RIDE_TIMEOUT_MS 2500
// Time of inactivity needed to start sychronization of a ride
#define OBC_SERVER_SYNC_DEBOUNCE_MS 30000

#define OBC_SERVER_SYNC_MIN_DIST 0.1

#define ESP_INTR_FLAG_DEFAULT 0
#define BLINK_GPIO 2
#define REED_IO_NUM 21

#endif


