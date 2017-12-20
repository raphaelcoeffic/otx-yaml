#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yaml_parser.h"
#include "yaml_node.h"
#include "model.h"

#define MIN(a,b) (a < b ? a : b)


Model model;

// RootNode root_node;
// InputsNode inputs_node(&root_node, &model.inputs[0]);

int str2int(const char* val, unsigned char val_len)
{
    bool  neg = false;
    int i_val = 0;
    
    for(unsigned char i=0; i < val_len; i++) {
        if (val[i] == '-')
            neg = true;
        else if (val[i] >= '0' && val[i] <= '9') {
            i_val = i_val * 10 + (val[i] - '0');
        }
    }

    return neg ? -i_val : i_val;
}

unsigned int str2uint(const char* val, unsigned char val_len)
{
    unsigned int i_val = 0;
    
    for(unsigned char i=0; i < val_len; i++) {
        if (val[i] >= '0' && val[i] <= '9') {
            i_val = i_val * 10 + (val[i] - '0');
        }
    }

    return i_val;
}

void copy_string(char* dst, const char* src, unsigned char len)
{
    memcpy(dst,src,len);
    dst[len] = '\0';
}

#define MASK_LOWER(bits) ((1 << (bits)) - 1)
#define MASK_UPPER(bits) (0xFF << bits)

void copy_bits(unsigned char* dst, unsigned int i, unsigned int bit_ofs, unsigned char bits)
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
        unsigned char mask = MASK_UPPER(bits);
        *dst &= mask;
        *dst |= i & ~mask;

        printf("<%x>(3)",i & ~mask);
    }

    printf("\n");
}

void copy_signed(unsigned char* dst, unsigned int bit_ofs, unsigned char bits,
                 const char* val, unsigned char val_len)
{
    int i = str2int(val, val_len);
    copy_bits(dst, (unsigned int)i, bit_ofs, bits);
}

void copy_unsigned(unsigned char* dst, unsigned int bit_ofs, unsigned char bits,
                   const char* val, unsigned char val_len)
{
    unsigned int i = str2uint(val, val_len);
    copy_bits(dst, i, bit_ofs, bits);
}

static void model_set_attr(unsigned char* ptr, unsigned int bit_ofs, const YamlNode* node,
                           const char* val, unsigned char val_len)
{
    printf("set(%s, %.*s, bit-ofs=%u, bits=%u)\n",
           node->tag, val_len, val, bit_ofs, node->size);

    ptr += bit_ofs >> 3UL;
    bit_ofs &= 0x07;

    switch(node->type) {
    case YDT_STRING:
        assert(!bit_ofs);
        copy_string((char*)ptr,val,MIN(val_len,node->size-1));
        break;
    case YDT_SIGNED:
        copy_signed(ptr,bit_ofs,node->size,val,val_len);
        break;
    case YDT_UNSIGNED:
        copy_unsigned(ptr,bit_ofs,node->size,val,val_len);
        break;
    default:
        break;
    }
}

#define print_mixer_attr(attr)                  \
    printf("\t" #attr "\t=\t%i\n", model.mixData[i].attr)

void print_model()
{
    printf("%s\n",model.name);
    for(int i=0; i < MAX_MIXERS; i++) {
        printf(" mixer %i\n",i);
        print_mixer_attr(weight);
        print_mixer_attr(destCh);
        print_mixer_attr(carryTrim);
        print_mixer_attr(offset);
        print_mixer_attr(swtch);
        print_mixer_attr(flightModes);
        printf("\t" "name" "\t=\t" "%s" "\n", model.mixData[i].name);
    }
}


int main()
{
    YamlParser yp(&modelNode);

    char   buffer[32];
    FILE * f = fopen("./test.yaml", "r");

    while(1) {

        size_t size = fread(buffer, 1, sizeof(buffer), f);
        if (size == 0) break;

        if (yp.parse(buffer,(unsigned int)size,
                     (attr_set_func)model_set_attr, &model) != YamlParser::CONTINUE_PARSING)
            break;
    } // until file consumed

    printf("RESULTAT\n");

    print_model();

    return 0;
}

