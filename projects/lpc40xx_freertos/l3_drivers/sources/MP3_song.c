#include "MP3_song.h"
#include "ff.h"
#include <stdbool.h>
#include <string.h>

typedef SONGS MP3_song_list[20];

static MP3_song_list song_list;
static size_t song_size;

#define strncmp_equal 0

static void MP3_song__append_new_genre(const char *genre);

static SONGS MP3_song__set_song(const char *song_buffer) {
  SONGS SONG_SET = {0};
  size_t INDEX = 0;
  uint8_t count = 0;
  SONG_SET.id = song_size + 1;
  while (song_buffer[INDEX] != '\0') {
    if (song_buffer[INDEX] == '|') {
      count++;
      INDEX++;
    } else if (count == 0)
      strncat(SONG_SET.file_name, &song_buffer[INDEX++], 1);
    else if (count == 1)
      strncat(SONG_SET.song_name, &song_buffer[INDEX++], 1);
    else if (count == 2)
      strncat(SONG_SET.artist, &song_buffer[INDEX++], 1);
    else if (count == 3)
      strncat(SONG_SET.album, &song_buffer[INDEX++], 1);
    else if (count == 4)
      SONG_SET.year = SONG_SET.year * 10 + (song_buffer[INDEX++] - 0x30);
    else
      strncat(SONG_SET.genre, &song_buffer[INDEX++], 1);
  }
  return SONG_SET;
}

/*================================================
 *      P U B L I C   F U N C T I O N S
 *================================================
 */

static string16_t genre_option[21] = {0};
static uint8_t num_of_genre = 0;

//============Overall Songlist===================
void MP3_song__init(void) {
  FIL file, file_out;
  song_size = 0;
  strncpy(genre_option[num_of_genre++], "All", sizeof(string16_t) - 1);
  if ((f_open(&file, "song_list.txt", FA_OPEN_EXISTING | FA_READ) == FR_OK) &&
      (f_open(&file_out, "song_list_out.txt", FA_OPEN_ALWAYS | FA_WRITE | FA_OPEN_APPEND) == FR_OK)) {
    string128_t song_buffer = {0};
    while (!f_eof(&file)) {
      f_gets(song_buffer, sizeof(string128_t) - 1, &file);
      // Will deal with this....
      // f_puts(song_list[song_size++].file_name, &file_out);
      uint32_t last_index = strlen(song_buffer) - 1;
      if (song_buffer[last_index] == '\n') {
        song_buffer[last_index] = '\0';
        song_buffer[last_index - 1] = '\0';
      }
      song_list[song_size] = MP3_song__set_song(song_buffer);
      MP3_song__append_new_genre(song_list[song_size++].genre);
    }
  } else {
    puts("Failed to read populated file");
  }
  puts("\n");
}

uint8_t MP3_song__get_overall_size(void) { return song_size; }
/*
SONGS MP3_song__search_by_index(uint8_t key) {
  cur_song_index = key;
  return song_list[key];
}*/

//============Genre Options===================
static void MP3_song__append_new_genre(const char *genre) {
  memset(genre_option[num_of_genre], 0, sizeof(string16_t));
  for (size_t INDEX = 0; INDEX < num_of_genre; INDEX++) {
    if (strncmp(genre, genre_option[INDEX], sizeof(string16_t) - 1) == strncmp_equal)
      return;
  }
  strncpy(genre_option[num_of_genre++], genre, strlen(genre));
}

const string16_t *MP3_song_get_genre_options(void) { return genre_option; }
uint8_t MP3_song__get_num_of_genre_options(void) { return num_of_genre; }

//============Genre Options===================

static MP3_song_list response_song_list;
static size_t song_reponse_size;

int8_t MP3_song__search_song_response_by_id(uint8_t id) {
  uint8_t INDEX;
  for (INDEX = 0; INDEX < song_reponse_size; INDEX++) {
    if (id == response_song_list[INDEX].id)
      return INDEX;
  }
  return MP3_song_search_invalid;
}

const SONGS *MP3_song__query_by_genre_and_year(MP3_song_payload *payload) {
  bool valid_criteria =
      payload->end_year == NULL || payload->end_year == NULL || !(*payload->end_year < *payload->start_year);
  if (valid_criteria) {
    song_reponse_size = 0;
    for (size_t INDEX = 0; INDEX < song_size; INDEX++) {
      bool genre_valid = ((0 == strncmp(payload->genre, "All", strlen(payload->genre))) ||
                          (0 == strncmp(song_list[INDEX].genre, payload->genre, strlen(payload->genre))));
      // printf("%i\n", genre_valid);

      bool year_valid = (payload->start_year == NULL && payload->end_year == NULL) ||
                        (payload->end_year == NULL && song_list[INDEX].year >= *payload->start_year) ||
                        (payload->start_year == NULL && song_list[INDEX].year <= *payload->end_year) ||
                        (song_list[INDEX].year >= *payload->start_year && song_list[INDEX].year <= *payload->end_year);

      if (genre_valid && year_valid) {
        response_song_list[song_reponse_size++] = song_list[INDEX];
      }
    }
  }
  return response_song_list;
}
uint8_t MP3_song__get_response_size(void) { return song_reponse_size; }
//================= DEBUG FUNCTIONS =======================
void MP3_song__print(void) {
  for (size_t INDEX = 0; INDEX < song_size; INDEX++) {
    printf("%d | %s | %s | %s | %s | %d | %s\n", song_list[INDEX].id, song_list[INDEX].file_name,
           song_list[INDEX].song_name, song_list[INDEX].artist, song_list[INDEX].album, song_list[INDEX].year,
           song_list[INDEX].genre);
  }
}
void MP3_song__print_genres(void) {
  for (size_t INDEX = 0; INDEX < num_of_genre; INDEX++) {
    printf(" %s|\n", genre_option[INDEX]);
  }
}

void MP3_song__print_response(void) {
  for (size_t INDEX = 0; INDEX < song_reponse_size; INDEX++) {
    printf("%d | %s | %s | %s | %s | %d | %s\n", response_song_list[INDEX].id, response_song_list[INDEX].file_name,
           response_song_list[INDEX].song_name, response_song_list[INDEX].artist, response_song_list[INDEX].album,
           response_song_list[INDEX].year, response_song_list[INDEX].genre);
  }
}