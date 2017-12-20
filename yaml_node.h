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
    YDT_PADDING
};

struct YamlNode {
    YamlDataType    type;
    unsigned char   size;  // bits or bytes, depending on type
    unsigned char   elmts; // maximum number of elements
    unsigned char   tag_len;
    const char*     tag;
    const YamlNode* child;
    const YamlNode* parent;
};

#define YAML_TAG(tag) (sizeof(tag)-1), (tag)

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

    // const YamlNode* getNode();
    // unsigned int    getBitOfs();
    //unsigned int    getElmts();
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
