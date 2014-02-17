#include <pebble.h>

#include "venue_list.h"
#include "confirm.h"
#include "social_options.h"

#define INBOUND_BUFFER_SIZE 124
#define OUTBOUND_BUFFER_SIZE 64

static Window *window;
static TextLayer *status_text;

static void check_connection(void) {
  char *status = bluetooth_connection_service_peek() ? "Fetching nearest locations..." : "Not connected to Phone";
  text_layer_set_text(status_text, status);
}

/** AppMessage Handlers **/
static void out_sent_handler(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing message delivered");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing message failed: %d", reason);

  Tuple *checkin = dict_find(failed, KEY_CHECKIN);
  if (checkin) {
    send_checkin();
  }
}

static void in_received_handler(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message recieved");

  Tuple *error_tuple = dict_find(received, KEY_ERROR);      // error
  Tuple *list_tuple  = dict_find(received, KEY_VENUE_LIST); // start or end of list
  Tuple *index_tuple = dict_find(received, KEY_INDEX);      // item in list

  if (error_tuple) {
    if (window == window_stack_get_top_window()) {
      text_layer_set_text(status_text, error_tuple->value->cstring);
    }
    else {
      //TODO
    }
  }
  else if (list_tuple) { 
    Tuple *num_tuple = dict_find(received, KEY_NUM_VENUES);
    if (num_tuple) { // start
      set_num_venues_returned(num_tuple->value->int8);
    }
    else { // end
      show_venue_list();
      window_stack_remove(window, false);
    }
  }
  else if (index_tuple) { // received venue data
    Tuple *name_tuple = dict_find(received, KEY_NAME);
    Tuple *address_tuple = dict_find(received, KEY_ADDRESS);
    add_venue(index_tuple->value->int8, name_tuple->value->cstring, address_tuple->value->cstring);
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message dropped: %d", reason);
}

static void app_message_init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(INBOUND_BUFFER_SIZE, OUTBOUND_BUFFER_SIZE);
}

/** Winow Callbacks **/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  status_text = text_layer_create((GRect) { .origin = { 10, 30 }, .size = { bounds.size.w - 20, 120 } });
  text_layer_set_text_alignment(status_text, GTextAlignmentCenter);
  text_layer_set_font(status_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(status_text));
}

static void window_unload(Window *window) {
  text_layer_destroy(status_text);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  venue_list_init();
  confirm_init();
  social_init();

  app_message_init();

  check_connection();

  app_event_loop();

  social_deinit();
  confirm_deinit();
  venue_list_deinit();
  deinit();
}
