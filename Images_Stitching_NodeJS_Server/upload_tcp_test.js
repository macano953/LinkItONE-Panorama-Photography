var net = require('net');

var server = new net.Server();
server.on('connection', function (socket) {
	var data = 0;
  socket.on('data', function (buffer) {
    data+=buffer;
  });
  socket.on('end', function (buffer) {
    console.log(data);	
  });
});
server.on('error', function (e) {
  if (e.code == 'EADDRINUSE') {
    console.log('Address in use, retrying...');
    setTimeout(function () {
      server.close();
      server.listen(PORT, HOST);
    }, 1000);
  }
});
server.listen(3000);
