#ifndef TASKS_STORAGE_SPIFFS_MAIN_H
#define TASKS_STORAGE_SPIFFS_MAIN_H

#define MAX_FILES 1

// init spiffs storage. Format partition if needed, print information about partition
void vInitSpiffs();

// save ride params and statistics to flash
void vSpiffsMainTask(void* data);

#endif