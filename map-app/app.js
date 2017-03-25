var express = require('express');
var app = express();

var server = require('http').Server(app);

server.listen(8000);

var io = require('socket.io').listen(server);

//UDP Socket parameters to communicate with GPS simulator
var PORT = 8001;
var HOST = '127.0.0.1';

//UDP Socket parameters to communicate with Control server
var PORT2 = 8002;
var HOST2 = '127.0.0.1';



//GPS SIMULATOR CLIENT

//Handle UDP Socket response from GPS simulator (confirmation and current location)
function handleUDPResponse(msg, rinfo){
	//console.log("got Response: %s", msg);
	if(msg.indexOf("LOCATION") > -1) {
		processCurLocation(msg);
	} else if(msg.indexOf("ADDED") > -1) {
		console.log("got start moving response: %s", msg);
	} else if(msg.indexOf("STOP") > -1) {
		console.log("got stop moving response: %s", msg);
	} else {
		console.log("got GPS UNKNOWN message: %s", msg);
	}
}

//Initialize UDP server client to communicate with GPS simulator
function initUDPClient(){
	var dgram = require('dgram');
	var client = dgram.createSocket('udp4', handleUDPResponse);
	return client;
}

//Client to communicate with GPS simulator 
var client = initUDPClient();

function sendUDPMessage(message){
	client.send(message, 0, message.length, PORT, HOST, function(err, bytes) {
	    if (err) throw err;
	    //console.log('UDP message sent to ' + HOST +':'+ PORT + " msg: " + message);
	});
}


//CONTROL CLIENT

//Handle UDP Socket response from Control system (OK to proceed to the next interval)
function handleUDPControl(msg, rinfo){
	//console.log("got Response: %s", msg);
	if(msg.indexOf("CONTROL") > -1) {
		processControl(msg);
	} else {
		console.log("got CONTROL UNKNOWN message: %s", msg);
	}
}

//Initialize UDP server client to communicate with Control server
function initUDPClientControl(){
	var dgram = require('dgram');
	var client = dgram.createSocket('udp4', handleUDPControl);
	return client;
}

//Client to communicate with Control server
var clientControl = initUDPClientControl();

function sendUDPControlMessage(message){
	clientControl.send(message, 0, message.length, PORT2, HOST2, function(err, bytes) {
	    if (err) throw err;
	    console.log('UDP message sent to ' + HOST2 +':'+ PORT2 + " msg: " + message);
	});
}



app.use(express.static('public'));
app.get('/index.htm', function (req, res) {
   res.sendFile( __dirname + "/" + "index.htm" );
})

io.on('connection', function (socket) {
  socket.on('startMoving', function (data) {
	  console.log("got startMoving event: %j", data);
	  
	  var msg = data.latitude0 + ";" + data.longitude0 + ";" + data.latitude1 + ";" + data.longitude1 + ";" + data.speed + ";" + data.pause
	  sendUDPMessage("PATH;" + msg);
	  
	  console.log("exit startMoving event");
  });
  
  socket.on('stopMoving', function (data) {
	  sendUDPMessage("STOP");
  });
  
  socket.on('getCurLocation', function (data) {
	  sendUDPMessage("CUR_LOC");
  });
  
  socket.on('startControl', function (data) {
	  console.log("got startControl event: %j", data);
	  var msg = data.tag + ";" + data.latitude + ";" + data.longitude
	  sendUDPControlMessage("CONTROL;" + msg);
  });
  
});

function processCurLocation(message){
	var msg = message.toString();
	var arr = msg.split(";");
	if (arr.length != 5) {
		console.log("Error! Got wrong LOCATION message: " + msg);
		return;
	}
	
	if (io) {
		io.emit('gpsUpdate', { latitude: arr[1], longitude : arr[2], status : arr[4] });
	}
}

function processControl(message){
	var msg = message.toString();
	var arr = msg.split(";");
	
	if (arr.length != 3) {
		console.log("Error! Got wrong CONTROL message: " + msg);
		return;
	}
	
	if (io) {
		io.emit('controlRelese', { state: arr[2]});
	}
	
//	console.log("processControl msg: " + msg);
//	console.log("processControl msg.state: " + msg.state);

}
