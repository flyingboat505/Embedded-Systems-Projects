#pragma once
#include <stdint.h>
#include <stdio.h>

//=================================================
//                G E N E R A L
//=================================================

typedef enum { reboot = 0, login, main_menu, setting, song_list, song_info, filter } UI_Page;

void MP3_menu__init(void);
void MP3_menu__UI_handler(const unsigned char);
void MP3_menu_SONG_LIST_rotate_string(void);

//=================================================
//                 L O G I N
//=================================================
typedef enum { IN_PROCESS = 0, INCORRECT, SUCCESS } LOGIN_stats;

LOGIN_stats MP3_menu__LOGIN_get_LOGIN_status(void);

void MP3_menu__VOL_BASS_TREM_handler(uint16_t);

void MP3_menu__finish_song_handler(void);