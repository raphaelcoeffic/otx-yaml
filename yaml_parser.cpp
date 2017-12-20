#include <stdio.h>

#include "yaml_parser.h"
#include "yaml_node.h"


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

// bool YamlParser::findAttr()
// {
//     while(current_attr->type != YDT_NONE) {

//         if (len == current_attr->tag_len && !strcmp(attr, current_attr->tag)) {
//             return true; // attribute found!
//         }

//         if (current_attr->type == YDT_STRUCT)
//             attr_bit_ofs += ((uint32_t)current_attr->elmts) << 3UL;
//         else
//             attr_bit_ofs += (uint32_t)current_attr->elmts;
//         //TODO: YDT_ARRAY: fetch array size...

//         curr_attr++;
//     }

//     return false;
// }

// void YamlParser::rewindAttrs()
// {
//     current_attr = current_node->children;
//     attr_bit_ofs = 0;
// }

YamlParser::YamlResult YamlParser::parse(const char* buffer, unsigned int size,
                                         attr_set_func f, void* opaque)
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

            //printf("Attr starting: ('%.*s')\n",(int)(end-c),c);

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
                //TODO: current_node->next();
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
                    f(opaque, walker.getBitOffset(), walker.getAttr(), val, val_len);
                    // printf("set(%.*s, %.*s, bit-ofs=%u, sz=%u)\n",
                    //        attr_len, attr, val_len, val,
                    //        walker.getBitOffset(),
                    //        walker.getAttr()->size);
                }
                //TODO: set value if YDT_SIGNED, YDT_UNSIGNED or YDT_STRING

                // reset state at EOL
                reset();
            }
            break;
        }

        c++;
    } // for each char

    return CONTINUE_PARSING;
}
