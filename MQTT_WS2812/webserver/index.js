/*****************************************************************************
* Instalação e execução:
* npm install mqtt express --save
* npm install express --save
* node index.js
******************************************************************************/

/*****************************************************************************
* MQTT
******************************************************************************/
// Certificado CA
// const fs = require('fs');
// var caFile = fs.readFileSync("ca.crt");

var config = {
   clientId: "webserver" + new Date().getTime(),
   port: 1883,
   // host:'192.168.1.71',
   // protocol:'mqtts',
   rejectUnauthorized : true,
   // ca:caFile
   username: "device",
   password: "device123",
   clean: true
};

var mqtt = require('mqtt');
var client = mqtt.connect("mqtt://localhost", config);

client.on("connect", function () { console.log("connected  " + client.connected); })
client.on("error", function (error) { console.log("Can't connect" + error); });
client.on('message', function (topic, message, packet) {
   console.log("message is " + message);
   console.log("topic is " + topic);
});

// Função publicar
async function publish(topic, msg, configPUB) {
   console.log("publishing", msg);
   if (client.connected == true) {
      const ret = await client.publish(topic, msg, configPUB);
      return {pub:true}
   }
   return {pub:false}
}

// Tópicos e configurações
const configPUB = {retain: false, qos: 1};
const topicConfig = `/lampadas/+/config`;
const topicStatus = `/lampadas/+/status`;
const topicMode = `/lampadas/+/mode`;

// Assinando
client.subscribe(topicConfig, { qos: 0 });
client.subscribe(topicStatus, { qos: 0 });
client.subscribe(topicMode, { qos: 0 });

// Inicia serviço HTTP
var express = require('express');
var bodyParser = require('body-parser');
const app = express();
app.use(bodyParser.json())

// Configurando middleware para arquivos estáticos
app.use(express.static('./public')); //Serves resources from public folder

/*****************************************************************************
* Rotas
******************************************************************************/
app.get('/', async (req, res) => {
   const ret = await publish(topicMode, JSON.stringify({mode:1}), configPUB);
   res.json({pub:ret});
})

app.post('/pubconfig', async (req, res) => {
   var buf = JSON.stringify(req.body.msg);
   console.log(buf);
   const ret = await publish(topicConfig.replace('+', req.body.id), buf, configPUB);
   res.json({pub:ret});
})

app.post('/pubmode', async(req, res) => {
   var buf = JSON.stringify(req.body.msg);
   const ret = await publish(topicMode.replace('+', req.body.id), buf, configPUB);
   res.json({pub:ret});
})

app.post('/pubstatus', async (req, res) => {
   var buf = new Buffer.from(JSON.stringify(req.body.msg));
   const ret = await publish(topicStatus.replace('+', req.body.id), buf, configPUB);
   res.json({pub:ret});
})

/*****************************************************************************
* Inicia servidor HTTP
******************************************************************************/
const port = 3000;
app.listen(port, () => {
  console.log(`APP listening at http://localhost:${port}`)
})