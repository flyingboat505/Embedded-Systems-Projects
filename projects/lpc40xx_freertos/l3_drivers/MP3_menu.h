#pragma once;
#include <stdint.h>
#include <stdio.h>

typedef enum { main_menu = 0, song_info } UI_Page;

void MP3_menu__init(void);

void MP3_menu__main_menu_scroll_up(void);

void MP3_menu__song_handler(void);

uint8_t MP3_menu__get_cur_song_index(void);