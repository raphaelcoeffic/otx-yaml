#include <stdio.h>

#include "yaml_parser.h"
#include "yaml_node.h"
#include "yaml_bits.h"

static void yaml_set_attr(uint8_t* ptr, uint32_t bit_ofs, const YamlNode* node,
                          const char* val, uint8_t val_len)
{
    printf("set(%s, %.*s, bit-ofs=%u, bits=%u)\n",
           node->tag, val_len, val, bit_ofs, node->size);

    ptr += bit_ofs >> 3UL;
    bit_ofs &= 0x07;

    switch(node->type) {
    case YDT_STRING:
        assert(!bit_ofs);
        copy_string((char*)ptr, val, MIN(val_len, node->size - 1));
        break;
    case YDT_SIGNED:
        copy_signed(ptr, bit_ofs, node->size, val, val_len);
        break;
    case YDT_UNSIGNED:
        copy_unsigned(ptr, bit_ofs, node->size, val, val_len);
        break;
    case YDT_ENUM:
        parse_enum(ptr, bit_ofs, node->size, node->u._enum.choices, val, val_len);
        break;
    default:
        break;
    }
}


YamlParser::YamlParser(const YamlNode * node)
    : state(ps_Indent),
      indent(0),
      last_indent(0),
      attr_len(0),
      val_len(0),
      walker(node)
{
}

void YamlParser::reset()
{
    state = ps_Indent;
    last_indent = indent;
    indent = 0;
    attr_len = 0;
    val_len  = 0;
}

YamlParser::YamlResult
YamlParser::parse(const char* buffer, unsigned int size, void* opaque)
{

#define CONCAT_STR(s,s_len,c)                   \
    {                                           \
        if(s_len < MAX_STR)                     \
            s[s_len++] = c;                     \
        else                                    \
            return STRING_OVERFLOW;             \
    }

    const char* c   = buffer;
    const char* end = c + size;

    while(c < end) {
        switch(state) {

        case ps_Indent:
            if (*c == '-') {
                state = ps_Dash;
                ++indent;
                break;
            }
            // trap
        case ps_Dash:
            if (*c == ' ') { // skip space(s), should be only one??
                ++indent;
                break;
            }

            // go up one level
            if (indent < last_indent) {
                if (!walker.toParent()) {
                    printf("STOP (no parent)!\n");
                    return DONE_PARSING;
                }
            }
            // go down one level
            else if (indent > last_indent) {
                if (!walker.toChild()) {
                    printf("STOP (stack full)!\n");
                    return DONE_PARSING;
                }
            }

            if (state == ps_Dash) {
                if (!walker.nextArrayElmt()) {
                    return DONE_PARSING;
                }
            }

            state = ps_Attr;
            CONCAT_STR(attr,attr_len,*c);
            break;

        case ps_Attr:
            if (*c == ' ') {// assumes nothing else comes after spaces start
                state = ps_AttrSP;
                break;
            }
            if (*c != ':')
                CONCAT_STR(attr,attr_len,*c);
            // trap
        case ps_AttrSP:
            if (*c == ':') {
                state = ps_Sep;
                break;
            }
            break;

        case ps_Sep:
            if (*c == ' ')
                break;
            if (*c == '\r' || *c == '\n'){
                state = ps_CRLF;
                continue;
            }
            state = ps_Val;
            CONCAT_STR(val,val_len,*c);
            break;

        case ps_Val:
            if (*c == ' ' || *c == '\r' || *c == '\n') {
                state = ps_CRLF;
                continue;
            }
            CONCAT_STR(val,val_len,*c);
            break;
                
        case ps_CRLF:
            if (*c == '\n') {
                //printf("<EOL>\n");

                if (!walker.findNode(attr,attr_len)) {
                    printf("unknown attribute <%.*s>\n",attr_len,attr);
                    walker.rewind();
                }
                else {
                    for(int i=0; i < indent;i++) printf(" ");
                    yaml_set_attr(opaque, walker.getBitOffset(), walker.getAttr(), val, val_len);
                }

                // reset state at EOL
                reset();
            }
            break;
        }

        c++;
    } // for each char

    return CONTINUE_PARSING;
}
