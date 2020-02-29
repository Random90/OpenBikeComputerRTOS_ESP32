# ESP32-PCD8544

A dedicated driver library implementation of PCD8544 graphic controller for
[Espressif IoT Development Framework (ESP-IDF)](https://github.com/espressif/esp-idf).

(PCD8544 is a.k.a. [Nokia 5110 LCD module](https://www.google.co.jp/search?q=nokia5110+lcd&tbm=isch)).

Documentation and API reference is available at readthedocs.org.
![docs](https://readthedocs.org/projects/esp32-pcd8544/badge/?version=latest)

http://esp32-pcd8544.readthedocs.io/en/latest/

Application examples are in separeted repository: https://github.com/yanbe/esp32-pcd8544-examples

## Key Features

- All functionalities in the PCD8544 datasheet are implemented
- Enforce native pin assignemnt for fast and low latency IO\_MUX direct IO.
- Manual Pin assignment is availble via API  ( no #define PIN\_NUM\_XX in header files)
- Asyncronous DMA-transfer mode is supported on HSPI / VSPI host
- Efficient draw lines / formatted text / bitmap APIs
- Good sample codes / examples (TBD)
