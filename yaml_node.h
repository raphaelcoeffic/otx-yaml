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
    { .type=YDT_STRING, .size=((max_len)<<3), YAML_TAG(tag) }

#define YAML_STRUCT(tag, stype, nodes)                                   \
    { .type=YDT_ARRAY, .size=(sizeof(stype)<<3), YAML_TAG(tag), .u={._array={ .elmts=1, .child=(nodes) }} }

#define YAML_ARRAY(tag, stype, max_elmts, nodes)                         \
    { .type=YDT_ARRAY, .size=(sizeof(stype)<<3), YAML_TAG(tag), .u={._array={ .elmts=(max_elmts), .child=(nodes) }} }

#define YAML_ENUM(tag, bits, id_strs)                                   \
    { .type=YDT_ENUM, .size=(bits), YAML_TAG(tag), .u={ ._enum={ .choices=(id_strs) } } }

#define YAML_PADDING(bits)                      \
    { .type=YDT_PADDING, .size=(bits) }

#define YAML_END                                \
    { .type=YDT_NONE, .size=0 }

#define YAML_ROOT(nodes)                                                \
    { .type=YDT_ARRAY, .size=0, .tag_len=0, .tag=NULL, .u={._array={ .elmts=1, .child=(nodes) }} }


class YamlTreeWalker
{
    struct State {
        const YamlNode* node;
        unsigned int    bit_ofs;
        uint8_t         attr_idx;
        uint8_t         elmts;
    };

    State   stack[NODE_STACK_DEPTH];
    uint8_t stack_level;
    uint8_t virt_level;

    unsigned int getAttrOfs() { return stack[stack_level].bit_ofs; }
    unsigned int getLevelOfs() {
        if (stack_level < NODE_STACK_DEPTH - 1)
            return stack[stack_level + 1].bit_ofs;
        return 0;
    }

    void setNode(const YamlNode* node) { stack[stack_level].node = node; }
    void setAttrIdx(uint8_t idx) { stack[stack_level].attr_idx = idx; }

    void setAttrOfs(unsigned int ofs) { stack[stack_level].bit_ofs = ofs; }

    void incAttr() { stack[stack_level].attr_idx++; }
    void incElmts() { stack[stack_level].elmts++; }

    bool empty() { return stack_level == NODE_STACK_DEPTH; }
    bool full()  { return stack_level == 0; }
    
    // return true on success
    bool push();
    bool pop();
    
public:
    YamlTreeWalker();

    void reset(const YamlNode* node);

    int getLevel() { return NODE_STACK_DEPTH - stack_level + virt_level; }
    
    const YamlNode* getNode() {
        return stack[stack_level].node;
    }

    const YamlNode* getAttr() {
        uint8_t idx = stack[stack_level].attr_idx;
        return &(stack[stack_level].node->u._array.child[idx]);
    }

    unsigned int getElmts() {
        return stack[stack_level].elmts;
    }

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

    bool toNextElmt();
    void toNextAttr();

    bool finished() { return empty(); }
};


#endif
