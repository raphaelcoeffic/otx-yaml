#include "yaml_bits.h"

#define MIN(a,b) (a < b ? a : b)

static int str2int(const char* val, uint8_t val_len)
{
    bool  neg = false;
    int i_val = 0;
    
    for(uint8_t i=0; i < val_len; i++) {
        if (val[i] == '-')
            neg = true;
        else if (val[i] >= '0' && val[i] <= '9') {
            i_val = i_val * 10 + (val[i] - '0');
        }
    }

    return neg ? -i_val : i_val;
}

static uint32_t str2uint(const char* val, uint8_t val_len)
{
    uint32_t i_val = 0;
    
    for(uint8_t i=0; i < val_len; i++) {
        if (val[i] >= '0' && val[i] <= '9') {
            i_val = i_val * 10 + (val[i] - '0');
        }
    }

    return i_val;
}

void yaml_copy_string(char* dst, const char* src, uint8_t len)
{
    memcpy(dst,src,len);
    dst[len] = '\0';
}

#define MASK_LOWER(bits) ((1 << (bits)) - 1)
#define MASK_UPPER(bits) (0xFF << bits)

void yaml_copy_bits(uint8_t* dst, uint32_t i, uint32_t bit_ofs, uint8_t bits)
{
    i &= ((1UL << bits) - 1);

    if (bit_ofs) {

        *dst &= MASK_LOWER(bit_ofs);
        *(dst++) |= (i << bit_ofs) & 0xFF;

        printf("<%x>(1)",(i << bit_ofs) & 0xFF);

        if (bits < 8 - bit_ofs)
            return;

        bits -= 8 - bit_ofs;
        i = i >> (8 - bit_ofs);
    }

    while(bits >= 8) {
        printf("<%x>(2)",i & 0xFF);

        *(dst++) = i & 0xFF;
        bits -= 8;
        i = i >> 8;
    }

    if (bits) {
        uint8_t mask = MASK_UPPER(bits);
        *dst &= mask;
        *dst |= i & ~mask;

        printf("<%x>(3)",i & ~mask);
    }

    printf("\n");
}

void yaml_copy_signed(uint8_t* dst, uint32_t bit_ofs, uint8_t bits,
                 const char* val, uint8_t val_len)
{
    int i = str2int(val, val_len);
    yaml_copy_bits(dst, (uint32_t)i, bit_ofs, bits);
}

void yaml_copy_unsigned(uint8_t* dst, uint32_t bit_ofs, uint8_t bits,
                   const char* val, uint8_t val_len)
{
    uint32_t i = str2uint(val, val_len);
    yaml_copy_bits(dst, i, bit_ofs, bits);
}

void yaml_parse_enum(uint8_t* dst, uint32_t bit_ofs, uint8_t bits,
                const struct YamlIdStr* choices, const char* val, uint8_t val_len)
{
    while (choices->str) {

        // we have a match!
        if (!strncmp(val, choices->str, val_len))
            break;

        choices++;
    }
            
    yaml_copy_bits(dst, choices->id, bit_ofs, bits);
}

