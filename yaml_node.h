#ifndef _node_h_
#define _node_h_

#include <stdint.h>

#define NODE_STACK_DEPTH 4

enum YamlDataType {
    YDT_NONE=0,
    YDT_SIGNED,
    YDT_UNSIGNED,
    YDT_STRING,
    YDT_ARRAY,
    YDT_ENUM,
    YDT_PADDING
};

struct YamlIdStr
{
    unsigned int id;
    const char*  str;
};

struct YamlNode
{
    YamlDataType    type;
    unsigned char   size;  // bits or bytes, depending on type
    unsigned char   tag_len;
    const char*     tag;
    union {
        struct {
            unsigned char   elmts; // maximum number of elements
            const YamlNode* child;
        } _array;

        struct {
            const YamlIdStr* choices;
        } _enum;
    } u;
};

#define YAML_TAG(str)                           \
    .tag_len=(sizeof(str)-1), .tag=(str)

#define YAML_SIGNED(tag, bits)                          \
    { .type=YDT_SIGNED, .size=(bits), YAML_TAG(tag) }

#define YAML_UNSIGNED(tag, bits)                        \
    { .type=YDT_UNSIGNED, .size=(bits), YAML_TAG(tag) }

#define YAML_STRING(tag, max_len)                               \
    { .type=YDT_STRING, .size=(max_len), YAML_TAG(tag) }

#define YAML_STRUCT(tag, stype, nodes)                                   \
    { .type=YDT_ARRAY, .size=sizeof(stype), YAML_TAG(tag), .u={._array={ .elmts=1, .child=(nodes) }} }

#define YAML_ARRAY(tag, stype, max_elmts, nodes)                         \
    { .type=YDT_ARRAY, .size=sizeof(stype), YAML_TAG(tag), .u={._array={ .elmts=(max_elmts), .child=(nodes) }} }

#define YAML_ENUM(tag, bits, id_strs)                                   \
    { .type=YDT_ENUM, .size=(bits), YAML_TAG(tag), .u={ ._enum={ .choices=(id_strs) } } }

#define YAML_PADDING(bits)                      \
    { .type=YDT_PADDING, .size=(bits) }

#define YAML_END                                \
    { .type=YDT_NONE, .size=0 }

#define YAML_ROOT(nodes)                                                \
    { .type=YDT_ARRAY, .size=0, .tag_len=0, .tag=NULL, .u={._array={ .elmts=1, .child=(nodes) }} }

struct NodeStackElmt {
    const YamlNode* node;
    unsigned int    bit_ofs;
    unsigned int    elmts;
};

class NodeStack
{
    NodeStackElmt nodeStack[NODE_STACK_DEPTH];
    unsigned int  level;

public:
    NodeStack();

    bool empty() { return level == NODE_STACK_DEPTH; }
    bool full()  { return level == 0; }
    
    // return true on success
    bool push(const YamlNode* node, unsigned int bit_ofs, unsigned int elmts);
    bool pop(const YamlNode*& node, unsigned int& bit_ofs, unsigned int& elmts);
};


class YamlTreeWalker
{
    const YamlNode* current_node;
    const YamlNode* current_attr;
    unsigned int    attr_bit_ofs;
    unsigned int    level_bit_ofs;
    unsigned int    elmts;

    NodeStack       stack;
    unsigned int    level;

public:
    YamlTreeWalker(const YamlNode* node);

    // Rewind to the current node's first attribute
    // (and reset the bit offset)
    void rewind();

    // Increment the cursor until a match is found or the end of
    // the current collection (node of type YDT_NONE) is reached.
    //
    // return true if a match has been found.
    bool findNode(const char* tag, uint8_t tag_len);

    // Get the current bit offset
    unsigned int getBitOffset();

    bool toParent();
    bool toChild();

    bool nextArrayElmt();

    const YamlNode* getAttr() { return current_attr; }
};


#endif
