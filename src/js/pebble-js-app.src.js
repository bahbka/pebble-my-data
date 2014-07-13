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
  SHAKE_UPDATE: 7,
  JSON_RESPONSE: 8,
  CONFIG: 9,
  ERROR: 10
};

// default settings
var config = {
  "config_location": false,
  "config_vibrate": true,
  "config_seconds": true,
  "config_shake": false
};

function http_request(url) {
  var req = new XMLHttpRequest();

  req.open('GET', url, true);

  if (!config["token"]) {
    config["token"] = Pebble.getAccountToken();
  }
  req.setRequestHeader('Pebble-Token', config["token"]);

  var auth = window.localStorage.getItem('pebble-my-data-auth');
  if (auth) {
    req.setRequestHeader('Pebble-Auth', auth);
  }

  req.onload = function(e) {

    if (req.readyState == 4) {
      if(req.status == 200) {
        try {
          var response = JSON.parse(req.responseText);
          //console.log("success: " + JSON.stringify(response));
          response["msg_type"] = MSG.JSON_RESPONSE;

          //TODO truncate content bytes= bytes.substring(0, bytes.length-1);

          Pebble.sendAppMessage(response);

          if (response["auth"] != null) {
            if (response["auth"] == "") {
              window.localStorage.removeItem('pebble-my-data-auth');
            } else if (config["password"]) {
              window.localStorage.setItem('pebble-my-data-auth', MD5(MD5(config["password"]) + response["auth"]));
            }
          }

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
      } else if (e.payload["refresh"] == MSG.SHAKE_UPDATE) {
        url = url + s + "shake=1";
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
  if (!config["token"]) {
    config["token"] = Pebble.getAccountToken();
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

var MD5 = function (string) {
  function RotateLeft(lValue, iShiftBits) {
    return (lValue<<iShiftBits) | (lValue>>>(32-iShiftBits));
  }

  function AddUnsigned(lX,lY) {
    var lX4,lY4,lX8,lY8,lResult;
    lX8 = (lX & 0x80000000);
    lY8 = (lY & 0x80000000);
    lX4 = (lX & 0x40000000);
    lY4 = (lY & 0x40000000);
    lResult = (lX & 0x3FFFFFFF)+(lY & 0x3FFFFFFF);
    if (lX4 & lY4) {
      return (lResult ^ 0x80000000 ^ lX8 ^ lY8);
    }
    if (lX4 | lY4) {
      if (lResult & 0x40000000) {
        return (lResult ^ 0xC0000000 ^ lX8 ^ lY8);
      } else {
        return (lResult ^ 0x40000000 ^ lX8 ^ lY8);
      }
    } else {
      return (lResult ^ lX8 ^ lY8);
    }
  }

  function F(x,y,z) { return (x & y) | ((~x) & z); }
  function G(x,y,z) { return (x & z) | (y & (~z)); }
  function H(x,y,z) { return (x ^ y ^ z); }
  function I(x,y,z) { return (y ^ (x | (~z))); }

  function FF(a,b,c,d,x,s,ac) {
    a = AddUnsigned(a, AddUnsigned(AddUnsigned(F(b, c, d), x), ac));
    return AddUnsigned(RotateLeft(a, s), b);
  };

  function GG(a,b,c,d,x,s,ac) {
    a = AddUnsigned(a, AddUnsigned(AddUnsigned(G(b, c, d), x), ac));
    return AddUnsigned(RotateLeft(a, s), b);
  };

  function HH(a,b,c,d,x,s,ac) {
    a = AddUnsigned(a, AddUnsigned(AddUnsigned(H(b, c, d), x), ac));
    return AddUnsigned(RotateLeft(a, s), b);
  };

  function II(a,b,c,d,x,s,ac) {
    a = AddUnsigned(a, AddUnsigned(AddUnsigned(I(b, c, d), x), ac));
    return AddUnsigned(RotateLeft(a, s), b);
  };

  function ConvertToWordArray(string) {
    var lWordCount;
    var lMessageLength = string.length;
    var lNumberOfWords_temp1=lMessageLength + 8;
    var lNumberOfWords_temp2=(lNumberOfWords_temp1-(lNumberOfWords_temp1 % 64))/64;
    var lNumberOfWords = (lNumberOfWords_temp2+1)*16;
    var lWordArray=Array(lNumberOfWords-1);
    var lBytePosition = 0;
    var lByteCount = 0;
    while ( lByteCount < lMessageLength ) {
      lWordCount = (lByteCount-(lByteCount % 4))/4;
      lBytePosition = (lByteCount % 4)*8;
      lWordArray[lWordCount] = (lWordArray[lWordCount] | (string.charCodeAt(lByteCount)<<lBytePosition));
      lByteCount++;
    }
    lWordCount = (lByteCount-(lByteCount % 4))/4;
    lBytePosition = (lByteCount % 4)*8;
    lWordArray[lWordCount] = lWordArray[lWordCount] | (0x80<<lBytePosition);
    lWordArray[lNumberOfWords-2] = lMessageLength<<3;
    lWordArray[lNumberOfWords-1] = lMessageLength>>>29;
    return lWordArray;
  };

  function WordToHex(lValue) {
    var WordToHexValue="",WordToHexValue_temp="",lByte,lCount;
    for (lCount = 0;lCount<=3;lCount++) {
      lByte = (lValue>>>(lCount*8)) & 255;
      WordToHexValue_temp = "0" + lByte.toString(16);
      WordToHexValue = WordToHexValue + WordToHexValue_temp.substr(WordToHexValue_temp.length-2,2);
    }
    return WordToHexValue;
  };

  function Utf8Encode(string) {
    string = string.replace(/\r\n/g,"\n");
    var utftext = "";

    for (var n = 0; n < string.length; n++) {

      var c = string.charCodeAt(n);

      if (c < 128) {
        utftext += String.fromCharCode(c);
      }
      else if((c > 127) && (c < 2048)) {
        utftext += String.fromCharCode((c >> 6) | 192);
        utftext += String.fromCharCode((c & 63) | 128);
      }
      else {
        utftext += String.fromCharCode((c >> 12) | 224);
        utftext += String.fromCharCode(((c >> 6) & 63) | 128);
        utftext += String.fromCharCode((c & 63) | 128);
      }

    }

    return utftext;
  };

  var x=Array();
  var k,AA,BB,CC,DD,a,b,c,d;
  var S11=7, S12=12, S13=17, S14=22;
  var S21=5, S22=9 , S23=14, S24=20;
  var S31=4, S32=11, S33=16, S34=23;
  var S41=6, S42=10, S43=15, S44=21;

  string = Utf8Encode(string);

  x = ConvertToWordArray(string);

  a = 0x67452301; b = 0xEFCDAB89; c = 0x98BADCFE; d = 0x10325476;

  for (k=0;k<x.length;k+=16) {
    AA=a; BB=b; CC=c; DD=d;
    a=FF(a,b,c,d,x[k+0], S11,0xD76AA478);
    d=FF(d,a,b,c,x[k+1], S12,0xE8C7B756);
    c=FF(c,d,a,b,x[k+2], S13,0x242070DB);
    b=FF(b,c,d,a,x[k+3], S14,0xC1BDCEEE);
    a=FF(a,b,c,d,x[k+4], S11,0xF57C0FAF);
    d=FF(d,a,b,c,x[k+5], S12,0x4787C62A);
    c=FF(c,d,a,b,x[k+6], S13,0xA8304613);
    b=FF(b,c,d,a,x[k+7], S14,0xFD469501);
    a=FF(a,b,c,d,x[k+8], S11,0x698098D8);
    d=FF(d,a,b,c,x[k+9], S12,0x8B44F7AF);
    c=FF(c,d,a,b,x[k+10],S13,0xFFFF5BB1);
    b=FF(b,c,d,a,x[k+11],S14,0x895CD7BE);
    a=FF(a,b,c,d,x[k+12],S11,0x6B901122);
    d=FF(d,a,b,c,x[k+13],S12,0xFD987193);
    c=FF(c,d,a,b,x[k+14],S13,0xA679438E);
    b=FF(b,c,d,a,x[k+15],S14,0x49B40821);
    a=GG(a,b,c,d,x[k+1], S21,0xF61E2562);
    d=GG(d,a,b,c,x[k+6], S22,0xC040B340);
    c=GG(c,d,a,b,x[k+11],S23,0x265E5A51);
    b=GG(b,c,d,a,x[k+0], S24,0xE9B6C7AA);
    a=GG(a,b,c,d,x[k+5], S21,0xD62F105D);
    d=GG(d,a,b,c,x[k+10],S22,0x2441453);
    c=GG(c,d,a,b,x[k+15],S23,0xD8A1E681);
    b=GG(b,c,d,a,x[k+4], S24,0xE7D3FBC8);
    a=GG(a,b,c,d,x[k+9], S21,0x21E1CDE6);
    d=GG(d,a,b,c,x[k+14],S22,0xC33707D6);
    c=GG(c,d,a,b,x[k+3], S23,0xF4D50D87);
    b=GG(b,c,d,a,x[k+8], S24,0x455A14ED);
    a=GG(a,b,c,d,x[k+13],S21,0xA9E3E905);
    d=GG(d,a,b,c,x[k+2], S22,0xFCEFA3F8);
    c=GG(c,d,a,b,x[k+7], S23,0x676F02D9);
    b=GG(b,c,d,a,x[k+12],S24,0x8D2A4C8A);
    a=HH(a,b,c,d,x[k+5], S31,0xFFFA3942);
    d=HH(d,a,b,c,x[k+8], S32,0x8771F681);
    c=HH(c,d,a,b,x[k+11],S33,0x6D9D6122);
    b=HH(b,c,d,a,x[k+14],S34,0xFDE5380C);
    a=HH(a,b,c,d,x[k+1], S31,0xA4BEEA44);
    d=HH(d,a,b,c,x[k+4], S32,0x4BDECFA9);
    c=HH(c,d,a,b,x[k+7], S33,0xF6BB4B60);
    b=HH(b,c,d,a,x[k+10],S34,0xBEBFBC70);
    a=HH(a,b,c,d,x[k+13],S31,0x289B7EC6);
    d=HH(d,a,b,c,x[k+0], S32,0xEAA127FA);
    c=HH(c,d,a,b,x[k+3], S33,0xD4EF3085);
    b=HH(b,c,d,a,x[k+6], S34,0x4881D05);
    a=HH(a,b,c,d,x[k+9], S31,0xD9D4D039);
    d=HH(d,a,b,c,x[k+12],S32,0xE6DB99E5);
    c=HH(c,d,a,b,x[k+15],S33,0x1FA27CF8);
    b=HH(b,c,d,a,x[k+2], S34,0xC4AC5665);
    a=II(a,b,c,d,x[k+0], S41,0xF4292244);
    d=II(d,a,b,c,x[k+7], S42,0x432AFF97);
    c=II(c,d,a,b,x[k+14],S43,0xAB9423A7);
    b=II(b,c,d,a,x[k+5], S44,0xFC93A039);
    a=II(a,b,c,d,x[k+12],S41,0x655B59C3);
    d=II(d,a,b,c,x[k+3], S42,0x8F0CCC92);
    c=II(c,d,a,b,x[k+10],S43,0xFFEFF47D);
    b=II(b,c,d,a,x[k+1], S44,0x85845DD1);
    a=II(a,b,c,d,x[k+8], S41,0x6FA87E4F);
    d=II(d,a,b,c,x[k+15],S42,0xFE2CE6E0);
    c=II(c,d,a,b,x[k+6], S43,0xA3014314);
    b=II(b,c,d,a,x[k+13],S44,0x4E0811A1);
    a=II(a,b,c,d,x[k+4], S41,0xF7537E82);
    d=II(d,a,b,c,x[k+11],S42,0xBD3AF235);
    c=II(c,d,a,b,x[k+2], S43,0x2AD7D2BB);
    b=II(b,c,d,a,x[k+9], S44,0xEB86D391);
    a=AddUnsigned(a,AA);
    b=AddUnsigned(b,BB);
    c=AddUnsigned(c,CC);
    d=AddUnsigned(d,DD);
  }

  var temp = WordToHex(a)+WordToHex(b)+WordToHex(c)+WordToHex(d);

  return temp.toLowerCase();
}
