#include "string_t.h"
#include <stddef.h>
#include <stdint.h>

static void rotate_string_left(char *STRING) {
  char first_char = STRING[0];
  size_t INDEX;
  for (INDEX = 0; STRING[INDEX + 1] != '\0'; INDEX++) {
    STRING[INDEX] = STRING[INDEX + 1];
  }
  STRING[INDEX] = first_char;
}

static void rotate_string_right(char *STRING) { // Might not need right rotation
  char temp_char[2] = {0};
  uint8_t temp_char_index = 0;
  size_t INDEX = 0;
  temp_char[temp_char_index] = STRING[INDEX];
  temp_char_index = (temp_char_index + 1) % 2;
  while (1) {
    temp_char[temp_char_index] = STRING[INDEX];
    temp_char_index = (temp_char_index + 1) % 2;
    if (STRING[INDEX + 1] == '\0') {
      STRING[0] = temp_char[temp_char_index];
      break;
    }
    STRING[++INDEX] = temp_char[temp_char_index];
  }
}

void rotate_string(char *STRING, rotate_DIR dir) {
  switch (dir) {
  case LEFT:
    rotate_string_left(STRING);
    break;
  case RIGHT:
    rotate_string_right(STRING);
    break;
  default:
    break;
  }
}