# ESP LOG via UDP
## Descrição

Biblioteca para utilizar o esp_log (ESP_LOI, ESP_LOGW, ESP_LOGE ...) via socket UDP.
Claro que se não estiver conectado a um wifi, não será possivel visualizar o log.
obs.: Não gosto de criar os arquivos Kconfig para configurar os parâmetros via menuconfig.

### Servidor:

Foi criado um serviço de monitoramento em Python. Este serviço tem as opções:
* Imprimir no console;
* Imprimir em um arquivo; Pacote: `pip install logging`.
* Imprimir num documento do MongoDB. Pacote: `pip install pymongo`.

## Como usar

* Mude o valores das constantes no .h:
``` 
const int LOG_UDP_PORT = SUA_PORTA; // 5001
const char *LOG_UDP_IP = "SEU IP SERVER"; // "192.168.1.100"
```

* Chame a função depois que conectar no wifi:
```
log_udp_init(LOG_UDP_IP, LOG_UDP_PORT, log_udp_vprintf);
```

* Execute o arquivo `LOG_UDP_SERVER.py` com o comando: 
```
python LOG_UDP_SERVER.py
```

## Demonstrando o funcionamento

Neste gif, do lado esquerdo é o LOG na serial, e do lado direito o LOG por socket UDP:

<img src="./videos/log udp.gif" width="800" heigth="600">