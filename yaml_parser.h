#ifndef _yaml_parser_h_
#define _yaml_parser_h_

#include <stdint.h>
#include "yaml_node.h"

#define MAX_STR 16
#define MAX_DEPTH 6 // 4 real + 2 virtual

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

    // last indents for each level
    uint8_t indents[MAX_DEPTH];

    // current indent
    uint8_t indent;

    // parser state
    uint8_t state;

    // scratch buffer w/ 16 bytes
    // used for attribute and values
    char    scratch_buf[MAX_STR];
    uint8_t scratch_len;

    // tree iterator state
    YamlTreeWalker walker;
    
    // Reset parser state for next line
    void reset();

    uint8_t getLastIndent();

public:
    // return false if error
    typedef bool (*yaml_writer_func)(void* opaque, const char* str, size_t len);

    enum YamlResult {
        DONE_PARSING,
        CONTINUE_PARSING,
        STRING_OVERFLOW
    };

    YamlParser();

    void init(const YamlNode * node);
    
    YamlResult parse(const char* buffer, unsigned int size, uint8_t* data);
    bool       generate(uint8_t* data, yaml_writer_func wf, void* opaque);
};


#endif
