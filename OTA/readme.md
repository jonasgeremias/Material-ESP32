# OTA via WEB SERVER

## Objetivo do projeto

Apresentar um exemplo de fácil implementação de aplicação que utiliza os dois modos de comunicação wi-fi, STA e AP e que necessite de atualização OTA pela rede local. A placa usada para os testes é do modelo ESP32-DEVKIT-V1.

Wi-fi STA - Configura o IP fixo e hostname na rede, conecta a um roteador pre confgurado.
Wi-fi AP - Gera um ponto de acesso para usar o serviço http via navegador ou outras aplicações.

OTA - O serviço de web server fica disponivel em ambas as redes Wi-fi.
No AP, o ip é fixo em 192.168.0.1 na porta http (80).

O sistema gera uma pagina web que é possivel carregar a imagem 'bin' para atualização do firmware para futuras atualizações, sem a necessidade de regravar via serial.

Por nao depender de API on line, pode ser enviada a imagem compilada via internet, e baixada em um smartphone para atualização do dispositivo em lugares em que não se tem acesso a internet, por exemplo.

O codigo possui tambem o led blink e monitoramento de tarefas e heap a cada tempo, além de sistemas de LOG do ESP32.

## Informações Importantes para o funcionamento do projeto   
* No menuconfig (`idf.py menuconfig`), deve ser habilitada a configuração de uso do arquivo CSV de partição (`partition.csv`). Nele deve ser criada as partições de OTA.
```
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,     data, nvs,  0x9000,  16K,
otadata, data, ota,  0xd000,  8K,
phy_init,data, phy,  0xf000,  4K,
factory,app,factory, 0x10000, 1M,
ota_0,app,ota_0,     ,1M,
ota_1,app,ota_1,     ,1M,
```
 * Caso de algum erro na compilação relacionada a partição, verifique se está configurado no menuconfig o tamanho correto da memória do seu ESP32.
 * Se o seu codigo estiver muito grande, ou seja, maior que o meu de 1M, será necessário criar as partições maiores, e dependendo do caso até comprar um esp32 com maior memória.

## Plugins
 * Para facilitar na parte de front-end, foi utilizado um framework chamado <a href="https://minicss.org/docs" target="_blank"> mini.css</a>, este é muito leve em comparações com alguns mais conhecidos, claro que não irei cita-los ;). 
 
## Algumas definições
 * No arquivo main.h, configure o nome da rede local e da rede que o ESP ira gerar. Atente para que o SSID fique com pelo menos 8 caracteres. Por padrão estão as minhas de teste:
```
// Rede Wifi
#define SSID_WIFI_AP "OTA-WEBSERVER"
#define PASS_WIFI_AP "123456789"
#define WIFI_AP_MAX_CON 1

// Rede local
#define SSID_WIFI_STA "Sua rede"
#define PASS_WIFI_STA "suasenha"
``` 
## Proximas implementações
 * Telas de: Tempo real,configurar dispositvo, relatório, Wifi manager.
 * Para isso será necessário usar os modos de procura de wi-fi, uso da memória não volátil (NVS) com uma partição dedicada.
 
## Projeto semelhante em ESP8266 com outras funcionalidades
<a href="https://github.com/jonasgeremias/ESP8266-WiFi-Manager" target="_blank"> Wi-Fi Manager com ESP8266</a>

## Autor
* Jonas P. Geremias

Se melhorar o código, não deixe de compartilhar comigo e qualquer coisa, mande um email para: 

<a href="mailto:jonasgeremias@hotmail.com?subject=projeto%20OTA%20via%20WEB%20%SERVER"> jonasgeremias@hotmail.com</a>


## Imagens da Aplicação
<img src="./images/taskmonitor.JPG" width="600" height="300" />
<img src="./images/1.jpeg" width="200" height="400" /><br/>
<img src="./images/2.jpeg" width="200" height="400" /><br/>
<img src="./images/3.jpeg" width="200" height="400" /><br/>

<video src="./images/video.mp4" width="600" height="auto" controls>
  Your browser does not support the video tag.
</video>

<br/>
 <br/>
 
##Valeu e Capricha! ;)