#include <pebble.h>

#include "confirm.h"
#include "social_options.h"

static Window *window;
static TextLayer *prompt_text;
static ActionBarLayer *action_bar;

static GBitmap *icon_confirm;
static GBitmap *icon_select;
static GBitmap *icon_cancel;

static char *prompt;
static int venue_index;

void send_checkin(void) {
  Tuplet header_tuplet = TupletInteger(KEY_CHECKIN, 1);
  Tuplet index_tuplet = TupletInteger(KEY_INDEX, venue_index);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    return;
  }
  dict_write_tuplet(iter, &header_tuplet);
  dict_write_tuplet(iter, &index_tuplet);

  if (share_to_facebook()) {
    Tuplet facebook_tuplet = TupletInteger(KEY_FACEBOOK, 1);
    dict_write_tuplet(iter, &facebook_tuplet);
  }
  if (share_to_twitter()) {
    Tuplet twitter_tuplet = TupletInteger(KEY_TWITTER, 1);
    dict_write_tuplet(iter, &twitter_tuplet);
  }
  dict_write_end(iter);
  app_message_outbox_send();
}

/** ActionBar Click Handlers **/
static void confirm_click_handler(ClickRecognizerRef recognizer, void *context) {
  send_checkin();
  free(prompt);
  window_stack_pop(true);
}

static void cancel_click_handler(ClickRecognizerRef recognizer, void *context) {
  free(prompt);
  window_stack_pop(true);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_social_options();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) confirm_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) cancel_click_handler);
}

/** Window Callbacks **/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  icon_confirm = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK);
  icon_cancel = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_X);
  icon_select = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SOCIAL);

  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, window);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, icon_confirm);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, icon_select);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, icon_cancel);

  prompt_text = text_layer_create((GRect) { .origin = { 10, 10 }, .size = { bounds.size.w - 30, 120 } });
  text_layer_set_text(prompt_text, prompt);
  text_layer_set_font(prompt_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(prompt_text));
}

static void window_unload(Window *window) {
  gbitmap_destroy(icon_confirm);
  gbitmap_destroy(icon_cancel);
  gbitmap_destroy(icon_select);
  action_bar_layer_remove_from_window(action_bar);
  action_bar_layer_destroy(action_bar);
  text_layer_destroy(prompt_text);
}

void confirm_init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
}

void confirm_deinit(void) {
  window_destroy(window);
}

void confirm_checkin(char *name, int index) {
  venue_index = index;

  prompt = malloc(strlen(name) + 14); //"Check-in to" plus trailing "?" and null terminator
  strcpy(prompt, "Check-in to ");
  strcat(prompt, name);
  strcat(prompt, "?");

  window_stack_push(window, true);
}
