/* -*-coding: utf-8 -*-
vim: sw=2 ts=2 expandtab ai

*********************************
* Pebble My Data App       *
* by bahbka <bahbka@bahbka.com> *
*********************************/

#include "pebble.h"

// define layers, bitmaps and other stuff
static Window *window;

static TextLayer *text_date_layer;
static TextLayer *text_info_layer;

static ScrollLayer *scroll_layer;

static Layer *line_layer;
static Layer *wbatt_level_layer;

static BitmapLayer *pos_layer[5];
static GBitmap *digits_bitmap[16];

static BitmapLayer *wbatt_icon_layer;
static GBitmap *wbatt_icon_bitmap = NULL;
static GBitmap *wbatt_charge_icon_bitmap = NULL;

static BitmapLayer *no_phone_icon_layer;
static GBitmap *no_phone_icon_bitmap = NULL;

static BitmapLayer *update_icon_layer;
static GBitmap *update_icon_bitmap = NULL;
static GBitmap *update_error_icon_bitmap = NULL;

static GBitmap *empty_icon_bitmap = NULL;

static AppTimer *timer = NULL;
static AppTimer *timeout_timer = NULL;
static AppTimer *blink_timer = NULL;

static uint8_t theme = 255;

static uint8_t blink_theme_save;
static uint8_t blink_count;

bool config_vibrate = true;
bool config_seconds = false;
bool config_shake = false;

bool updown = false;

bool update_in_progress = false;

#define DEFAULT_REFRESH 300*1000
#define RETRY_DELAY 60*1000
#define IN_RETRY_DELAY 100
#define REQUEST_TIMEOUT 30*1000

#define BLINK_INTERVAL 500
#define BLINK_MAX 20

#define CONTENT_SIZE 1024

#define DONT_SCROLL 255

static char content[CONTENT_SIZE];

enum { // AppMessage keys
  KEY_MSG_TYPE,
  KEY_CONTENT,
  KEY_REFRESH,
  KEY_VIBRATE,
  KEY_FONT,
  KEY_THEME,
  KEY_SCROLL,
  KEY_LIGHT,
  KEY_BLINK,
  KEY_UPDOWN,
  KEY_CONFIG_LOCATION,
  KEY_CONFIG_VIBRATE,
  KEY_CONFIG_SECONDS,
  KEY_CONFIG_SHAKE
};

enum { // msg type
  MSG_PERIODIC_UPDATE,
  MSG_SELECT_SHORT_PRESS_UPDATE,
  MSG_SELECT_LONG_PRESS_UPDATE,
  MSG_UP_SHORT_PRESS_UPDATE,
  MSG_UP_LONG_PRESS_UPDATE,
  MSG_DOWN_SHORT_PRESS_UPDATE,
  MSG_DOWN_LONG_PRESS_UPDATE,
  MSG_SHAKE_UPDATE,
  MSG_JSON_RESPONSE,
  MSG_CONFIG,
  MSG_ERROR,
  MSG_IN_RETRY
};

enum { // themes
  THEME_BLACK,
  THEME_WHITE
};

static uint8_t update_type = MSG_PERIODIC_UPDATE;

// protos
static void fill_layer(Layer *layer, GContext* ctx);

static void request_update();
static void schedule_update(uint32_t delay, uint8_t type);
static void request_timeout();

static void draw_digit(BitmapLayer *position, char digit);

static void handle_timer_tick(struct tm *tick_time, TimeUnits units_changed);
static void handle_battery(BatteryChargeState charge_state);
static void handle_bluetooth(bool connected);
static void handle_shake(AccelAxisType axis, int32_t direction);

static void change_info_theme(uint8_t theme);
static void blink_info();
static void update_info_layer(char *content, uint8_t font, uint8_t scroll_offset, bool new_updown);
static void change_theme(uint8_t t);

static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);
static void click_config_provider_updown(void *context);

