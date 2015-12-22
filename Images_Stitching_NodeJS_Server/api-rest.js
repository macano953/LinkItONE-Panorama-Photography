/**
 *-------------------------------------------------------------------
 * REST API: main function (https://en.wikipedia.org/wiki/Representational_state_transfer)
 * Allows to:
 *    - GET the current timestamp. 
 *    - POST a certain image from every electronics development board able to communicate with the server.
 * Developed using Node.js v4.2.1 and Express framework > 4.0.
 *-------------------------------------------------------------------
 */
var util = require('util');
var express = require("express"),
    app = express(),
    bodyParser  = require("body-parser"),
    methodOverride = require("method-override"),
    cors = require("cors"),
    path = require('path');
var devices = require('./controllers/devices');

app.enabled('trust proxy');
/*!< Will allow HTTP GET, POST urlencoded and POST with JSON body requests*/
app.use(bodyParser.urlencoded({
  extended: true,
  limit: '1000kb'
}));
app.use(bodyParser.json({limit: '1000kb'}));

app.use(methodOverride());
app.use(cors());

app.get('/api', function(req, res) {
  res.status(200).send("It's alive, it's alive!"); 
});

/*!< Routes definition. Second parameter is the function that handles the request. */
app.get('/api/timestamp/', devices.getTimestamp);
app.post('/api/images/upload', devices.uploadLinkit);

/*!< Handling non-declared route error */
app.use(function(req, res, next) {
  res.status(404).jsonp({msg: "Error", info: "404 Not Found"});
});

app.listen(3000,'0.0.0.0' ,function(req,res,err) {
  if(err){
    res.send(err);
  }
  else{
    util.log('API REST running on port 3000');
  }
});
