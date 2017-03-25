//var center = [48.287776, 25.933566]; //Chernivtsi
//var center = [24.480085, 54.346342]; //Abu Dhabi
//var center = [45.753136, 21.224145]; //Timisuary
//var center = [38.328938, -76.465785]; //Solomons Island
//var center = [48.868279, 24.697502]; //Ivano Frankovsk
//var center = [49.810482, 23.970609]; //Lviv
//var center = [49.236338, 28.457390]; //Vinnitsa
//var center = [46.484863, 30.734532]; //Odessa
//var center = [48.470909, 35.029497]; //Dnipro
//var center = [44.617085, 33.523283]; //Sevastopol
//var center = [33.592476, -7.620509]; //Casablanka
//var center = [33.592476, -7.620509]; //Casablanka
var center = [28.130413, -15.449473]; //Las-Palmas


var curLocation = {
	coords : {
		latitude : center[0],
		longitude : center[1],
	}
};

var markerLocation = {
	coords : {
		latitude : center[0],
		longitude : center[1],
	}
};

var mapCenter = {
	coords : {
		latitude : center[0],
		longitude : center[1],
	}
};

//Create Base64 Object
var Base64={_keyStr:"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",encode:function(e){var t="";var n,r,i,s,o,u,a;var f=0;e=Base64._utf8_encode(e);while(f<e.length){n=e.charCodeAt(f++);r=e.charCodeAt(f++);i=e.charCodeAt(f++);s=n>>2;o=(n&3)<<4|r>>4;u=(r&15)<<2|i>>6;a=i&63;if(isNaN(r)){u=a=64}else if(isNaN(i)){a=64}t=t+this._keyStr.charAt(s)+this._keyStr.charAt(o)+this._keyStr.charAt(u)+this._keyStr.charAt(a)}return t},decode:function(e){var t="";var n,r,i;var s,o,u,a;var f=0;e=e.replace(/[^A-Za-z0-9+/=]/g,"");while(f<e.length){s=this._keyStr.indexOf(e.charAt(f++));o=this._keyStr.indexOf(e.charAt(f++));u=this._keyStr.indexOf(e.charAt(f++));a=this._keyStr.indexOf(e.charAt(f++));n=s<<2|o>>4;r=(o&15)<<4|u>>2;i=(u&3)<<6|a;t=t+String.fromCharCode(n);if(u!=64){t=t+String.fromCharCode(r)}if(a!=64){t=t+String.fromCharCode(i)}}t=Base64._utf8_decode(t);return t},_utf8_encode:function(e){e=e.replace(/rn/g,"n");var t="";for(var n=0;n<e.length;n++){var r=e.charCodeAt(n);if(r<128){t+=String.fromCharCode(r)}else if(r>127&&r<2048){t+=String.fromCharCode(r>>6|192);t+=String.fromCharCode(r&63|128)}else{t+=String.fromCharCode(r>>12|224);t+=String.fromCharCode(r>>6&63|128);t+=String.fromCharCode(r&63|128)}}return t},_utf8_decode:function(e){var t="";var n=0;var r=c1=c2=0;while(n<e.length){r=e.charCodeAt(n);if(r<128){t+=String.fromCharCode(r);n++}else if(r>191&&r<224){c2=e.charCodeAt(n+1);t+=String.fromCharCode((r&31)<<6|c2&63);n+=2}else{c2=e.charCodeAt(n+1);c3=e.charCodeAt(n+2);t+=String.fromCharCode((r&15)<<12|(c2&63)<<6|c3&63);n+=3}}return t}}

function getDistance(lat1, lon1, lat2, lon2) {
	  var R = 6378137.0; // Radius of the earth in m
	  var dLat = deg2rad(lat2-lat1);
	  var dLon = deg2rad(lon2-lon1); 
	  var a = 
	    Math.sin(dLat/2) * Math.sin(dLat/2) +
	    Math.cos(deg2rad(lat1)) * Math.cos(deg2rad(lat2)) * 
	    Math.sin(dLon/2) * Math.sin(dLon/2)
	    ; 
	  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a)); 
	  var d = R * c; // Distance in m
	  return d;
	}

