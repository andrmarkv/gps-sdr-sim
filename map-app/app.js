var express = require('express');
var app = express();

var server = require('http').Server(app);

server.listen(8000);

var io = require('socket.io').listen(server);

//UDP Socket parameters to communicate with GPS simulator
var PORT = 8001;
var HOST = '127.0.0.1';

//Handle UDP Socket response from GPS simulator (confirmation and current location)
function handleUDPResponse(msg, rinfo){
	console.log("got Response: %s", msg);
}

//Initialize UDP server client to communicate with GPS simulator
function initUDPClient(){
	var dgram = require('dgram');
	var message = new Buffer('PATH|123;123;100;100');

	var client = dgram.createSocket('udp4', handleUDPResponse);
	
	return client;
}

//Client to communicate with GPS simulator 
var client = initUDPClient();

app.use(express.static('public'));
app.get('/index.htm', function (req, res) {
   res.sendFile( __dirname + "/" + "index.htm" );
})

io.on('connection', function (socket) {
  socket.on('startMoving', function (data) {
	  console.log("got startMoving event: %j", data);
	  
	  var msg = data.latitude0 + ";" + data.longitude0 + ";" + data.latitude1 + ";" + data.longitude1 + ";" + data.speed + ";" + data.pause
	  sendPath("PATH;" + msg);
	  
	  for (var i = 0; i < 100; i++) {
		socket.emit('gpsUpdate', { pos: i });
	  }
	  
	  console.log("exit startMoving event");
  });
  
  socket.on('stopMoving', function (data) {
	  console.log("got stopMoving event: " + data);
  });
  
});

function sendPath(message){
	client.send(message, 0, message.length, PORT, HOST, function(err, bytes) {
	    if (err) throw err;
	    console.log('UDP message sent to ' + HOST +':'+ PORT + "msg: " + message);
	});
}
