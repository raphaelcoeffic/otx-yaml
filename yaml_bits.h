#ifndef _yaml_bits_h_
#define _yaml_bits_h_

#include <stdint.h>

void yaml_copy_string(char* dst, const char* src, uint8_t len);
void yaml_copy_bits(uint8_t* dst, uint32_t i, uint32_t bit_ofs, uint8_t bits);
void yaml_copy_signed(uint8_t* dst, uint32_t bit_ofs, uint8_t bits, const char* val, uint8_t val_len);
void yaml_copy_unsigned(uint8_t* dst, uint32_t bit_ofs, uint8_t bits, const char* val, unsigned char val_len);
void yaml_parse_enum(uint8_t* dst, uint32_t bit_ofs, uint8_t bits, const struct YamlIdStr* choices, const char* val, unsigned char val_len);

#endif