// fill layer used for drawing line and battery charge
static void fill_layer(Layer *layer, GContext* ctx) {
  if (theme != THEME_BLACK) {
    graphics_context_set_fill_color(ctx, GColorBlack);
  } else {
    graphics_context_set_fill_color(ctx, GColorWhite);
  }
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

// request data update thru AppMessage
static void request_update() {
  // show update icon
  bitmap_layer_set_bitmap(update_icon_layer, update_icon_bitmap);
  update_in_progress = true;

  // prepare request
  DictionaryIterator *send_message;
  app_message_outbox_begin(&send_message);

  Tuplet value = TupletInteger(KEY_REFRESH, update_type);
  dict_write_tuplet(send_message, &value);

  app_message_outbox_send();

  // set timeout timer
  timeout_timer = app_timer_register(REQUEST_TIMEOUT, request_timeout, NULL);
}

// schedule request_update
static void schedule_update(uint32_t delay, uint8_t type) {
  if (!update_in_progress) {
    if (timeout_timer) {
      app_timer_cancel(timeout_timer);
    }

    if (timer) {
      app_timer_cancel(timer);
    }

    update_type = type; // can't win callbacks data :( use global variable
    timeout_timer = app_timer_register(delay, request_update, NULL);
  }
}

// reschedule update if data waiting timed out, also set update_error icon
static void request_timeout() {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  update_in_progress = false;
  schedule_update(RETRY_DELAY, update_type);
}

// draw digit on position layer
static void draw_digit(BitmapLayer *position, char digit) {
  if (digit >= 48 && digit <= 63) {
    bitmap_layer_set_bitmap(position, digits_bitmap[digit - 48]);
  } else {
    bitmap_layer_set_bitmap(position, digits_bitmap[11]); // space
  }
}

// handle minute tick to update time/date
static void handle_timer_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed == SECOND_UNIT) {
    if (tick_time->tm_sec % 2 == 0) {
      draw_digit(pos_layer[2], ':');
    } else {
      draw_digit(pos_layer[2], ' ');
    }

  } else {
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char date_text[] = "Mon Jan  1";
    static char ampm[] = "AM";

    char *time_format;

    if (!tick_time) {
      time_t now = time(NULL);
      tick_time = localtime(&now);
    }

    strftime(date_text, sizeof(date_text), "%a %b %e", tick_time);
    text_layer_set_text(text_date_layer, date_text);

    if (clock_is_24h_style()) {
      time_format = "%R";
    } else {
      time_format = "%I:%M";
    }

    strftime(time_text, sizeof(time_text), time_format, tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style()) {
      strftime(ampm, sizeof(ampm), "%p", tick_time);
      if (time_text[0] == '0') {
        if (strcmp(ampm, "AM") == 0) {
          time_text[0] = '0' + 12; // 0 am
        } else {
          time_text[0] = '0' + 13; // 0 pm
        }
      } else {
        if (strcmp(ampm, "AM") == 0) {
          time_text[0] = '0' + 14; // 1 am
        } else {
          time_text[0] = '0' + 15; // 1 pm
        }
      }
    }

    for (int i = 0; i < 5; i++) {
      draw_digit(pos_layer[i], time_text[i]);
    }
  }
}

// handle battery status changes
static void handle_battery(BatteryChargeState charge_state) {
  // change bitmap for charging/non-charging states
  if (charge_state.is_charging) {
    bitmap_layer_set_bitmap(wbatt_icon_layer, wbatt_charge_icon_bitmap);
  } else {
    bitmap_layer_set_bitmap(wbatt_icon_layer, wbatt_icon_bitmap);
  }

  // resize battery level layer according battery charge
  layer_set_frame(wbatt_level_layer, GRect(11, 5, charge_state.charge_percent / 10, 4));
}

// handle bluetooth status changes
static void handle_bluetooth(bool connected) {
  if (connected) {
    bitmap_layer_set_bitmap(no_phone_icon_layer, empty_icon_bitmap); // hide update icon (layer_set_hidden function sometimes works strange)
    schedule_update(5 * 1000, MSG_PERIODIC_UPDATE); // schedule update when connection made
  } else {
    bitmap_layer_set_bitmap(no_phone_icon_layer, no_phone_icon_bitmap);
    if (config_vibrate) {
      vibes_long_pulse(); // achtung! phone lost!
    }
  }
}

