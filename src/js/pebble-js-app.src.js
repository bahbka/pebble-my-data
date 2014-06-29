// -*-coding: utf-8 -*-
// vim: sw=2 ts=2 expandtab ai

//TODO geolocation

var config = {};

function fetch_data() {
  var req = new XMLHttpRequest();

  if (config["url"]) {
    req.open('GET', config["url"], true);
    req.onload = function(e) {

      if (req.readyState == 4) {
        if(req.status == 200) {
          try {
            var response = JSON.parse(req.responseText);
            //console.log("success: " + JSON.stringify(response));
            Pebble.sendAppMessage(response);

          } catch(e) {
            console.log("json parse error");
          }

        } else {
          console.log("fetch error");
        }
      }
    }
    req.send(null);

  } else {
    Pebble.sendAppMessage({ "content": "URL not defined, chech settings in Pebble App" });
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

      } catch(e) {
        console.log("json parse error");
      }
    }
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    //console.log("received message: " + JSON.stringify(e.payload));
    fetch_data();
  }
);

Pebble.addEventListener('showConfiguration', function () {
  if (config["url"]) {
    url = config["url"];
  } else {
    url = "";
  }

  Pebble.openURL('data:text/html,'+encodeURI('_HTMLMARKER_<!--.html'.replace('"_URL_"', url, 'g')));
});

Pebble.addEventListener("webviewclosed", function(e) {
  if ((typeof e.response === 'string') && (e.response.length > 0)) {
    config = JSON.parse(decodeURIComponent(e.response));
    //console.log("got options = " + JSON.stringify(config));

    window.localStorage.setItem('pebble-my-data', e.response);

    fetch_data();
  }
});
