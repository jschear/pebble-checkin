var latitude; // saved to include in checkin request
var longitude;

var venueIds = []; // save venueIds so we don't need to send venue IDs to pebble

Pebble.addEventListener("ready",
	function(e) {
    if (localStorage['token']) {
      buildVenueList();
    }
    else {
      Pebble.sendAppMessage({ "error" : "Log in to Foursquare in the Pebble phone app" });
    }
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
		if (token !== null && token !== '' && token !== 'denied' && token !== 'CANCELLED') {
			localStorage['token'] = token;
			Pebble.showSimpleNotificationOnPebble("Checkin", "Successfully signed in to Foursquare");
      buildVenueList();
		}
		else {
			localStorage.removeItem('token');
			Pebble.showSimpleNotificationOnPebble("Checkin", "Error signing in to Foursquare");
		}
	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("Received message: " + JSON.stringify(e.payload));
		if (e.payload.checkin) {
			sendCheckIn(e.payload['index']);
		}
		else {
			console.log("Unknown appmessage key");
		}
	}
);

function buildVenueList() {
	navigator.geolocation.getCurrentPosition(successCallback, errorCallback); // Query for location
};

function successCallback(position) {
  var numVenues = 6;

  if (localStorage['token']) {
    latitude = position.coords.latitude;
    longitude = position.coords.longitude;

    var req = new XMLHttpRequest();
    req.open('GET', 'https://api.foursquare.com/v2/venues/search?v=20140102&oauth_token=' + localStorage['token'] + '&ll=' + latitude + "," + longitude + "&limit=" + numVenues, true);
    req.onload = function(e) {
      if (req.readyState == 4 && req.status == 200) {
          var venues = JSON.parse(req.responseText).response.venues;
          var messages = [{'venueList': 1, 'numVenues': venues.length}]; // signal start of list
          
          venues.forEach(function (venue, ind) {
            venueIds[ind] = venue.id;
            var address = venue.location.address != null ? venue.location.address.substring(0, 40) : "";
            var name = venue.name.substring(0, 40);
            myVenue = {'index': ind, 'address': address, 'name': name};
            messages.push(myVenue);
          });

          messages.push({'venueList': 1}); // signal end of list
          sendMessages(messages);
        } else {
          console.log("Error fetching venues");
          Pebble.sendAppMessage({ "error" : "Failed to fetch data from Foursquare" });
        }
    };
    req.send(null);
  }
  else {
    Pebble.sendAppMessage({ "error" : "Please log in again" });
  }
};

function errorCallback(error) {
    Pebble.sendAppMessage({ "error" : "Failed to get phone location" });
};

function sendMessages(messages) {
	console.log("Sending messages: " + JSON.stringify(messages));
	var timeout = 15;
  var maxNumRetries = 10;

	function sendMessage() {
		if (messages.length > 0) {
			var msg = messages.shift();
			Pebble.sendAppMessage(msg, 
				function(e) {
					setTimeout(sendMessage, timeout);
				},
				function(e) {
					console.log("Unable to deliver message with transactionId = " + e.data.transactionId + "\nRetrying");
          if (numRetries < maxNumRetries) {
					  messages.unshift(msg); // retry message
            numRetries++;
          }
          else {
            console.log("Message with transactionId = " + e.data.transactionId + " failed");
          }
					setTimeout(sendMessage, timeout);
				}
			);
		}
	};

	sendMessage();
};

function sendCheckIn(index) {
  if (venueIds[index] == null) {
    console.log("Received index that isn't in local array");
    Pebble.sendAppMessage({ "error" : "Failed to check in"});
  }
  else if (localStorage['token']) {
    var venueId = venueIds[index];
  	var req = new XMLHttpRequest();
  	req.open('POST', 'https://api.foursquare.com/v2/checkins/add?v=20140102&oauth_token=' + localStorage['token'] + "&venueId="+ venueId + '&ll=' + latitude + "," + longitude, true);
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
    			Pebble.showSimpleNotificationOnPebble("Checked in!", message);
    		}
    		else {
    			Pebble.showSimpleNotificationOnPebble("Checked in!", "Successfully checked in!");
    		}
    	} else {
    		console.log("Error checking in");
    		Pebble.sendAppMessage({ "error" : "Failed to check in"});
    	}
    };
  	req.send(null);
  }
  else {
    Pebble.sendAppMessage({ "error" : "Please log in again" });
  }
};
