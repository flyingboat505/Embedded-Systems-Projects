#pragma once
#include "string_t.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MP3_song_search_invalid -1

typedef struct {
  uint8_t id;
  string32_t file_name;
  string32_t song_name;
  string32_t artist;
  string32_t album;
  uint16_t year;
  string16_t genre;
} SONGS;

void MP3_song__init(void); // Populating Songs
/*

uint8_t MP3_song__get_index(void);

void MP3_song__next(void);

void MP3_song__prev(void);

void MP3_song__set_index(uint8_t);*/

// Songlist
uint8_t MP3_song__get_overall_size(void);
// SONGS MP3_song__search_by_index(uint8_t);

// Genre Options

const string16_t *MP3_song_get_genre_options(void);
uint8_t MP3_song__get_num_of_genre_options(void);

// Songlist (query)
typedef struct {
  char *genre;
  uint16_t *start_year;
  uint16_t *end_year;
} MP3_song_payload;

int8_t MP3_song__search_song_response_by_id(uint8_t id);
const SONGS *MP3_song__query_by_genre_and_year(MP3_song_payload *);
uint8_t MP3_song__get_response_size(void);

//================= DEBUG FUNCTIONS =======================
void MP3_song__print(void);
void MP3_song__print_genres(void);
void MP3_song__print_response(void);
