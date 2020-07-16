#ifndef TASKS_STORAGE_SPIFFS_MAIN_H
#define TASKS_STORAGE_SPIFFS_MAIN_H

#define SPIFFS_SYNC_INTERVAL_MS 60000
#define MAX_FILES 1

// init spiffs storage. Format partition if needed, print information about partition
void vInitSpiffs();

// save ride params and statistics to flash
void vSpiffsSyncOnStopTask(void* data);

#endif