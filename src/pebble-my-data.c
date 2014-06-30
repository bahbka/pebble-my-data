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
static TextLayer *text_time_layer;
static TextLayer *text_info_layer;

static ScrollLayer *scroll_layer;

static Layer *line_layer;
static Layer *wbatt_level_layer;

static BitmapLayer *wbatt_icon_layer;
static GBitmap *wbatt_icon_bitmap = NULL;
static GBitmap *wbatt_charge_icon_bitmap = NULL;

static BitmapLayer *no_phone_icon_layer;
static GBitmap *no_phone_icon_bitmap = NULL;

static BitmapLayer *update_icon_layer;
static GBitmap *update_icon_bitmap = NULL;
static GBitmap *update_error_icon_bitmap = NULL;

static AppTimer *timer = NULL;
static AppTimer *timeout_timer = NULL;

static uint8_t theme = 255;

#define DEFAULT_REFRESH 300*1000
#define RETRY_DELAY 60*1000
#define REQUEST_TIMEOUT 30*1000

enum { // AppMessage keys
  KEY_CONTENT,
  KEY_REFRESH,
  KEY_VIBRATE,
  KEY_FONT,
  KEY_THEME
};

enum { // update types
  PERIODIC_UPDATE,
  SHORT_PRESS_UPDATE,
  LONG_PRESS_UPDATE
};

enum { // themes
  THEME_BLACK,
  THEME_WHITE
};

static uint8_t update_type = PERIODIC_UPDATE;

// fill layer used for drawing line and battery charge
static void fill_layer(Layer *layer, GContext* ctx) {
  if (theme != THEME_BLACK) {
    graphics_context_set_fill_color(ctx, GColorBlack);
  } else {
    graphics_context_set_fill_color(ctx, GColorWhite);
  }
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void request_timeout(); // proto

// request data update thru AppMessage
static void request_update() {
  // show update icon
  bitmap_layer_set_bitmap(update_icon_layer, update_icon_bitmap);
  layer_set_hidden(bitmap_layer_get_layer(update_icon_layer), false);

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
  if (timeout_timer) {
    app_timer_cancel(timeout_timer);
  }

  if (timer) {
    app_timer_cancel(timer);
  }

  update_type = type; // can't win callbacks data :( use global variable
  timeout_timer = app_timer_register(delay, request_update, NULL);
}

// reschedule update if data waiting timed out, also set update_error icon
static void request_timeout() {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  schedule_update(RETRY_DELAY, PERIODIC_UPDATE);
}

// handle minute tick to update time/date (got from Simplicity watchface without modification)
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  if (!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
  }

  // TODO: Only update the date when it's changed.
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
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
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
    layer_set_hidden(bitmap_layer_get_layer(no_phone_icon_layer), true);
    schedule_update(5 * 1000, PERIODIC_UPDATE); // schedule update when connection made
  } else {
    layer_set_hidden(bitmap_layer_get_layer(no_phone_icon_layer), false);
    vibes_long_pulse(); // achtung! phone lost!
  }
}

// update data in main layer (content, font)
static void update_info_layer(char *content, uint8_t font) {
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

  text_layer_set_text(text_info_layer, content); // set text
  GSize max_size = text_layer_get_content_size(text_info_layer);
  max_size.h += 4;
  text_layer_set_size(text_info_layer, max_size); // resize layer
  scroll_layer_set_content_size(scroll_layer, GSize(144, max_size.h)); // resize scroll layer
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
    text_layer_set_text_color(text_time_layer, fg);
    text_layer_set_text_color(text_date_layer, fg);
    text_layer_set_text_color(text_info_layer, fg);

    layer_mark_dirty(wbatt_level_layer);
    layer_mark_dirty(line_layer);

    // destroy and recreate bitmaps
    if (wbatt_icon_bitmap) {
      gbitmap_destroy(wbatt_icon_bitmap);
    }
    if (wbatt_charge_icon_bitmap) {
      gbitmap_destroy(wbatt_charge_icon_bitmap);
    }
    if (no_phone_icon_bitmap) {
      gbitmap_destroy(no_phone_icon_bitmap);
    }
    if (update_icon_bitmap) {
      gbitmap_destroy(update_icon_bitmap);
    }
    if (update_error_icon_bitmap) {
      gbitmap_destroy(update_error_icon_bitmap);
    }

    if (theme == THEME_BLACK) {
      wbatt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_WHITE);
      wbatt_charge_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_CHARGE_WHITE);
      no_phone_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_PHONE_WHITE);
      update_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_WHITE);
      update_error_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_ERROR_WHITE);
    } else {
      wbatt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_BLACK);
      wbatt_charge_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WBATT_CHARGE_BLACK);
      no_phone_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_PHONE_BLACK);
      update_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_BLACK);
      update_error_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UPDATE_ERROR_BLACK);
    }

    bitmap_layer_set_bitmap(no_phone_icon_layer, no_phone_icon_bitmap); // single place to set no_phone bitmap to layer

    // update bitmaps layers
    layer_mark_dirty(bitmap_layer_get_layer(wbatt_icon_layer));
    layer_mark_dirty(bitmap_layer_get_layer(no_phone_icon_layer));
    layer_mark_dirty(bitmap_layer_get_layer(update_icon_layer));
  }
}