static void handle_shake(AccelAxisType axis, int32_t direction) {
  schedule_update(0, MSG_SHAKE_UPDATE);
}

// update data in main layer (content, font)
static void update_info_layer(char *content, uint8_t font, uint8_t scroll_offset, bool new_updown) {
  // change font
  switch (font) {
    case 1:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
      break;
    case 2:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
      break;
    case 3:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
      break;
    case 4:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      break;
    case 5:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
      break;
    case 6:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      break;
    case 7:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
      break;
    case 8:
      text_layer_set_font(text_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
      break;
    default:
      break;
  }

  // change up/down buttons behavior
  if (new_updown != updown) {
    if (new_updown) {
      scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
        .click_config_provider = &click_config_provider_updown
      });
      scroll_layer_set_click_config_onto_window(scroll_layer, window);
    } else {
      scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
        .click_config_provider = &click_config_provider
      });
      scroll_layer_set_click_config_onto_window(scroll_layer, window);
    }
    updown = new_updown;
  }

  text_layer_set_text(text_info_layer, content); // set text
  GSize max_size = text_layer_get_content_size(text_info_layer);
  max_size.w = 144;
  if (max_size.h < (168 - 31)) {
    max_size.h = (168 - 31);
  } else {
    max_size.h += 4;
  }
  text_layer_set_size(text_info_layer, max_size); // resize layer
  scroll_layer_set_content_size(scroll_layer, GSize(144, max_size.h)); // resize scroll layer

  if (scroll_offset != DONT_SCROLL) {
    GPoint offset;
    offset.x = 0;

    offset.y = -(int)((float)max_size.h / 100.0 * scroll_offset);

    scroll_layer_set_content_offset(scroll_layer, offset, true); // scroll to beginning with animation
  }
}

// change theme (black/white), do things only when theme realy changed
static void change_theme(uint8_t t) {
  if ((t == THEME_BLACK || t == THEME_WHITE) && (t != theme)) {
    theme = t;

    uint8_t bg = GColorBlack;
    uint8_t fg = GColorWhite;

    if (theme == THEME_WHITE) {
      bg = GColorWhite;
      fg = GColorBlack;
    }

    // set foreground, background colors and update window, layers
    window_set_background_color(window, bg);
    text_layer_set_text_color(text_date_layer, fg);

    text_layer_set_background_color(text_info_layer, bg);
    text_layer_set_text_color(text_info_layer, fg);

    layer_mark_dirty(wbatt_level_layer);
    layer_mark_dirty(line_layer);

    // destroy and recreate bitmaps
    gbitmap_destroy(wbatt_icon_bitmap);
    gbitmap_destroy(wbatt_charge_icon_bitmap);
    gbitmap_destroy(no_phone_icon_bitmap);
    gbitmap_destroy(update_icon_bitmap);
    gbitmap_destroy(update_error_icon_bitmap);
    gbitmap_destroy(empty_icon_bitmap);


    for (int i = 0; i < 16; i++)
      gbitmap_destroy(digits_bitmap[i]);

    if (theme == THEME_BLACK) {
      wbatt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_WHITE);
      wbatt_charge_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_CHARGE_WHITE);
      no_phone_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_PHONE_WHITE);
      update_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_WHITE);
      update_error_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_ERROR_WHITE);
      empty_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_EMPTY_WHITE);

      digits_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0_WHITE);
      digits_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1_WHITE);
      digits_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_2_WHITE);
      digits_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_3_WHITE);
      digits_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_4_WHITE);
      digits_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_5_WHITE);
      digits_bitmap[6] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_6_WHITE);
      digits_bitmap[7] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_7_WHITE);
      digits_bitmap[8] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_8_WHITE);
      digits_bitmap[9] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_9_WHITE);
      digits_bitmap[10] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_DOTS_WHITE);
      digits_bitmap[11] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_DOTS_SPACE_WHITE);
      digits_bitmap[12] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0AM_WHITE);
      digits_bitmap[13] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0PM_WHITE);
      digits_bitmap[14] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1AM_WHITE);
      digits_bitmap[15] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1PM_WHITE);
    } else {
      wbatt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_BLACK);
      wbatt_charge_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_CHARGE_BLACK);
      no_phone_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_PHONE_BLACK);
      update_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_BLACK);
      update_error_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_ERROR_BLACK);
      empty_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_EMPTY_BLACK);

      digits_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0_BLACK);
      digits_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1_BLACK);
      digits_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_2_BLACK);
      digits_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_3_BLACK);
      digits_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_4_BLACK);
      digits_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_5_BLACK);
      digits_bitmap[6] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_6_BLACK);
      digits_bitmap[7] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_7_BLACK);
      digits_bitmap[8] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_8_BLACK);
      digits_bitmap[9] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_9_BLACK);
      digits_bitmap[10] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_DOTS_BLACK);
      digits_bitmap[11] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_DOTS_SPACE_BLACK);
      digits_bitmap[12] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0AM_BLACK);
      digits_bitmap[13] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_0PM_BLACK);
      digits_bitmap[14] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1AM_BLACK);
      digits_bitmap[15] = gbitmap_create_with_resource(RESOURCE_ID_DIGIT_1PM_BLACK);
    }

    handle_timer_tick(NULL, MINUTE_UNIT); // update time

    // update bitmaps layers
    layer_mark_dirty(bitmap_layer_get_layer(wbatt_icon_layer));
    layer_mark_dirty(bitmap_layer_get_layer(no_phone_icon_layer));
    layer_mark_dirty(bitmap_layer_get_layer(update_icon_layer));
  }
}

