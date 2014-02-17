#pragma once

enum {
  KEY_VENUE_LIST = 0x0,
  KEY_NUM_VENUES = 0x1,
  KEY_INDEX = 0x2,
  KEY_NAME = 0x3,
  KEY_ADDRESS = 0x4,
  KEY_CHECKIN = 0x5,
  KEY_ERROR = 0x6,
  KEY_FACEBOOK = 0x7,
  KEY_TWITTER = 0x8
};

void send_checkin(void);

void confirm_init(void);

void confirm_deinit(void);

void confirm_checkin(char *name, int index);
