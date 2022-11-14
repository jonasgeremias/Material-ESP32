# TFT Project

* Este projeto tem por objectivo ligar um display TFT de 3,5" (480 x 320) em um ESP32 utilizando o IDF.
Porém a maioria das bibliotecas utilizam o core Arduino, por isos este utiliza o core arduino como um componente.
* Esse projeto usa a biblioteca TFT_eSPI como componentes.

## Componentes
* Core Arduino: https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/esp-idf_component.html

   * Criar uma pasta chamada `components` e baixar o core arduino dentro. Se usar frequentemente, coloque na pasta components da instalação do IDF.

   ```
      mkdir -p components && \
      cd components && \
      git clone https://github.com/espressif/arduino-esp32.git arduino && \
      cd arduino && \
      git submodule update --init --recursive && \
      cd ../.. && \
      idf.py menuconfig
   ```

   * Vá para a seção `Arduino Configuration --->` e defina as configurações do projeto: 
      * `Autostart Arduino setup and loop on boot` -> _off_ para usar com o app_main() ou _on_ para setup() e loop().

   * Source code: adicione a linha `#include "Arduino.h"` e no app_main() `initArduino();`
   
   * Obs.: Se der o erro abaixo, altere o arquivo sdkconfig para `CONFIG_FREERTOS_HZ=1000`.
   ```
   CMake Error at components/arduino/CMakeLists.txt:215 (message):
   esp32-arduino requires CONFIG_FREERTOS_HZ=1000 (currently 100)
   ```

* TFT_eSPI
   * Baixar e colocar na pasta compoenents `git clone https://github.com/Bodmer/TFT_eSPI.git`.
   * Va no menuconfig/components/TFTe_SPI
   * Se não der certo, edite o arquivo `User_Setup.h` para utilizar o ILI9488:
   ```c
   #define ILI9488_DRIVER
   #define TFT_MISO 19
   #define TFT_MOSI 23
   #define TFT_SCLK 18
   #define TFT_CS   15  // Chip select control pin
   #define TFT_DC    2  // Data Command control pin
   #define TFT_RST   4  // Reset pin (could connect to RST pin)
   ```
   * Comentar outras definições de pino e driver.
   

## Criando a imagem em arquivo .h
* Selecione a imagem em bitmap que queira converter no programa `lcd_image_converter.exe`.
* Utilize o arquivo de configurações XML e JPG  
* Criar o arquivo `img.h` com a imagem bitmap.
* Definir os tamanhos: 
   ```c
      const uint16_t img_width = 320;
      const uint16_t img_height = 480;
      const unsigned short  img[] PROGMEM={
      array da imagem gerada pelo software
   }
   ```
* importe o arquivo  `.h` no `main.cpp` e no loop
   ```c
      tft.init();
      tft.setRotation(2);
      tft.fillScreen(TFT_BLACK);
      tft.setSwapBytes(true);
      tft.pushImage(0, 0, img_width, img_height, img);
   ```

* Buld e flash : `idf.py -p com11 flash monitor`