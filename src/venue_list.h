#pragma once

#define MAX_NUM_VENUES 6

typedef struct {
  char name[42];
  char address[42];
} Venue;

void set_num_venues_returned(int num_venues);

void add_venue(int index, char *name, char *address);

void show_venue_list(void);

void venue_list_init(void);

void venue_list_deinit(void);