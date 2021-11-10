# Fita de LEDs WS2812 com ESP32 e MQTT

* Essa aplicação tem por principal objetivo utilizar o protocolo MQTT para atualizar
a cor dos LEDs dos dispositivos.
* O serviço MQTT será instalado no Windows. As configurações estão em na pasta `Broker_MQTT`.
* O serviço NODE com rotas HTTP e função de MQTT está na pasta `webserver`. 
* O envio de dados ara o dispositivo será no fomrato JSON.
* Tem 10 modos pré definidos de cores, então deve pressionar o botão 11 vezes para chegar no modo MQTT.
* Principais aprendizados:
    1. MQTT;
    2. Criação de structs;
    3. Filas (Queue)
    4. Biblioteca cJSON;
    5. Atualização da fita por SPI;
    6. Uso de Ponteiros;
* Como melhorias:
    1. Publicação no tópico `lampadas/ID/status`.
    2. NVS para salvar o modo.
    3. WEB SERVER para configurar os links MQTT;
    4. Memórias para varios modos.

## Tutoria para implementação do MQTT

### Instalando o Mosquitto:
1. Instalar o mosquitto em <a href="https://mosquito.org/">mosquito.org</a>
2. Adicionar os arquivos dlls.
3. Configurar o PATH do Mosquitto nas variáveis de ambiente do Windows: inserir no PATH o link do diretorio da instalação do Mosquitto. Ex.: `C:\Program Files\Mosquitto\mosquitto_pub.exe`
4. Configurar o serviço do Mosquitto para inicialização manual em `services.msc`.
5. Testar a instalação do Mosquito com o comando mosquito -–help

### Testando manualmente:
Acessar a pasta da instalação e abrir 3 CMDs:
* Broker: `mosquitto -v -p 1883`
* Sub: `Mosquitto_sub –h localhost –p 1883 –t /topic/subtopic`
* Pub: `mosquitto_pub –h localhost –p 1883 –t /topic/subtopic –m teste`

####  Caracteres Curingas:
* Usar o `#` é utilizado para permitir todos os 
níveis subsequentes da hierarquia. Ex.: `topic/#`. 
* Usar o `+` é utilizado para permitir um único nível. Ex.: `+/subtopic`. 
* Para receber as mensagens de status do broker assine o tópico: `$SYS/#`.
* Para receber todas as mensagens dos clientes MQTT assine o tópico: `#`.

### Configuração do Broker
* Iniciar o Broker com arquivo de configuração: `mosquitto -c mosquitto.conf –v`. Ou `mosquitto -c C:\Users\Jonas\Desktop\mqtt\trabalhoMQTT\MQTT_WS2812\Broker_MQTT\mosquitto.conf –v`

#### Criar usuário ADMIN
* Execute o comando `mosquitto_passwd -c passwordfile.pwd admin` para criar um usuário chamado admin inserir uma senha: Ex.: `abc`.
* No arquivo `mosquitto.conf`, coloque `allow_anonymous false`para bloquear acessos anónimos.
* No arquivo `mosquitto.conf`,  na seção `Default authentication and topic access control` tire o comentário do comando
`password_file` e adicione o link do arquivo `passwordfile.pwd`.
    * Para _adicionar_ um usuário: `mosquitto_passwd -b passwordfile.pwd user1 pass1234`.
    * Para _deletar_ um usuário: `mosquitto_passwd -D passwordfile.pwd user1`
    * Para gerar um _novo hashed password_ (para todos os usuários): `mosquitto_passwd -U passwordfile.pwd`
* Adcicione o comando `listener 1883 0.0.0.0` para que o MQTT seja executado na LAN e não apenas no computador local.

#### Criar usuário DEVICE
* Execute o comando: `mosquitto_passwd -b passwordfile.pwd device device123`.

#### Testando com usuário
* iniciar o Mosquitto Broker: `mosquitto -c mosquitto.conf –v`.
* Este comando será recusado por autenticação: `Mosquitto_sub –h localhost –p 1883 –t /topic/subtopic`.
* Comando com autenticação será aceito: `Mosquitto_sub –h localhost –p 1883 –u device –P device123 –t /topic/subtopic`. Note o `–u admin –P abc`. para publicar: `Mosquitto_pub –h localhost –p 1883 –u device –P device123 –t /topic/subtopic -m {"color":[0,255,0], "qtd_leds": 72}`

### Documentando a aplicação com a fita de led WS2812

#### Funcionamento do dispositvo:

* O dispositivo se conectará ao broker através da autenticação : usuário: `device` e senha `device123`.
* Cada dispositivo terá um ID diferente e assinarao os tópicos: `lampadas/ID/config`
* As configurações de modo vai depender da configuração recebida: 
```
{
    "mode": 1,
    "description": "meu modo 1",
    "timer_on" : 500,
    "timer_off" : 500,
    "timer_rising" : 500,
    "timer_falling" : 500,
    \"colors\":[\"0xF00000\",\"0xFF0000\",\"0xFFF000\",\"0xFFFF00\", \"0xFFFFF0\", \"0xFFFFFF\"]
}
```

#### Testando a placa
* Inciar mosquito: `mosquitto -c mosquitto.conf –v`

* Publique conforme o comando abaixo para atualizar com uma cor só (modo 1):
`Mosquitto_pub –h localhost –p 1883 –u device –P device123 –t /lampadas/1/config -m "{\"mode\":1,\"description\":\"meu modo 1\",\"colors\":[\"0xF00000\"],\"timer_on\" : 500,\"timer_off\" : 500,\"timer_rising\" : 0,\"timer_falling\" : 0}"`

* Publique conforme o comando abaixo para atualizar com varias cores (modo 2):
`Mosquitto_pub –h localhost –p 1883 –u device –P device123 –t /lampadas/1/config -m "{\"mode\":1,\"description\":\"meu modo 1\",\"colors\":[\"0xFF0000\",\"0x00FF00\",\"0x0000FF\",\"0xFFFF00\", \"0x00FFFF\", \"0x400080\"],\"timer_on\" : 100,\"timer_off\" : 100,\"timer_rising\" : 500,\"timer_falling\" : 500}"`

* Note que o json deve ser no formato de texto.
* Após enviar o comando, se o dispositivo 1 estiver conectado ao broker, ele receberá o comando.

* Algumas cores para referencia:
    * RED : 0xFF0000
    * GREEN : 0x00FF00
    * BLUE : 0x0000FF
    * WHITE : 0xFFFFFF
    * BLACK : 0x000000
    * YELLOW : 0xFFFF00
    * CYAN : 0x00FFFF
    * MAGENTA : 0xFF00FF
    * PURPLE : 0x400080
    * ORANGE : 0xFF3000
    * PINK : 0xFF1493