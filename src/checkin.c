#include <pebble.h>

#include "confirm.h"

#define MAX_NUM_VENUES 8

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data);
static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data);
static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void load_venues(void);
static void out_sent_handler(DictionaryIterator *sent, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
static void in_received_handler(DictionaryIterator *received, void *context);
static void in_dropped_handler(AppMessageResult reason, void *context);
static void app_message_init(void);
static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);

typedef struct {
  char id[32];
  char name[42];
  char address[42];
} Venue; 

static Window *window;
static TextLayer *text_layer;
static MenuLayer *menu_layer;

static uint16_t num_venues_returned;
static Venue venues[MAX_NUM_VENUES];

/** MenuLayer Callbacks **/
static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 44;
}

static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data) {
  const int index = cell_index->row;
  if (index < 0 || index >= MAX_NUM_VENUES) {
    return;
  }
  Venue venue = venues[index];
  menu_cell_basic_draw(ctx, cell_layer, venue.name, venue.address, NULL);
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return num_venues_returned;
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  const int index = cell_index->row;
  Venue venue = venues[index];
  confirm_checkin(venue.id, venue.name);
}

/** AppMessages **/
static void load_venues(void) {
  Tuplet header_tuplet = TupletInteger(KEY_FETCH_VENUES, 1);
  Tuplet venues_tuplet = TupletInteger(KEY_NUM_VENUES, 8);
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    return;
  }
  dict_write_tuplet(iter, &header_tuplet);
  dict_write_tuplet(iter, &venues_tuplet);
  dict_write_end(iter);
  app_message_outbox_send();
}

/** AppMessage Handlers **/
static void out_sent_handler(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing message delivered");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing message failed: %d", reason);

  Tuple *fetch_venues = dict_find(failed, KEY_FETCH_VENUES);
  Tuple *checkin = dict_find(failed, KEY_CHECKIN);

  // retry sending failed message
  if (fetch_venues) {
    load_venues();
  }
  else if (checkin) {
    send_checkin();
  }
}

static void in_received_handler(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message recieved");

  Tuple *error_tuple = dict_find(received, KEY_ERROR); // received error
  if (error_tuple) {
    text_layer_set_text(text_layer, error_tuple->value->cstring);
    layer_set_hidden(text_layer_get_layer(text_layer), false);
    layer_set_hidden(menu_layer_get_layer(menu_layer), true);
  }
  else {
    Tuple *index_tuple = dict_find(received, KEY_INDEX);
    Tuple *fetch_tuple = dict_find(received, KEY_FETCH_VENUES);

    if (fetch_tuple) { // received start/end of list
      Tuple *num_tuple = dict_find(received, KEY_NUM_VENUES);
      if (num_tuple) { // start
        num_venues_returned = num_tuple->value->int8;
      }
      else { // end
        layer_set_hidden(text_layer_get_layer(text_layer), true);
        layer_set_hidden(menu_layer_get_layer(menu_layer), false);
        menu_layer_reload_data(menu_layer);
      }
    }
    else if (index_tuple) { // received venue data
      Tuple *id_tuple = dict_find(received, KEY_VENUE_ID);
      Tuple *name_tuple = dict_find(received, KEY_NAME);
      Tuple *address_tuple = dict_find(received, KEY_ADDRESS);

      Venue venue;
      strncpy(venue.id, id_tuple->value->cstring, sizeof(venue.id));
      strncpy(venue.name, name_tuple->value->cstring, sizeof(venue.name));
      strncpy(venue.address, address_tuple->value->cstring, sizeof(venue.address));
      venues[index_tuple->value->int8] = venue;
    }
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
  app_message_open(124, 64); //inbound, outbound sizes
}

/** Winow Callbacks **/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect window_frame = layer_get_frame(window_layer);

  // text layer
  text_layer = text_layer_create((GRect) { .origin = { 10, 30 }, .size = { bounds.size.w - 20, 120 } });
  text_layer_set_text(text_layer, "Finding nearest locations...");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  // menu layer (hidden initially)
  menu_layer = menu_layer_create(window_frame);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
    .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
    .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
    .select_click = (MenuLayerSelectCallback) select_callback,
  });
  menu_layer_set_click_config_onto_window(menu_layer, window);
  layer_set_hidden(menu_layer_get_layer(menu_layer), true);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  menu_layer_destroy(menu_layer);
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
  confirm_init();
  app_message_init();

  load_venues();

  app_event_loop();

  confirm_deinit();
  deinit();
}
