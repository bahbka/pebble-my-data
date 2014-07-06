# Pebble My Data App

Pebble watches application to show only your own data, prepared on your own server.
Sources available on [github](https://github.com/bahbka/pebble-my-data).
Inspired by [Pebble Cards](http://keanulee.com/pebblecards).

[My Data at Pebble App Store](https://apps.getpebble.com/applications/53b0607c94943f8e710001e2)

[Discussion at Pebble forums](http://forums.getpebble.com/discussion/13590/watch-app-sdk2-pebble-my-data-shows-your-data-json-prepared-on-your-own-server)

## Features

* Fetch JSON from custom URL, specified in settings
* No companion app required, using PebbleKit JS
* Force update with select button
* Append short=1 or long=1 GET param on short/long select button update
* Append coordinates to URL (configurable)
* Scrollable data area
* Custom update interval, specified in JSON
* Vibrate on update if specified in JSON
* Change text font from JSON
* Black/white theme switched from JSON
* Turn on light from JSON
* Blink content from JSON
* Scroll-up content after update from JSON
* Vibrate on bluetooth connection loss (configurable)
* Watches battery charge status
* Digital clock (12h/24h support), seconds dots blinking (configurable)

## Changelog

### 2.0.3

- Append coordinates to URL (configurable)
- Digital clock font, AM/PM support
- Seconds dots blinking (configurable)
- Configurable vibration on bluetooth loss
- Turn on light (value in JSON)
- Blink content (value in JSON)
- Scroll-up content after update (value in JSON)
- Improved configuration page
- Some minor fixes

### 1.1.0

- Append short=1 or long=1 GET param to URL on short/long select button update 

### 1.0.0

- Initial release

## Screenshots
![pebble screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-18-15.png)
![pebble screenshot 2](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-19-33.png)
![pebble screenshot 3](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-23-00.png)
![pebble screenshot 4](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-26-22.png)
![pebble screenshot 5](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-27-09.png)
![android screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/Screenshot_2014-07-06-18-31-03_small.png)

## JSON

JSON output example (some fields are optional):

    {
      "content": "Hello\nWorld!",
      "refresh": 300,
      "vibrate": 0,
      "font": 4,
      "theme": 0,
      "scroll": 1,
      "light": 1,
      "blink": 3
    }

GET param short=1 or long=1 added to URL on short or long select button update

### content
Your content to display. Use "\n" as CR.

### refresh
Next update delay in seconds.

### vibrate

- 0 - Don't vibrate
- 1 - Short vibrate
- 2 - Double vibrate
- 3 - Long vibrate

### font

- 1 - GOTHIC_14
- 2 - GOTHIC_14_BOLD
- 3 - GOTHIC_18
- 4 - GOTHIC_18_BOLD
- 5 - GOTHIC_24
- 6 - GOTHIC_24_BOLD
- 7 - GOTHIC_28
- 8 - GOTHIC_28_BOLD

### theme

- 0 - Black
- 1 - White

### scroll

- 0 - Keep position
- 1 - Scroll up

### light

- 0 - Do nothing
- 1 - Turn pebble light on for short time

### blink

- 1..10 - Blink content count (blinks with black/white for "count" times)

## Bugs

Sometime after install JS app fails to start, issue related Pebble App. Force stop Pebble App and start it again.