function deg2rad(deg) {
  return deg * (Math.PI/180)
}

var socket = io.connect('http://localhost:8000');

var currentPathIndex = 0; // index of the currently active path element
var isReplayRunning = false; // to indicate if we are processing replay
var isStopped = false; // to indicate that STOP button was pressed
						// moving
var readyForNextMotion = false; // to indicate that we can replay next motion
var gotOK = false; // Indicator of confirmation to proceed to the next interval

//To handle state changes from moving to stopped and back
var curState = 0; //0 - stopped, 1 - moving

var myapp = angular.module('appMaps', [ 'uiGmapgoogle-maps', 'simpleGrid' ]);

myapp.config(function(uiGmapGoogleMapApiProvider) {
	uiGmapGoogleMapApiProvider.configure({
		libraries : 'geometry,visualization'
	});
});

myapp.controller("TimerController", function($scope, $interval) {
	$interval(callAtTimeout, 1000, 0);
});

function callAtTimeout() {
	// console.log("Timeout occurred 1 " + curLocation.coords.latitude);
	socket.emit('getCurLocation', {});
}

function sendControlMessage(tag, lat, lon) {
	gotOK = false;
	socket.emit('startControl', {
		tag : tag,
		latitude : lat,
		longitude : lon,
	});
	console.log("sendControlMessage done, tag: " + tag);
}

socket.on('controlRelese', function(data) {
	console.log("controlRelese state:" + data.state);
	gotOK = true;
});


