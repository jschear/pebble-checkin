#include <pebble.h>

#include "venue_list.h"
#include "confirm.h"

static Window *window;
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
  Venue *venue = &venues[index];
  menu_cell_basic_draw(ctx, cell_layer, venue->name, venue->address, NULL);
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  return num_venues_returned;
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  const int index = cell_index->row;
  Venue *venue = &venues[index];
  confirm_checkin(venue->name, index);
}

/** Winow Callbacks **/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_frame = layer_get_frame(window_layer);

  menu_layer = menu_layer_create(window_frame);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
    .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
    .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
    .select_click = (MenuLayerSelectCallback) select_callback,
  });
  menu_layer_set_click_config_onto_window(menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(menu_layer);
}


void set_num_venues_returned(int num_venues) {
  num_venues_returned = num_venues;
}

void add_venue(int index, char *name, char *address) {
  Venue venue;
  strncpy(venue.name, name, sizeof(venue.name));
  strncpy(venue.address, address, sizeof(venue.address));
  venues[index] = venue;
}

void show_venue_list(void) {
  window_stack_push(window, true);
}

void venue_list_init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
}

void venue_list_deinit(void) {
  window_destroy(window);
}