// change only info layer theme (for blinking)
static void change_info_theme(uint8_t theme) {
    if (theme == THEME_BLACK) {
      text_layer_set_background_color(text_info_layer, GColorBlack);
      text_layer_set_text_color(text_info_layer, GColorWhite);
    } else {
      text_layer_set_background_color(text_info_layer, GColorWhite);
      text_layer_set_text_color(text_info_layer, GColorBlack);
    }
}

// blink info theme
static void blink_info() {
  if (blink_timer) {
    app_timer_cancel(blink_timer);
  }

  if (blink_count > 0) {
    change_info_theme((blink_count + !blink_theme_save) % 2);

    blink_count--;
    blink_timer = app_timer_register(BLINK_INTERVAL, blink_info, NULL);

  } else {
    change_info_theme(blink_theme_save);
  }
}

// useful in the future probably
void out_sent_handler(DictionaryIterator *sent, void *context) {
}

// reschedule update if request_update sending failed, also set update_error icon
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  update_in_progress = false;
  schedule_update(RETRY_DELAY, update_type);
}

// reschedule update if received data dropped, also set update_error icon
void in_dropped_handler(AppMessageResult reason, void *context) {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  update_in_progress = false;
  schedule_update(IN_RETRY_DELAY, MSG_IN_RETRY);
}

