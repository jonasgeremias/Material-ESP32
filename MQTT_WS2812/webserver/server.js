var express = require('express');
var app = express();

//setting middleware
app.use(express.static('./public')); //Serves resources from public folder


app.listen(5000);
console.log('hdjashdjkas')