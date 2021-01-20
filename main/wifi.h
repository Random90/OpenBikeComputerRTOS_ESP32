#ifndef WIFI_H
#define WIFI_H


/**
 * @brief synchronously initialize wifi as a station and connect to sdkconfig specified wifi network
 * */
short vInitWifiStation(void);

/**
 * @brief disconnect, turn off and unload wifi driver;
 * */
void vDeinitWifiStation();

#endif