// process received data
void in_received_handler(DictionaryIterator *received, void *context) {
  // update main layer with received content and font
  Tuple *msg_type_tuple = dict_find(received, KEY_MSG_TYPE);
  if (msg_type_tuple) {
    if(msg_type_tuple->value->uint8 == MSG_CONFIG) {
      Tuple *config_vibrate_tuple = dict_find(received, KEY_CONFIG_VIBRATE);
      if (config_vibrate_tuple) {
        if (strcmp(config_vibrate_tuple->value->cstring, "true") == 0) {
          config_vibrate = true;
        } else {
          config_vibrate = false;
        }
      }

      Tuple *config_seconds_tuple = dict_find(received, KEY_CONFIG_SECONDS);
      if (config_seconds_tuple) {
        if (strcmp(config_seconds_tuple->value->cstring, "true") == 0) {
          if (!config_seconds) {
            config_seconds = true;
            tick_timer_service_subscribe(SECOND_UNIT, handle_timer_tick);
          }
        } else {
          if (config_seconds) {
            config_seconds = false;
            tick_timer_service_subscribe(MINUTE_UNIT, handle_timer_tick);
            handle_timer_tick(NULL, MINUTE_UNIT);
          }
        }
      }

      Tuple *config_shake_tuple = dict_find(received, KEY_CONFIG_SHAKE);
      if (config_shake_tuple) {
        if (strcmp(config_shake_tuple->value->cstring, "true") == 0) {
          if (!config_shake) {
            config_shake = true;
            accel_tap_service_subscribe(handle_shake);
          }
        } else {
          if (config_shake) {
            config_shake = false;
            accel_tap_service_unsubscribe();
          }
        }
      }

      // TODO location icon?

    } else if (msg_type_tuple->value->uint8 == MSG_ERROR) {
      bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
      update_in_progress = false;
      schedule_update(RETRY_DELAY, update_type);

    } else if (msg_type_tuple->value->uint8 == MSG_JSON_RESPONSE) {
      bitmap_layer_set_bitmap(update_icon_layer, empty_icon_bitmap); // hide update icon (layer_set_hidden function sometimes works strange)
      update_in_progress = false;

      Tuple *content_tuple = dict_find(received, KEY_CONTENT);
      if (content_tuple) {
        memcpy(content, content_tuple->value->cstring, strlen(content_tuple->value->cstring) + 1);

        Tuple *scroll_offset_tuple = dict_find(received, KEY_SCROLL);
        uint8_t scroll_offset = DONT_SCROLL;
        if (scroll_offset_tuple) {
          scroll_offset = scroll_offset_tuple->value->uint8;
          if (scroll_offset > 100)
            scroll_offset = DONT_SCROLL;
        }

        Tuple *updown_tuple = dict_find(received, KEY_UPDOWN);
        bool updown_beh = false;
        if (updown_tuple && updown_tuple->value->uint8 == 1) {
          updown_beh = true;
        }

        Tuple *font_tuple = dict_find(received, KEY_FONT);
        uint8_t font = 0;
        if (font_tuple) {
          font = font_tuple->value->uint8;
        }

        update_info_layer(content, font, scroll_offset, updown_beh);
      }

      // maybe need to vibrate?
      Tuple *vibrate = dict_find(received, KEY_VIBRATE);
      if (vibrate) {
        switch(vibrate->value->uint8) {
          case 1:
            vibes_short_pulse();
            break;
          case 2:
            vibes_double_pulse();
            break;
          case 3:
            vibes_long_pulse();
            break;
          default:
            break;
        }
      }

      // or switch theme?
      Tuple *new_theme = dict_find(received, KEY_THEME);
      if (new_theme) {
        change_theme(new_theme->value->uint8);
      }

      // or turn on light?
      Tuple *light = dict_find(received, KEY_LIGHT);
      if (light && light->value->uint8 == 1) {
        light_enable_interaction();
      }

      // or blink content?
      Tuple *blink = dict_find(received, KEY_BLINK);
      if (blink) {
        if (blink->value->uint8 > 0) {
          if (blink->value->uint8 > BLINK_MAX) {
            blink_count = BLINK_MAX;
          } else {
            blink_count = blink->value->uint8 * 2;
          }

          blink_theme_save = theme;
          blink_info();
        }
      }

      // schedule next update
      uint32_t delay = DEFAULT_REFRESH;
      Tuple *refresh = dict_find(received, KEY_REFRESH);
      if (refresh) {
        delay = refresh->value->uint32 * 1000;
      }

      schedule_update(delay, MSG_PERIODIC_UPDATE);
    }
  }
}

// force update on up short click
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_UP_SHORT_PRESS_UPDATE);
  }
}

// force update on up long click
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_UP_LONG_PRESS_UPDATE);
  }
}

// force update on down short click
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_DOWN_SHORT_PRESS_UPDATE);
  }
}

// force update on down long click
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_DOWN_LONG_PRESS_UPDATE);
  }
}

// force update on select short click
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_SELECT_SHORT_PRESS_UPDATE);
  }
}

// force update on select long click
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (blink_count > 0) { // if blinking - stop it!
    blink_count = 0;
    blink_info();

  } else {
    schedule_update(0, MSG_SELECT_LONG_PRESS_UPDATE);
  }
}

// handle select button clicks
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
}

// handle select/up/down buttons clicks
static void click_config_provider_updown(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 0, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, down_long_click_handler, NULL);
}

