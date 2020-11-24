#include "MP3_song.h"
#include "ff.h"
#include <string.h>

typedef SONGS MP3_song_list[20];

static MP3_song_list song_list;
static size_t song_size;
static uint8_t cur_song_index = 0;

/*================================================
 *      P U B L I C   F U N C T I O N S
 *================================================
 */
void MP3_song__init(void) {
  FIL file, file_out;
  if ((f_open(&file, "song_list.txt", FA_OPEN_EXISTING | FA_READ) == FR_OK) &&
      (f_open(&file_out, "song_list_out.txt", FA_OPEN_ALWAYS | FA_OPEN_APPEND) == FR_OK)) {
    song_size = 0;
    while (!f_eof(&file)) {
      f_gets(song_list[song_size].file_name, file_size, &file);

      // Will deal with this....
      f_puts(song_list[song_size++].file_name, &file_out);

      uint32_t last_index = strlen(song_list[song_size - 1].file_name) - 1;
      if (song_list[song_size - 1].file_name[last_index] == '\n')
        song_list[song_size - 1].file_name[last_index] = '\0';
    }
  } else {
    puts("Failed to read populated file");
  }
}

uint8_t MP3_song__get_index(void) { return cur_song_index; }

void MP3_song__next(void) {
  cur_song_index++;
  // display song name into LCD
}

void MP3_song__prev(void) {
  cur_song_index--;
  // display song name into LCD
}

void MP3_song__set_index(uint8_t index) { cur_song_index = index; }

uint8_t MP3_song__get_size(void) { return song_size; }

SONGS MP3_song__search_by_index(uint8_t key) {
  cur_song_index = key;
  return song_list[key];
}

void MP3_song__print(void) {
  for (size_t INDEX = 0; INDEX < song_size; INDEX++) {
    printf("%s\n", song_list[INDEX].file_name);
  }
}