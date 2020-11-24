#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define file_size 30

typedef struct {
  char file_name[file_size];
  /*
  const char * genre;
  const char * song_name;
  const char * artist;
  */

} SONGS;

void MP3_song__init(void); // Populating Songs

uint8_t MP3_song__get_index(void);

void MP3_song__next(void);

void MP3_song__prev(void);

void MP3_song__set_index(uint8_t);

uint8_t MP3_song__get_size(void);

SONGS MP3_song__search_by_index(uint8_t);

void MP3_song__print(void);