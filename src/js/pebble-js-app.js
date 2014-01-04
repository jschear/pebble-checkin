Pebble.addEventListener("ready",
	function(e) {
    	console.log("JavaScript app ready and running!");
  	}
);

Pebble.addEventListener("showConfiguration", 
	function(e) {
	    var configUrl = 'https://s3.amazonaws.com/pebble-checkin/configurable.html';
	    Pebble.openURL('https://foursquare.com/oauth2/authorize?client_id=OSFON3MS5S0ZRHL5CFN1EN0QB0JGLDGINRJB4YSEJTMHKTRM&response_type=token&redirect_uri=' + encodeURIComponent(configUrl));
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
  		var token = e.response;
  		if (token != null && token !== '' && token !== 'denied' && token !== 'CANCELLED') {
  			localStorage['token'] = token;
  			console.log("Storing token: " + token);
  			Pebble.showSimpleNotificationOnPebble("Checkin", "Successfully signed in to Foursquare");
  			buildVenueList();
  		}
  		else {
  			localStorage['token'] = '';
  			Pebble.showSimpleNotificationOnPebble("Checkin", "Error signing in to Foursquare");
  		}
  	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));
		if (e.payload.fetchVenues) {
			buildVenueList(e.payload.numVenues);
		}
		else if (e.payload.checkin) {
			sendCheckIn(e.payload.venueId);
		}
		else {
			console.log("Unknown appmessage key");
		}
	}
);

function buildVenueList(numVenues) {

	function successCallback(position) {
		if (!localStorage['token']) {
			Pebble.sendAppMessage({ "error" : "Failed to authenticate\nTry logging into Foursquare again" });
		} else {
			var req = new XMLHttpRequest();
		  	req.open('GET', 'https://api.foursquare.com/v2/venues/search?v=20140102&oauth_token=' + localStorage['token'] + '&ll=' +  position.coords.latitude + "," + position.coords.longitude + "&limit=" + numVenues, true);
		  	req.onload = function(e) {
		    	if (req.readyState == 4 && req.status == 200) {
			        var venues = JSON.parse(req.responseText).response.venues;
			        var messages = [{'fetchVenues': 1, 'numVenues': venues.length}]; // signal start of list
			        venues.forEach(function (venue, ind) {
		            	messages.push({'index': ind, 
		            				   'venueId': venue.id,
		            				   'address': venue.location.address != null ? venue.location.address.substring(0, 40) : "",
		            				   'name': venue.name.substring(0, 40)
		            				  });
		   	            });
		            messages.push({'fetchVenues': 1}); // signal end of list
		            sendMessages(messages);
		      	} else {
		      		console.log("Error fetching venues");
		      		Pebble.sendAppMessage({ "error" : "There was an error fetching data from Foursquare" });
		      	}
		    };
		  	req.send(null);
		}
	};

	navigator.geolocation.getCurrentPosition(successCallback, errorCallback); // Query for location
};

function errorCallback(error) {
    Pebble.sendAppMessage({ "error" : "Failed to get phone location\nIs your GPS on?" });
};

function sendMessages(messages) {
	console.log("Sending messages: " + JSON.stringify(messages));
	timeout = 50;

	function sendMessage() {
		if (messages.length > 0) {
			var msg = messages.shift();
			console.log("Sending message: " + JSON.stringify(msg));
			Pebble.sendAppMessage(msg, 
				function(e) {
					console.log("Successfully delivered message with transactionId = " + e.data.transactionId);
					setTimeout(sendMessage, timeout);
				},
				function(e) {
					console.log("Unable to deliver message with transactionId = " + e.data.transactionId + " Error is: " + e.error.message + " \nRetrying");
					messages.unshift(msg); // retry message
					setTimeout(sendMessage, timeout);
				}
			);
		} else {
			console.log("Done sending!");
		}
	};
	sendMessage();
};

function sendCheckIn(venueId) {
	console.log("Checking in at venue: " + venueId);

	if (!localStorage['token']) {
			Pebble.sendAppMessage({ "error" : "Failed to authenticate\nTry logging in again" });
	} else {
		var req = new XMLHttpRequest();
	  	req.open('POST', 'https://api.foursquare.com/v2/checkins/add?v=20140102&oauth_token=' + localStorage['token'] + "&venueId="+ venueId, true);
	  	req.onload = function(e) {
	    	if (req.readyState == 4 && req.status == 200) {
	    		var notifications = JSON.parse(req.responseText).response.notifications;
    			var hasMessage = false;
    			var message = '';
    			if (notifications) {
		    		for (var i = 0; i < notifications.length; i++) {
		    			if (notifications[i]['type'] === 'message') {
		    				hasMessage = true;
		    				message = notifications[i].item.message;
		    				break;
		    			}
		    		}
		    	}
	    		if (hasMessage) {
	    			Pebble.showSimpleNotificationOnPebble("Checkin", "Checked in! Foursquare says: " + message);
	    		}
	    		else {
	    			Pebble.showSimpleNotificationOnPebble("Checkin", "Successfully checked in!");
	    		}
	      	} else {
	      		console.log("Error checking in");
	      		Pebble.sendAppMessage({ "error" : "Failed to check-in\nTry logging in again" });
	      	}
	    };
	  	req.send(null);
	}
};
