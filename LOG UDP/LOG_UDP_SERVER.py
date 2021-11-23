###############################################################################
# Bibliotecas:
# pip install pymongo
##############################################################################
from logging import error
import os
import socket
from datetime import datetime

DEBUG_MONGO = False
DEBUG_FILE = True

if (DEBUG_FILE):
    import logging

if (DEBUG_MONGO):
    from pymongo import MongoClient

###############################################################################
# Inicia serviço de log no IP e porta especificado
##############################################################################
UDP_IP = "0.0.0.0"
UDP_PORT = 6000
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

class color:
    Red = '\033[91m'
    Green = '\033[92m'
    Blue = '\033[94m'
    Cyan = '\033[96m'
    White = '\033[97m'
    Yellow = '\033[93m'
    Magenta = '\033[95m'
    Grey = '\033[90m'
    Black = '\033[90m'
    Default = '\033[99m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


os.system('color')  # Habilitar cores no log


###############################################################################
# Iniciando banco de dados
###############################################################################
if (DEBUG_MONGO):
    client = MongoClient('mongodb://localhost:27017/')  # Conecta ao banco
    db = client['logs']  # Referencia o database, neste caso é os logs

###############################################################################
#  Algumas funcções uteis:
#  * db.list_collection_names() -> lista coleções
#  * new_articles = articles.insert_many([article1, article2]), -> gravar mais de uma coleção.
#  print("The new article IDs are {}".format(new_articles.inserted_ids))
#  * articles.find_one() -> pega o unico elemento da coleção
# for article in articles.find():
#    print(article)

# result = logs.insert_one(data) -> Inserir no banco
# print("First article key is: {}".format(result.inserted_id))

# Pegar o documento do banco
# from bson.objectid import ObjectId
# def get(post_id):
#    document = client.db.collection.find_one({'_id': ObjectId(post_id)})

# pesquisas com filtros
# for article in articles.find({}, {"_id": 0, "author": 1, "about": 1}):
#   print(article)

###############################################################################
#  Aplicação
###############################################################################
print(color.Blue,  "+--------------------------------------------------------------------+")
print(color.Red,   "+--------------------------------------------------------------------+")
print(color.Green, "+--------------------------------------------------------------------+")
print(color.Green, "|                         LOG UDP SERVER                             |")
print(color.Green, "+--------------------------------------------------------------------+")
print(color.Red,   "+--------------------------------------------------------------------+")
print(color.Blue,  "+--------------------------------------------------------------------+")
print(color.Cyan, "Hostname: ", socket.gethostbyaddr(UDP_IP)[0])
print(color.Cyan, "Port: ", UDP_PORT)
print(color.White)

###############################################################################
#  Debug File
###############################################################################
if (DEBUG_FILE):
    directory = 'logs'
    logging.basicConfig(format='%(message)s', level=logging.INFO)
    
    if not os.path.exists(directory):
        os.makedirs(directory)

    def setup_logger(name, log_file, level=logging.INFO):
      
      for namelog in logging.root.manager.loggerDict:
         if (namelog == name):
            return logging.getLogger(name)
         
      handler = logging.FileHandler(log_file)
      logger = logging.getLogger(name)
      logger.setLevel(level)
      logger.addHandler(handler)
         
      return logger

###############################################################################
#  Loop Socket
###############################################################################


while True:
    try:
      data, addr = sock.recvfrom(2048)
      
      addres = str(addr[0]).replace('.', '_')
      
      insert = {
         "IP": addres,
         "data": data.decode(encoding='UTF-8'),
         "datetime": datetime.now()
      }
      
      text = insert["data"][:-1]
      
      if (DEBUG_MONGO):
         logs = db[addres]  # Coleção logs, se nao ter ele cria
         logs.insert_one(insert)  # Insere um novo documento
      
      if (DEBUG_FILE):
         #loggers = [logging.getLogger(name) for name in logging.root.manager.loggerDict]
         #print('loggers', loggers)
      
         datalogger = setup_logger(addres, '{}/{}.log'.format(directory, addres))
         datalogger.info(text)
         
         # logger = logging.getLogger()
         # handler = logging.FileHandler('{}/{}.log'.format(directory, addres))
         # logger.addHandler(handler)        
         # logger.info(text)
    except Exception as e:
      print("error:", e)
   