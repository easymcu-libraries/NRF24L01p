NRF24L01p
NRF24L01+ library for EasyMCU
This is a wireless module with spi protocol

Check easymcu.ir for our tutorials and wiring diagrams

To download. click the DOWNLOAD ZIP button. Check that the downloaded folder contains .cpp and .h files. Place the content (*.h + *.cpp) of downloaded library folder @ ./module_libraries/ Directory (that is next to EasyMCU app). Restart the EasyMCU app. it is visible in library tab of EasyMCU app now.

Double check config.h for EMCU_SSP, make sure following values are ...

#define EMCU_SSP                1
#define EMCU_SPI                0
Have fun ...
