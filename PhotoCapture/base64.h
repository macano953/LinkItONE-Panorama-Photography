#ifndef BASE64_H
#define BASE64_H
#include <stdint.h>
#include <stdlib.h>
#include "Arduino.h"
char *base64_encode(char *data,
                    size_t input_length,
                    size_t *output_length);
unsigned char *base64_decode(char *data,
                             size_t input_length,
                             size_t *output_length);
void build_decoding_table();
void base64_cleanup();
#endif
