var socket = io.connect('http://localhost:8000');

socket.on('gpsUpdate', function (data) {
	console.log(data);
});

angular.module('appMaps', [ 'uiGmapgoogle-maps' ]).config(
		function(uiGmapGoogleMapApiProvider) {
			uiGmapGoogleMapApiProvider.configure({
				libraries : 'geometry,visualization'
			});
		}).controller('mainCtrl', function($scope, uiGmapGoogleMapApi) {
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
		    coords: {
		    	latitude : 24.495774,
				longitude : 54.358074
		    },
		    options: { 
		    	draggable: false, 
		    	opacity: 0.5, 
		    	icon: {
		    		//path: google.maps.SymbolPath.CIRCLE,
		    		url: 'img/green_marker.png',
		    		scaledSize: new google.maps.Size(20, 34)
		    	}, 
		    	//animation: google.maps.Animation.BOUNCE,
		    },
		};
		
		$scope.marker1 = {
			    id: 0,
			    coords: {
			    	latitude : 24.496152,
					longitude : 54.358846
			    },
			    options: { draggable: true },
			    events: {
			        dragend: function (marker, eventName, args) {
			        	msg = "lat: " + $scope.marker1.coords.latitude + ' ' + 'lon: ' + $scope.marker1.coords.longitude;
			        	//console.log(msg)
			        }
			    }
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
});
