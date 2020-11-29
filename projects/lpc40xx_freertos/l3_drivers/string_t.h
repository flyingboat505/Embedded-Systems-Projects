#pragma once

typedef char string16_t[16 + 1]; // Accommodate 1 space for NULL termination

//========If you wish to have a string more than 16 chars====================
typedef char string32_t[32 + 1];
typedef char string48_t[48 + 1];
typedef char string64_t[64 + 1];
typedef char string128_t[128 + 1];
typedef char string192_t[192 + 1];

typedef enum { LEFT = 0, RIGHT } rotate_DIR;

void rotate_string(char *STRING, rotate_DIR dir);