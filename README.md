# Pebble My Data App

Pebble watches application to show only your own data, prepared on your own server.
Sources available on [github](https://github.com/bahbka/pebble-my-data).
Inspired by [Pebble Cards](http://keanulee.com/pebblecards).

[My Data at Pebble App Store](https://apps.getpebble.com/applications/53b0607c94943f8e710001e2)

[Discussion](http://forums.getpebble.com/discussion/13590/watch-app-sdk2-pebble-my-data-shows-your-data-json-prepared-on-your-own-server)

## Features

* Fetch JSON from custom URL, specified in settings
* No companion app required, using PebbleKit JS
* Force update with select button
* Scrollable data area
* Custom update interval, specified in JSON
* Vibrate on update if specified in JSON
* Change text font from JSON
* Black/white theme switched from JSON
* Vibrate on bluetooth connection loss
* Watches battery charge status

## Screenshots
![pebble screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble_screenshot1.png)
![pebble screenshot 2](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble_screenshot2.png)
![pebble screenshot 6](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble_screenshot6.png)
![pebble screenshot 4](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble_screenshot4.png)
![pebble screenshot 7](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble_screenshot7.png)
![android screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/android_screenshot1_small.png)

## JSON

JSON output example (some fields are optional):

    {
      "content": "Hello\nWorld!",
      "refresh": 300,
      "vibrate": 0,
      "font": 4,
      "theme": 0
    }

### Vibrate

- 0 - Don't vibrate
- 1 - Short vibrate
- 2 - Double vibrate
- 3 - Long vibrate

### Fonts

- 1 - FONT_KEY_GOTHIC_14
- 2 - FONT_KEY_GOTHIC_14_BOLD
- 3 - FONT_KEY_GOTHIC_18
- 4 - FONT_KEY_GOTHIC_18_BOLD
- 5 - FONT_KEY_GOTHIC_24
- 6 - FONT_KEY_GOTHIC_24_BOLD
- 7 - FONT_KEY_GOTHIC_28
- 8 - FONT_KEY_GOTHIC_28_BOLD

### Theme

- 0 - Black
- 1 - White

## Bugs

Sometime after install JS app fails to start, issue related Pebble App. Force stop Pebble App and start it again.
