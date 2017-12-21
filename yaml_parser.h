#ifndef _yaml_parser_h_
#define _yaml_parser_h_

#include <stdint.h>
#include "yaml_node.h"

#define MAX_STR 16

class YamlParser
{
    enum ParserState {
        ps_Indent=0,
        ps_Dash,
        ps_Attr,
        ps_AttrSP,
        ps_Sep,
        ps_Val,
        ps_CRLF
    };

    ParserState state;
    
    uint8_t indent;
    uint8_t last_indent;

    char    attr[MAX_STR];
    uint8_t attr_len;

    char    val[MAX_STR];
    uint8_t val_len;

    YamlTreeWalker walker;
    
    // Reset parser state for next line
    void reset();

public:
    enum YamlResult {
        DONE_PARSING,
        CONTINUE_PARSING,
        STRING_OVERFLOW
    };

    YamlParser(const YamlNode * node);
    YamlResult parse(const char* buffer, unsigned int size, void* opaque);
};


#endif
