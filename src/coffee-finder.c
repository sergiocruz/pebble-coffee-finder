#include <pebble.h>
 
static Window *window;
static TextLayer *title_layer;
static TextLayer *business_layer;
static TextLayer *address_layer;
static TextLayer *time_layer;
static TextLayer *date_layer;

// static InverterLayer *inverter_layer;
 
static AppSync sync;
static uint8_t sync_buffer[256];
 
enum {
  OUR_LOCATION = 0x0,
  OUR_BUSINESS = 0x1
};
 
void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
   
  switch (key) {
    case OUR_LOCATION:
      text_layer_set_text(address_layer, new_tuple->value->cstring);
      break;

    case OUR_BUSINESS:
      text_layer_set_text(business_layer, new_tuple->value->cstring);
      break;
  }
}
 
// http://stackoverflow.com/questions/21150193/logging-enums-on-the-pebble-watch/21172222#21172222
char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}
 
void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "... Sync Error: %s", translate_error(app_message_error));
}
 
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00";
 
  strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
  text_layer_set_text(time_layer, time_text);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  if (!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
  }

  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%a %e", tick_time);
  text_layer_set_text(date_layer, date_text);


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

  text_layer_set_text(time_layer, time_text);
}
 
static void init_clock(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // inverter_layer = inverter_layer_create(GRect(0, 90, bounds.size.w, bounds.size.h/2));
  // layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
 
  time_layer = text_layer_create(GRect(0, 90, bounds.size.w, bounds.size.h-100));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

  date_layer = text_layer_create((GRect) { .origin = { 0, 130 }, .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(date_layer, "June 12");
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_overflow_mode(date_layer, GTextOverflowModeFill);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
 
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

  size_t strftime (char* ptr, size_t maxsize, const char* format, const struct tm* current_time );
 
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}
 
static void init_location_search(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
 
  title_layer = text_layer_create((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w, 100 } });
  text_layer_set_text(title_layer, "Closest Coffee Shop:");
  text_layer_set_text_color(title_layer, GColorWhite);
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_background_color(title_layer, GColorClear);
  text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(title_layer));
 
  business_layer = text_layer_create((GRect) { .origin = { 0, 30 }, .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(business_layer, "");
  text_layer_set_text_color(business_layer, GColorWhite);
  text_layer_set_text_alignment(business_layer, GTextAlignmentCenter);
  text_layer_set_background_color(business_layer, GColorClear);
  text_layer_set_overflow_mode(business_layer, GTextOverflowModeWordWrap);
  text_layer_set_font(business_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(business_layer));

  address_layer = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(address_layer, "");
  text_layer_set_text_color(address_layer, GColorWhite);
  text_layer_set_text_alignment(address_layer, GTextAlignmentCenter);
  text_layer_set_background_color(address_layer, GColorClear);
  text_layer_set_overflow_mode(address_layer, GTextOverflowModeWordWrap);
  text_layer_set_font(address_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(address_layer));

 
  Tuplet initial_values[] = {
     TupletCString(OUR_BUSINESS, "Looking for coffee..."),
     TupletCString(OUR_LOCATION, "")
  };
 
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}
 
static void window_load(Window *window) {
  init_location_search(window);
  init_clock(window);
}
 
static void window_unload(Window *window) {
  text_layer_destroy(title_layer);
  text_layer_destroy(business_layer);
  text_layer_destroy(address_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
}
 
static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
 
  app_message_open(256, 256);
  // app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum())
 
  const bool animated = true;
  window_stack_push(window, animated);
  window_set_background_color(window, GColorBlack);
  // window_set_background_color(window, GColorWhite);
}
 
static void deinit(void) {
  window_destroy(window);
}
 
int main(void) {
  init();
 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
 
  app_event_loop();
  deinit();
}
