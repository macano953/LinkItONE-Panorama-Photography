var util = require('util');
var express = require("express"),
    app = express(),
    bodyParser  = require("body-parser"),
    methodOverride = require("method-override"),
    cors = require("cors"),
    path = require('path'),
    mysql = require('mysql');
var devices = require('./controllers/devices');

app.enabled('trust proxy');
app.use(bodyParser.urlencoded({
  extended: true,
  limit: '1000kb'
}));
app.use(bodyParser.json({limit: '1000kb'}));

app.use(methodOverride());
app.use(cors());

connection = mysql.createConnection({
  host: '46.101.6.192',
  user: 'root',
  password: 'mediatek',
  database: 'images',
  port: 3306
}); 

app.get('/api', function(req, res) {
  res.status(200).send("Yes, I am alive!"); 
});

app.get('/api/images/', devices.getImages);
app.get('/api/uploadimage/', devices.upload);
app.get('/api/images/:id/:drone', devices.findImages);
app.get('/api/timestamp/', devices.getTimestamp);
app.post('/api/images/upload', devices.uploadLinkit);

app.use(function(req, res, next) {
  res.status(404).jsonp({msg: "Error", info: "404 Not Found"});
});

app.listen(3000,'0.0.0.0' ,function(req,res,err) {
  if(err){
    res.send(err);
  }
  else{
    handleDisconnect(); 
    util.log('API REST running on port 3000');
  }
});

var handleDisconnect = function handleDisconnect() {
                                             
  connection.connect(function(err) {              
    if(err) {                                     
        util.log('REST_API: error when connecting to db:', err);
        setTimeout(handleDisconnect, 2000); 
      }
      else {
        util.log("Connected to database");     
      }
  });                                     
                                        
  connection.on('error', function(err) {
    util.log('db error:', err);
    if(err.code === 'PROTOCOL_CONNECTION_LOST') { 
      handleDisconnect();                         
    } else {                                      
      throw err;                                 
    }
  });
  
  setInterval(function () {
    connection.query('SELECT 1');
	}, 30000);
}
