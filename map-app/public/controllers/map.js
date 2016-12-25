var curLocation = {
	coords: {
		latitude : 24.495774,
		longitude : 54.358074
	}

};

var socket = io.connect('http://localhost:8000');

socket.on('gpsUpdate', function (data) {
	//console.log(data.latitude + " " + data.longitude);
	curLocation.coords.latitude = data.latitude; 
	curLocation.coords.longitude = data.longitude;
});

var myapp = angular.module('appMaps', [ 'uiGmapgoogle-maps' ]);

myapp.config(
		function(uiGmapGoogleMapApiProvider) {
			uiGmapGoogleMapApiProvider.configure({
				libraries : 'geometry,visualization'
			});
		});

myapp.controller("TimerController", function($scope, $interval){

    $interval(callAtTimeout, 1000, 0);

});


function callAtTimeout() {
//	console.log("Timeout occurred 1 " + curLocation.coords.latitude);
	socket.emit('getCurLocation', {});
}

myapp.controller('mainCtrl', function($scope, uiGmapGoogleMapApi) {
	$scope.map = {
		center : {
			latitude : 24.495774,
			longitude : 54.358074
		},
		zoom : 18,
		bounds : {}
	};
	uiGmapGoogleMapApi.then(function() {
		$scope.polyline = {
			id : 1,
			path : [ {
				latitude : 24.495310,
				longitude : 54.357449
			}, {
				latitude : 24.496082,
				longitude : 54.357787
			}, {
				latitude : 24.496512,
				longitude : 54.358929
			}, {
				latitude : 24.495867,
				longitude : 54.358336
			} ],
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
					path : google.maps.SymbolPath.BACKWARD_OPEN_ARROW
				},
				offset : '25px',
				repeat : '50px'
			} ],
			
			//Events registered with the polyline
			events: {
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
		    id: 0,
		    coords: curLocation.coords,
		    options: { 
		    	draggable: false, 
		    	opacity: 0.85, 
		    	icon: {
		    		url: 'img/cur_pos.png',
		    		scaledSize: new google.maps.Size(24, 24),
		    		anchor: new google.maps.Point(12, 12),
		    	}, 
		    	//animation: google.maps.Animation.BOUNCE,
		    },
		};
		
		$scope.marker1 = {
		    id: 0,
		    coords: {
		    	latitude : 50.393484,
				longitude : 30.516613
		    },
		    options: { draggable: true },
		    events: {
		        dragend: function (marker, eventName, args) {
		        	msg = "lat: " + $scope.marker1.coords.latitude + ' ' + 'lon: ' + $scope.marker1.coords.longitude;
		        	//console.log(msg)
		        }
		    }
		};
		
		$scope.connector = {
			id : 2,
			path : [$scope.marker0.coords, $scope.marker1.coords],
			stroke : {
				color : '#4e99e5',
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
		speed: 15,
		pause: 0,
	}
	
	$scope.startMoving = function() {
		//alert("GO!");
		socket.emit('startMoving', {
			latitude0: $scope.marker0.coords.latitude, 
			longitude0: $scope.marker0.coords.longitude,
			latitude1: $scope.marker1.coords.latitude, 
			longitude1: $scope.marker1.coords.longitude,
			speed: $scope.moveControl.speed,
			pause: $scope.moveControl.pause,
			});
	};
	
	$scope.stopMoving = function() {
		socket.emit('stopMoving', {});
	};
	
	$scope.joinMarkers = function() {
		socket.emit('startMoving', {
			latitude0: $scope.marker0.coords.latitude, 
			longitude0: $scope.marker0.coords.longitude,
			latitude1: $scope.marker1.coords.latitude, 
			longitude1: $scope.marker1.coords.longitude,
			speed: 10000,
			pause: 0,
			});
	};
});
