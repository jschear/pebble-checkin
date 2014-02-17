#include <pebble.h>

#include "social_options.h"

static Window *window;
static MenuLayer *social_menu;

static bool facebook = false;
static bool twitter = false;

bool share_to_facebook(void) {
	return facebook;
}

bool share_to_twitter(void) {
	return twitter;
}

/** MenuLayer Callbacks **/
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Sharing Options");
      break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 44;
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 2;
}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case 0:
      facebook = !facebook;
      break;
    case 1:
      twitter = !twitter;
      break;
  }
  layer_mark_dirty(menu_layer_get_layer(social_menu));
}

static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char *label = "";
  char *status = "";
  switch (cell_index->row) {
    case 0:
      label = "Facebook";
      status = facebook ? "ON" : "OFF";
      break;
    case 1:
      label = "Twitter";
      status = twitter ? "ON" : "OFF";
      break;
  }
  GRect bounds = layer_get_bounds(cell_layer);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  graphics_context_set_text_color(ctx, GColorBlack);
  
  GRect label_bounds = { .origin = { bounds.origin.x + 5, bounds.origin.y + 4 }, .size = { bounds.size.w - 50, bounds.size.h }};
  graphics_draw_text(ctx, label, font, label_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  GRect status_bounds = { .origin = { bounds.origin.x + 112, bounds.origin.y + 4 }, .size = { bounds.size.w - 112, bounds.size.h }};
  graphics_draw_text(ctx, status, font, status_bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

/** Window Callbacks **/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_frame = layer_get_frame(window_layer);

  social_menu = menu_layer_create(window_frame);
  menu_layer_set_callbacks(social_menu, NULL, (MenuLayerCallbacks) {
    .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
    .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
    .select_click = (MenuLayerSelectCallback) select_callback,
    .get_header_height = (MenuLayerGetHeaderHeightCallback) menu_get_header_height_callback,
    .draw_header = (MenuLayerDrawHeaderCallback) menu_draw_header_callback,
  });
  menu_layer_set_click_config_onto_window(social_menu, window);
  layer_add_child(window_layer, menu_layer_get_layer(social_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(social_menu);
}

void show_social_options(void) {
  window_stack_push(window, true);
}

void social_init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
}

void social_deinit(void) {
  window_destroy(window);
}
