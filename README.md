# Pebble My Data App

Pebble watches application to show only your own data, prepared on your own server.
Sources available on [github](https://github.com/bahbka/pebble-my-data).
Inspired by [Pebble Cards](http://keanulee.com/pebblecards).

[My Data at Pebble App Store](https://apps.getpebble.com/applications/53b0607c94943f8e710001e2)

[Discussion at Pebble forums](http://forums.getpebble.com/discussion/13590/watch-app-sdk2-pebble-my-data-shows-your-data-json-prepared-on-your-own-server)

## Features

* Fetch JSON from custom URL, specified in settings
* No companion app required, using PebbleKit JS
* Force update with buttons or shaking
* Append select=1/select=2 GET param on short/long select button update
* Ability to change up/down buttons behavior from JSON (scrolling or up=1|2,down=1|2 params)
* Append coordinates to URL (configurable)
* Append HTTP request header Pebble-Token (unique to device/app pair), can be used for server-side device identification
* Authentication (see documentation)
* Scrollable data area
* Custom update interval, specified in JSON
* Vibrate on update if specified in JSON
* Change text font from JSON
* Black/white theme switched from JSON
* Turn on light from JSON
* Blink content from JSON
* Define scroll offset as percentage after update from JSON
* Vibrate on bluetooth connection loss (configurable)
* Watches battery charge status
* Digital clock (12h/24h support), seconds dots blinking (configurable)

## Changelog

### 2.3.5
- Reduced GPS cache lifetime (from 30 mins to 10 mins).

### 2.3.4
- Workaround for APP_MSG_INTERNAL_ERROR (request last response after 0.1s if occur)

### 2.3.3
- Extract fields from any level of JSON (useful with [KimonoLabs API](https://www.kimonolabs.com)); multiple content fields will be concatenated with '\n\n'; other fields will be converted to integer, first copy will be used
- Don't schedule update if another one already in progress
- Keeps update type on retries when update failed

### 2.3.2
- Update with shake function (append shake=1 GET param while update, configurable)
- Changed scroll param behavior, now used to define scroll offset as percentage
- Truncate content if too big

### 2.2.0
- Authentication (see documentation)

### 2.1.2
- Ability to change up/down buttons behavior from JSON (scrolling or up=1|2,down=1|2 params)
- Added HTTP request header Pebble-Token (unique to device/app pair), can be used for server-side device identification
- **WARNING:** Changed short=1/long=1 params to select=1/select=2 (sorry for this)

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

- Append short=1 or long=1 GET param to URL on short/long select button update (changed to select=1/select=2 in 2.1.2)

### 1.0.0

- Initial release

## Screenshots
![pebble screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-18-15.png)
![pebble screenshot 2](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-19-33.png)
![pebble screenshot 3](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-23-00.png)
![pebble screenshot 4](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-26-22.png)
![pebble screenshot 5](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/pebble-screenshot_2014-07-06_18-27-09.png)
[![android screenshot 1](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/Screenshot_2014-07-06-18-31-03_small.png)](https://raw.githubusercontent.com/bahbka/pebble-my-data/master/stuff/screenshots/Screenshot_2014-07-06-18-31-03.png)

## JSON

JSON output example (some fields are optional):

    {
      "content": "Hello\nWorld!",
      "refresh": 300,
      "vibrate": 0,
      "font": 4,
      "theme": 0,
      "scroll": 33,
      "light": 1,
      "blink": 3,
      "updown": 1,
      "auth": "salt"
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
Scroll content to offset (as percentage 0..100).
If param not defined or >100 - position will be kept.

### light

- 0 - Do nothing
- 1 - Turn pebble light on for short time

### blink

- 1..10 - Blink content count (blinks with black/white for "count" times)

### updown
- 0 use up/down buttons for scrolling
- 1 use up/down buttons for update, appending up=1|2/down=1|2 params (1=short/2=long)

### auth
Salt for Pebble-Auth hash (see below)

## Auth

Authentication algorithm example (reinvent the wheel):
  1. -> Pebble makes HTTP request with Pebble-Token header (Pebble App Token by default, unique to device/app pair, can be changed at configuration page, clear to restore default)
  2. <- Server answers with JSON like { ..., "content": "logging in...", "refresh": 5, "auth": "randomsalt", ... }
  3.    Pebble calculates MD5(MD5(password)+"randomsalt"), saves it as auth token and uses as Pebble-Auth HTTP request header in future requests.
  4. -> Pebble makes HTTP request after 5 seconds with Pebble-Token header and with Pebble-Auth header (calculated and stored in previous step)
  5.    Server checks Pebble-Token and Pebble-Auth headers if data equal data in database (Pebble-Token <=> login, calculate MD5(password_md5_db+"randomsalt"))
  6. <- Server answers with private content (seems your need https for more security), or some error if auth failed; auth field in JSON not needed anymore, until you desire to regenerate auth token with new salt (paranoid mode) or to clear Pebble-Auth header

To clear Pebble-Auth header, send { ..., "auth": "", ...} (eg logout).

## Bugs

Sometime after install JS app fails to start, issue related Pebble App. Force stop Pebble App and start it again.