// useful in the future probably
void out_sent_handler(DictionaryIterator *sent, void *context) {
}

// reschedule update if request_update sending failed, also set update_error icon
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  schedule_update(RETRY_DELAY, PERIODIC_UPDATE);
}

// reschedule update if received data dropped, also set update_error icon
void in_dropped_handler(AppMessageResult reason, void *context) {
  bitmap_layer_set_bitmap(update_icon_layer, update_error_icon_bitmap);
  schedule_update(RETRY_DELAY, PERIODIC_UPDATE);
}

// process received data
// TODO blink whith theme and light function
void in_received_handler(DictionaryIterator *received, void *context) {
  layer_set_hidden(bitmap_layer_get_layer(update_icon_layer), true); // hide update icon

  // update main layer with received content and font
  Tuple *content = dict_find(received, KEY_CONTENT);
  if (content) {
    Tuple *font = dict_find(received, KEY_FONT);
    if (font) {
      update_info_layer(content->value->cstring, font->value->uint8);
    } else {
      update_info_layer(content->value->cstring, 0);
    }
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

  // schedule next update
  uint32_t delay = DEFAULT_REFRESH;
  Tuple *refresh = dict_find(received, KEY_REFRESH);
  if (refresh) {
    delay = refresh->value->uint32 * 1000;
  }

  schedule_update(delay, PERIODIC_UPDATE);
}

// force update on select short click
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  schedule_update(0, SHORT_PRESS_UPDATE);
}

// force update on select long click
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  schedule_update(0, LONG_PRESS_UPDATE);
}

// handle select button clicks
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, select_long_click_handler, NULL);
}

// prepare window!
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  // time layer
  text_time_layer = text_layer_create(GRect(1, -7, 142, 30));
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  // date layer
  text_date_layer = text_layer_create(GRect(1, 9, 142, 14));
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
  no_phone_icon_layer = bitmap_layer_create(GRect(41, 2, 10, 10));
  layer_set_hidden(bitmap_layer_get_layer(no_phone_icon_layer), true);
  layer_add_child(window_layer, bitmap_layer_get_layer(no_phone_icon_layer));

  // update icon layer
  update_icon_layer = bitmap_layer_create(GRect(53, 2, 10, 10));
  layer_set_hidden(bitmap_layer_get_layer(update_icon_layer), true);
  layer_add_child(window_layer, bitmap_layer_get_layer(update_icon_layer));

  // line layer
  GRect line_frame = GRect(0, 25, 144, 2);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, fill_layer);
  layer_add_child(window_layer, line_layer);

  // main info layer
  scroll_layer = scroll_layer_create(GRect(0, 26, 144, 168-26));
  scroll_layer_set_click_config_onto_window(scroll_layer, window);
  scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = &click_config_provider
  });

  text_info_layer = text_layer_create(GRect(0, 0, 144, 2000));
  text_layer_set_background_color(text_info_layer, GColorClear);

  scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_info_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

  // apply black theme
  change_theme(THEME_BLACK);

  // subscribe events
  battery_state_service_subscribe(&handle_battery);
  handle_battery(battery_state_service_peek());

  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  handle_minute_tick(NULL, MINUTE_UNIT);
}

static void window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_info_layer);

  scroll_layer_destroy(scroll_layer);

  if (wbatt_icon_bitmap) {
    gbitmap_destroy(wbatt_icon_bitmap);
  }
  if (wbatt_charge_icon_bitmap) {
    gbitmap_destroy(wbatt_charge_icon_bitmap);
  }
  if (no_phone_icon_bitmap) {
    gbitmap_destroy(no_phone_icon_bitmap);
  }
  if (update_icon_bitmap) {
    gbitmap_destroy(update_icon_bitmap);
  }
  if (update_error_icon_bitmap) {
    gbitmap_destroy(update_error_icon_bitmap);
  }

  bitmap_layer_destroy(wbatt_icon_layer);
  bitmap_layer_destroy(no_phone_icon_layer);
  bitmap_layer_destroy(update_icon_layer);

  layer_destroy(wbatt_level_layer);
  layer_destroy(line_layer);
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

  const uint32_t inbound_size = 1024;
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