// prepare window!
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // time layers
  // pos1
  pos_layer[0] = bitmap_layer_create(GRect(65, 2, 16, 25));
  layer_add_child(window_layer, bitmap_layer_get_layer(pos_layer[0]));
  // pos2
  pos_layer[1] = bitmap_layer_create(GRect(82, 2, 16, 25));
  layer_add_child(window_layer, bitmap_layer_get_layer(pos_layer[1]));
  // pos3
  pos_layer[2] = bitmap_layer_create(GRect(101, 2, 6, 25));
  layer_add_child(window_layer, bitmap_layer_get_layer(pos_layer[2]));
  // pos4
  pos_layer[3] = bitmap_layer_create(GRect(109, 2, 16, 25));
  layer_add_child(window_layer, bitmap_layer_get_layer(pos_layer[3]));
  // pos5
  pos_layer[4] = bitmap_layer_create(GRect(126, 2, 16, 25));
  layer_add_child(window_layer, bitmap_layer_get_layer(pos_layer[4]));

  // date layer
  text_date_layer = text_layer_create(GRect(0, 13, 142, 14));
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  // watch batt icon layer
  wbatt_icon_layer = bitmap_layer_create(GRect(1, 2, 30, 10));
  layer_add_child(window_layer, bitmap_layer_get_layer(wbatt_icon_layer));

  // watch batt level
  wbatt_level_layer = layer_create(GRect(11, 5, 0, 4));
  layer_set_update_proc(wbatt_level_layer, fill_layer);
  layer_add_child(window_layer, wbatt_level_layer);

  // no phone icon layer
  no_phone_icon_layer = bitmap_layer_create(GRect(33, 2, 10, 10));
  bitmap_layer_set_bitmap(no_phone_icon_layer, empty_icon_bitmap); // hide update icon (layer_set_hidden function sometimes works strange)
  layer_add_child(window_layer, bitmap_layer_get_layer(no_phone_icon_layer));

  // update icon layer
  update_icon_layer = bitmap_layer_create(GRect(44, 2, 10, 10));
  bitmap_layer_set_bitmap(update_icon_layer, empty_icon_bitmap); // hide update icon (layer_set_hidden function sometimes works strange)
  layer_add_child(window_layer, bitmap_layer_get_layer(update_icon_layer));

  // line layer
  GRect line_frame = GRect(0, 29, 144, 2);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, fill_layer);
  layer_add_child(window_layer, line_layer);

  // main info layer
  scroll_layer = scroll_layer_create(GRect(0, 31, 145, 168-31));
  scroll_layer_set_click_config_onto_window(scroll_layer, window);
  scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = &click_config_provider
  });

  text_info_layer = text_layer_create(GRect(0, 0, 145, 2000));
  //text_layer_set_background_color(text_info_layer, GColorClear);

  scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_info_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

  // apply black theme
  change_theme(THEME_BLACK);

  // subscribe events
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());

  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());

  tick_timer_service_subscribe(MINUTE_UNIT, handle_timer_tick);
  handle_timer_tick(NULL, MINUTE_UNIT);
}

static void window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  if (config_shake) {
    config_shake = false;
    accel_tap_service_unsubscribe();
  }

  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_info_layer);

  scroll_layer_destroy(scroll_layer);

  gbitmap_destroy(wbatt_icon_bitmap);
  gbitmap_destroy(wbatt_charge_icon_bitmap);
  gbitmap_destroy(no_phone_icon_bitmap);
  gbitmap_destroy(update_icon_bitmap);
  gbitmap_destroy(update_error_icon_bitmap);
  gbitmap_destroy(empty_icon_bitmap);

  for (int i = 0; i < 16; i++)
    gbitmap_destroy(digits_bitmap[i]);

  bitmap_layer_destroy(wbatt_icon_layer);
  bitmap_layer_destroy(no_phone_icon_layer);
  bitmap_layer_destroy(update_icon_layer);

  for (int i = 0; i < 5; i++)
    bitmap_layer_destroy(pos_layer[i]);

  layer_destroy(line_layer);
  layer_destroy(wbatt_level_layer);
}

static void init(void) {
  window = window_create();
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  // prepare AppMessage
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

  const uint32_t inbound_size = CONTENT_SIZE;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  // show window
  window_stack_push(window, false);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
