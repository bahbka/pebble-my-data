// -*-coding: utf-8 -*-
// vim: sw=2 ts=2 expandtab ai

var MSG = {
  PERIODIC_UPDATE: 0,
  SELECT_SHORT_PRESS_UPDATE: 1,
  SELECT_LONG_PRESS_UPDATE: 2,
  UP_SHORT_PRESS_UPDATE: 3,
  UP_LONG_PRESS_UPDATE: 4,
  DOWN_SHORT_PRESS_UPDATE: 5,
  DOWN_LONG_PRESS_UPDATE: 6,
  JSON_RESPONSE: 7,
  CONFIG: 8,
  ERROR: 9
};

// default settings
var config = {
  "config_location": false,
  "config_vibrate": true,
  "config_seconds": true
};

function http_request(url) {
  var req = new XMLHttpRequest();

  req.open('GET', url, true);
  req.onload = function(e) {

    if (req.readyState == 4) {
      if(req.status == 200) {
        try {
          var response = JSON.parse(req.responseText);
          //console.log("success: " + JSON.stringify(response));
          response["msg_type"] = MSG.JSON_RESPONSE;
          Pebble.sendAppMessage(response);

        } catch(e) {
          console.log("json parse error");
          Pebble.sendAppMessage({ "msg_type": MSG.ERROR });
        }

      } else {
        console.log("fetch error");
        Pebble.sendAppMessage({ "msg_type": MSG.ERROR });
      }
    }

  }

  req.send(null);
}

function fetch_data(url) {
  if (config["config_location"]) {
    if(navigator.geolocation) {
      navigator.geolocation.getCurrentPosition(
        function(position) {
          var latitude = position.coords.latitude;
          var longitude = position.coords.longitude;

          var s = (url.indexOf("?")===-1)?"?":"&";
          http_request(url + s + 'lat=' + latitude + '&lon=' + longitude);
        },
        function(error) {
          //error error.message
          /*
            TODO inform user about error
            PERMISSION_DENIED (numeric value 1)
            POSITION_UNAVAILABLE (numeric value 2)
            TIMEOUT (numeric value 3)
          */
          http_request(url);
        },
        { maximumAge: 1800000 } // 30 minutes
      );
    } else {
      //error geolocation not supported
      http_request(url);
    }
  } else {
    http_request(url);
  }
}

Pebble.addEventListener("ready",
  function(e) {
    console.log("JavaScript app ready and running!");

    var json = window.localStorage.getItem('pebble-my-data');

    if (typeof json === 'string') {
      try {
        config = JSON.parse(json);
        //console.log("loaded config = " + JSON.stringify(config));
        config["msg_type"] = MSG.CONFIG;
        Pebble.sendAppMessage(config); // send current config to pebble

      } catch(e) {
        console.log("stored config json parse error");
        Pebble.sendAppMessage({ "msg_type": MSG.ERROR });
      }
    }
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    //console.log("received message: " + JSON.stringify(e.payload));

    config["msg_type"] = MSG.CONFIG;
    Pebble.sendAppMessage(config); // send current config to pebble

    if (config["url"]) {
      var url = config["url"];
      var s = (url.indexOf("?")===-1)?"?":"&";

      if (e.payload["refresh"] == MSG.SELECT_SHORT_PRESS_UPDATE) {
        url = url + s + "select=1";
      } else if (e.payload["refresh"] == MSG.SELECT_LONG_PRESS_UPDATE) {
        url = url + s + "select=2";
      } else if (e.payload["refresh"] == MSG.UP_SHORT_PRESS_UPDATE) {
        url = url + s + "up=1";
      } else if (e.payload["refresh"] == MSG.UP_LONG_PRESS_UPDATE) {
        url = url + s + "up=2";
      } else if (e.payload["refresh"] == MSG.DOWN_SHORT_PRESS_UPDATE) {
        url = url + s + "down=1";
      } else if (e.payload["refresh"] == MSG.DOWN_LONG_PRESS_UPDATE) {
        url = url + s + "down=2";
      }

      fetch_data(url);

    } else {
      Pebble.sendAppMessage({ "msg_type": MSG.JSON_RESPONSE, "content": "URL not defined, check settings in Pebble App" });
    }
  }
);

Pebble.addEventListener('showConfiguration', function () {
  if (config["url"]) {
    url = config["url"];
  } else {
    url = "";
  }
  //console.log("put options = " + JSON.stringify(config));

  Pebble.openURL('data:text/html,'+encodeURI('_HTMLMARKER_<!--.html'.replace('_CONFIG_', JSON.stringify(config), 'g')));
});

Pebble.addEventListener("webviewclosed", function(e) {
  if ((typeof e.response === 'string') && (e.response.length > 0)) {
    config = JSON.parse(decodeURIComponent(e.response));
    //console.log("got options = " + JSON.stringify(config));

    window.localStorage.setItem('pebble-my-data', e.response);

    config["msg_type"] = MSG.CONFIG;
    //console.log("push config = " + JSON.stringify(config));
    Pebble.sendAppMessage(config); // send current config to pebble

    if (config["url"]) {
      fetch_data(config["url"]);

    } else {
      Pebble.sendAppMessage({ "msg_type": MSG.JSON_RESPONSE, "content": "URL not defined, check settings in Pebble App" });
    }
  }
});
