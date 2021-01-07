#ifndef WIFI_H
#define WIFI_H


/**
 * @brief synchronously initialize wifi as a station and connect to sdkconfig specified wifi network
 * */
esp_netif_t *vInitWifiStation(void);

/**
 * @brief disconnect, turn off and unload wifi driver;
 * */
void vDeinitWifiStation(esp_netif_t * wifi_netif_instance);

#endif