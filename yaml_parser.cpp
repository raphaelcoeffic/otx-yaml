#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "yaml_parser.h"

YamlParser::YamlParser()
{
}

void YamlParser::init(const YamlParserCalls* parser_calls, void* parser_ctx)
{
    indent = 0;
    memset(indents, 0, sizeof(indents));

    calls = parser_calls;
    ctx   = parser_ctx;
    reset();
}

void YamlParser::reset()
{
    state = ps_Indent;
    indents[calls->get_level(ctx) - 1] = indent;
    indent = scratch_len  = 0;
    node_found = false;
}

uint8_t YamlParser::getLastIndent()
{
    return indents[calls->get_level(ctx) - 1];
}

YamlParser::YamlResult
YamlParser::parse(const char* buffer, unsigned int size)
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

            if (indent < getLastIndent()) {
                // go up as many levels as necessary
                do {
                    if (!calls->to_parent(ctx)) {
                        printf("STOP (no parent)!\n");
                        return DONE_PARSING;
                    }
                } while (indent < getLastIndent());

                if (state == ps_Dash) {
                    if (!calls->to_next_elmt(ctx)) {
                        return DONE_PARSING;
                    }
                }
            }
            // go down one level
            else if (indent > getLastIndent()) {
                if (!calls->to_child(ctx)) {
                    printf("STOP (stack full)!\n");
                    return DONE_PARSING; // ERROR
                }

                if (calls->get_level(ctx) > MAX_DEPTH) {
                    printf("STOP (indent stack full)!\n");
                    return DONE_PARSING; // ERROR
                }
            }
            // same level, next element
            else  if (state == ps_Dash) {
                if (!calls->to_next_elmt(ctx)) {
                    return DONE_PARSING;
                }
            }

            state = ps_Attr;
            CONCAT_STR(scratch_buf, scratch_len, *c);
            break;

        case ps_Attr:
            if (*c == ' ') {// assumes nothing else comes after spaces start
                node_found = calls->find_node(ctx, scratch_buf, scratch_len);
                if (!node_found) {
                    printf("Could not find node '%.*s' (1)\n", scratch_len, scratch_buf);
                }
                state = ps_AttrSP;
                break;
            }
            if (*c != ':')
                CONCAT_STR(scratch_buf, scratch_len, *c);
            // trap
        case ps_AttrSP:
            if (*c == '\r' || *c == '\n') {
                if (state == ps_Attr) {
                    node_found = calls->find_node(ctx, scratch_buf, scratch_len);
                    if (!node_found) {
                        printf("Could not find node '%.*s' (2)\n", scratch_len, scratch_buf);
                    }
                }
                state = ps_CRLF;
                continue;
            }
            if (*c == ':') {
                if (state == ps_Attr) {
                    node_found = calls->find_node(ctx, scratch_buf, scratch_len);
                    if (!node_found) {
                        printf("Could not find node '%.*s' (3)\n", scratch_len, scratch_buf);
                    }
                }
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
            scratch_len = 0;
            CONCAT_STR(scratch_buf, scratch_len, *c);
            break;

        case ps_Val:
            if (*c == '\r' || *c == '\n') {
                // set attribute
                if (node_found) {
                    calls->set_attr(ctx, scratch_buf, scratch_len);
                }
                state = ps_CRLF;
                continue;
            }
            CONCAT_STR(scratch_buf, scratch_len, *c);
            break;
                
        case ps_CRLF:
            if (*c == '\n') {
                // reset state at EOL
                reset();
            }
            break;
        }

        c++;
    } // for each char

    return CONTINUE_PARSING;
}