myapp.controller('mainCtrl', function($scope, uiGmapGoogleMapApi) {
	$scope.map = {
		center : mapCenter.coords,
		zoom : 18,
		bounds : {}
	};
	
	$scope.myPath = [];
	
	$scope.isRepeat = true; //indicates that replay should keep running from beginning after reaching last position
	$scope.isControlled = false; //indicates that next step of the replay should wait for confirmation to proceed
	
	$scope.markers = []; //markers to shows loaded locations

	$scope.myGridConfig = {
		// should return your data (an array)
		getData : function() {
			return $scope.myPath;
		},

		options : {
			"showDeleteButton": true,
			"dynamicColumns": true,
			rowDeleted: function (row) {alert(row) },
			columns : [ {
				field : 'tag',
			}, {
				field : 'latitude'
			}, {
				field : 'longitude'
			}, {
				field : 'speed',
				inputType : 'number'
			}, {
				field : 'pause',
				inputType : 'number'
			}]
		}
	}
	
	socket.on('gpsUpdate', function(data) {
		// console.log(data.latitude + " " + data.longitude);
		stateChanged = false; //indicates state change
		
		tmp = data.status;
		if (tmp == 'STOPPED') {
			console.log('Got STOPPED gpsUpdate status');
			
			//Check if state changed from stopped/moving
			if(curState == 1){
				curState = 0;
				stateChanged = true;
			}
		} else if (tmp == 'MOVING'){
			console.log('Got MOVING gpsUpdate status');
			
			//Check if state changed from stopped/moving
			if(curState == 0){
				curState = 1;
				stateChanged = true;
			}
		} else {
			console.log('Got wrong gpsUpdate status: ' + tmp);
		}
		
		//Handle state changed from moving to stopped
		if (stateChanged) {
			if (curState == 0) {
				if ($scope.isControlled){ //Indicates that movement controlled by other messages
					if (isReplayRunning && $scope.myPath.length > 0) {	
						sendControlMessage($scope.myPath[currentPathIndex].tag, data.latitude, data.longitude);
					}
				} else {
					if (isReplayRunning) {
						$scope.replayNextElement();
					}
				}
			}
		}
		
		//Check if we got OK from the control server
		if (curState == 0) {
			if ($scope.isControlled){ //Indicates that movement controlled by other messages
				if (gotOK){ //Indicates that we got OK to proceed to the next interval
					if (isReplayRunning) {
						$scope.replayNextElement();
					}
				}
			}
		}
		
		curLocation.coords.latitude = data.latitude;
		curLocation.coords.longitude = data.longitude;
	});

	uiGmapGoogleMapApi.then(function() {
		$scope.polyline = {
			id : 1,
			path : $scope.myPath,
			stroke : {
				color : '#6060FB',
				weight : 3
			},
			editable : true,
			draggable : false,
			geodesic : true,
			visible : true,
			icons : [ {
				icon : {
					path : google.maps.SymbolPath.FORWARD_OPEN_ARROW
				},
				offset : '25px',
				repeat : '50px'
			} ],

			// Events registered with the polyline
			events : {
				click : function(gMarker, eventName, model) {
					alert("Model: event:" + eventName);
				}
			}
		};

		$scope.mapEvents = {
			click : function(eventName) {
				alert("Model: event:" + eventName);
			},
			dblclick : function(eventName) {
				alert("Model: event:" + eventName);
			},
		};

		$scope.marker0 = {
			id : 0,
			coords : curLocation.coords,
			options : {
				draggable : false,
				opacity : 0.85,
				icon : {
					url : 'img/cur_pos.png',
					scaledSize : new google.maps.Size(24, 24),
					anchor : new google.maps.Point(12, 12),
				},
			// animation: google.maps.Animation.BOUNCE,
			},
		};

		$scope.marker1 = {
			id : 0,
			coords : markerLocation.coords,
			options : {
				draggable : true,
				zIndex : google.maps.Marker.MAX_ZINDEX + 1,
			},
			events : {
				dragend : function(marker, eventName, args) {
					msg = "lat: " + $scope.marker1.coords.latitude + ' '
							+ 'lon: ' + $scope.marker1.coords.longitude;
					// console.log(msg)
				}
			}
		};

		$scope.connector = {
			id : 2,
			path : [ $scope.marker0.coords, $scope.marker1.coords ],
			stroke : {
				color : '#e53027',
				weight : 1
			},
			editable : false,
			draggable : false,
			geodesic : true,
			visible : true,
			icons : [ {
				icon : {
					path : google.maps.SymbolPath.FORWARD_OPEN_ARROW
				},
				offset : '25px',
				repeat : '50px'
			} ],
		};

	});

	$scope.moveControl = {
		speed : 15,
		pause : 0,
	}

	$scope.startMoving = function() {
		// alert("GO!");
		isStopped = false;
		socket.emit('startMoving', {
			latitude0 : $scope.marker0.coords.latitude,
			longitude0 : $scope.marker0.coords.longitude,
			latitude1 : $scope.marker1.coords.latitude,
			longitude1 : $scope.marker1.coords.longitude,
			speed : $scope.moveControl.speed,
			pause : $scope.moveControl.pause,
		});
	};

	$scope.stopMoving = function() {
		isStopped = true;
		isReplayRunning = false;
		socket.emit('stopMoving', {});
	};

	$scope.joinMarkers = function() {
		socket.emit('startMoving', {
			latitude0 : $scope.marker0.coords.latitude,
			longitude0 : $scope.marker0.coords.longitude,
			latitude1 : $scope.marker1.coords.latitude,
			longitude1 : $scope.marker1.coords.longitude,
			speed : 10000,
			pause : 0,
		});
	};
	
	$scope.showHideTable = function() {
		if (document.getElementById('grid-container-div').style.display == 'none') {
			document.getElementById('grid-container-div').style.display = 'inline';
			document.getElementById('show-hide').innerHTML = 'Hide'
		} else {
			document.getElementById('grid-container-div').style.display = 'none';
			document.getElementById('show-hide').innerHTML = 'Show'
		}
	};
	
	$scope.addLocation = function() {
		var newPoint = {
				tag : '',
				latitude : curLocation.coords.latitude,
				longitude : curLocation.coords.longitude,
				speed : $scope.moveControl.speed,
				pause :  $scope.moveControl.pause,
			};
		$scope.myPath.push(newPoint);
	};
	
	$scope.calcTotals = function() {
		if ($scope.myPath.length < 2) {
			return 0;
		}
		var tl = 0; //Total length of the path
		var tt = 0; //total seconds to travel distance
		for (var i = 1; i < $scope.myPath.length; i++) {
			var tl1 = getDistance(
					$scope.myPath[i].latitude, 
					$scope.myPath[i].longitude, 
					$scope.myPath[i - 1].latitude, 
					$scope.myPath[i - 1].longitude);
			
			var tt1 = tl1 / ($scope.myPath[i - 1].speed / 3.6); //time in m/sec
			
			tl = tl + tl1;
			tt = tt + tt1;
		}
		
		$scope.tl = tl;
		$scope.tt = tt;
	};
	
	$scope.tl = 0; //Total length of the path
	$scope.tt = 0; //total seconds to travel distance
	
//	$scope.$watch('polyline.path', function (newValue, oldValue, scope) {
//		alert(1);
//	}, true);
	
	$scope.parseLoadedFile = function (buf){
		try{
			arr = JSON.parse(buf);
			
			//Logically all what has to be done is
			//$scope.myPath = arr;
			//but it does not update polyline, updates only table
			
			/*
			 * We do complex and crazy way by reassigning existing
			 * elements and adding new ones
			 */
			for (var i = 0; i < arr.length; i++){
			    var obj = arr[i];
			    
			    var newPoint = {
						tag : obj.tag,
						latitude : obj.latitude,
						longitude : obj.longitude,
						speed : obj.speed,
						pause :  obj.pause,
				};
			    
			    if ($scope.myPath.length > i) {
			    	$scope.myPath[i] = newPoint;
				} else {
					$scope.myPath.push(newPoint);	
				}
			}
			
			/*
			 * Now we have to delete extra elements if initial
			 * array was longer then new one
			 */
			if ($scope.myPath.length > arr.length) {
				$scope.myPath.splice(arr.length, ($scope.myPath.length - arr.length));
			}
			
			/*
			 * Move center of the map to the first point of the polyline
			 */
			curLocation.coords.latitude = $scope.myPath[0].latitude;
			curLocation.coords.longitude = $scope.myPath[0].longitude;
		} catch(err) {
			return 0;
		}
	}
	
	$scope.parseLocationFile = function (buf){
		try{
			var rootObj = JSON.parse(buf);
			
			/*
			 * Loop over all nested objects extracting encoded attributes
			 * z3iafj (lat), f24sfvs(lon)
			 */
			for (var key in rootObj) {
			    // skip loop if the property is from prototype
			    if (!rootObj.hasOwnProperty(key)) continue;

			    var obj = rootObj[key];
			    
			    //Decode the String
			    var decodedString = Base64.decode(obj['z3iafj']);
			    var lat = (Number(decodedString) / 1.852) / 1e6;
			    
			    decodedString = Base64.decode(obj['f24sfvs']);
			    var lon = (Number(decodedString) / 1.852) / 1e6;
			    
			    var newMarker = {
                    id: Date.now(),
                    coords: {
                        latitude: lat,
                        longitude: lon
                    },
                    options : {
        				draggable : false,
        				opacity : 0.85,
        				icon : {
        					path: 'M 0,0 C -2,-20 -10,-22 -10,-30 A 10,10 0 1,1 10,-30 C 10,-22 2,-20 0,0 z',
	    	                scale: 1,
	    	                strokeWeight:2,
	    	                strokeColor:"#B40404"
        				},
        			},
                };
			    
			    $scope.markers.push(newMarker);
                $scope.$apply();
			}
		} catch(err) {
			return 0;
		}
	}
	
	$scope.loadMotionFile = function() {
		var fileSelector = document.createElement('input');
		fileSelector.setAttribute('type', 'file');
		fileSelector.addEventListener('change', readSingleFile, false);
		fileSelector.click();
		
		function readSingleFile(evt) {
		    //Retrieve the first (and only!) File from the FileList object
		    var f = evt.target.files[0]; 

		    if (f) {
		      var r = new FileReader();
		      r.onload = function(e) { 
		    	  var content = e.target.result;
		    	  $scope.parseLoadedFile(content);
		      }
		      r.readAsText(f);
		    } else { 
		      alert("Failed to load file");
		    }
		  }
	};
	
	$scope.loadLocationsFile = function() {
		var fileSelector = document.createElement('input');
		fileSelector.setAttribute('type', 'file');
		fileSelector.addEventListener('change', readLocationFile, false);
		fileSelector.click();
		
		function readLocationFile(evt) {
		    //Retrieve the first (and only!) File from the FileList object
		    var f = evt.target.files[0]; 

		    if (f) {
		      var r = new FileReader();
		      r.onload = function(e) { 
		    	  var content = e.target.result;
		    	  $scope.parseLocationFile(content);
		      }
		      r.readAsText(f);
		    } else { 
		      alert("Failed to load file");
		    }
		  }
	};
	
	$scope.saveLocationsFile = function() {
		var data = $scope.myPath;
		var json = JSON.stringify(data);
		var blob = new Blob([json], {type: "application/json"});
		var url = URL.createObjectURL(blob);
		
		// update link to new 'url'
		link = document.getElementById('save-location');
	    link.download = "motion_path.json";
	    link.href = url;
	};
	
	/*
	 * Signal to start replay stored locations.
	 * On the first step we have to get from current location
	 * to the currentPathIndex location
	 */
	$scope.startReplay = function() {
		if ($scope.myPath.length < 2) {
			alert("Error! Need at least two locations to replay!");
			return;
		}
		
		var lat0 = curLocation.coords.latitude;
		var lon0 = curLocation.coords.longitude;
		
		var lat1 = $scope.myPath[currentPathIndex].latitude;
		var lon1 = $scope.myPath[currentPathIndex].longitude;
		var speed = $scope.moveControl.speed;
		var pause = $scope.moveControl.pause;
		
		/*
		 * We need this indicator to start detection of the move
		 * as it actual move might happen after some delay
		 */
		isMoveDetected = false;
		
		socket.emit('startMoving', {
			latitude0 : lat0,
			longitude0 : lon0,
			latitude1 : lat1,
			longitude1 : lon1,
			speed : speed,
			pause : pause,
		});
		
		isReplayRunning = true;
		isStopped = false;
		readyForNextMotion = true;
		gotOK = true;
	}
	
	/*
	 * Signal to replay next element from the saved path
	 */
	$scope.replayNextElement = function() {
		if ($scope.myPath.length < 2) {
			alert("Error! Need at least two locations to replay!");
			return;
		}
		
		var lat0 = $scope.myPath[currentPathIndex].latitude;
		var lon0 = $scope.myPath[currentPathIndex].longitude;
		
		var lat1 = 0.0;
		var lon1 = 0.0;
		var speed = 0.0;
		var pause = 0.0;
		var tag = "";
		if ($scope.myPath.length == currentPathIndex + 1) {
			lat1 = $scope.myPath[0].latitude;
			lon1 = $scope.myPath[0].longitude;
			speed = $scope.myPath[currentPathIndex].speed;
			pause = $scope.myPath[currentPathIndex].pause;
			tag = $scope.myPath[currentPathIndex].tag;
			currentPathIndex = 0;
		} else {
			lat1 = $scope.myPath[currentPathIndex + 1].latitude;
			lon1 = $scope.myPath[currentPathIndex + 1].longitude;
			speed = $scope.myPath[currentPathIndex].speed;
			pause = $scope.myPath[currentPathIndex].pause;
			tag = $scope.myPath[currentPathIndex].tag;
			currentPathIndex = currentPathIndex + 1;
		}
		
		//If we have to introduce a pause
//		alert("before pause");

		setTimeout(function(){
			//alert("execution of the detalyed function starts, tag: " + tag);
			
			/*
			 * We need this indicator to start detection of the move
			 * as it actual move might happen after some delay
			 */
			readyForNextMotion = false;
			
			socket.emit('startMoving', {
				latitude0 : lat0,
				longitude0 : lon0,
				latitude1 : lat1,
				longitude1 : lon1,
				speed : speed,
				pause : pause,
			});
			
		}, pause * 1000);
		
//		alert("after pause");
	}
});
