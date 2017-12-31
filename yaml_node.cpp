#include "yaml_node.h"
#include "model.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

YamlTreeWalker::YamlTreeWalker()
    : stack_level(NODE_STACK_DEPTH),
      virt_level(0)
{
}

void YamlTreeWalker::reset(const YamlNode* node)
{
    stack_level = NODE_STACK_DEPTH;
    virt_level  = 0;

    push();
    setNode(node);
    rewind();
}

bool YamlTreeWalker::push()
{
    if (full())
        return false;

    stack_level--;
    memset(&(stack[stack_level]), 0, sizeof(State));

    return true;
}

bool YamlTreeWalker::pop()
{
    if (empty())
        return false;

    stack_level++;
    return true;
}

// Rewind to the current node's first attribute
// (and reset the bit offset)
void YamlTreeWalker::rewind()
{
    if (getNode()->type == YDT_ARRAY) {
        setAttrIdx(0);
        setAttrOfs(getLevelOfs());
    }
}

// Increment the cursor until a match is found or the end of
// the current collection (node of type YDT_NONE) is reached.
//
// return true if a match has been found.
bool YamlTreeWalker::findNode(const char* tag, uint8_t tag_len)
{
    if (virt_level)
        return false;
    
    rewind();

    const struct YamlNode* attr = getAttr();
    while(attr && attr->type != YDT_NONE) {

        if ((tag_len == attr->tag_len)
            && !strncmp(tag, attr->tag, tag_len)) {
            return true; // attribute found!
        }

        toNextAttr();
        attr = getAttr();
    }

    return false;
}

// Get the current bit offset
unsigned int YamlTreeWalker::getBitOffset()
{
    return ((uint32_t)getElmts()) * ((uint32_t)getNode()->size) + getAttrOfs();
}

bool YamlTreeWalker::toParent()
{
    if(virt_level) {
        virt_level--;
        return true;
    }

    if (!pop())
        return false;

    return !empty();
}

bool YamlTreeWalker::toChild()
{
    const struct YamlNode* attr = getAttr();
    if (!attr || attr->type != YDT_ARRAY) {
        virt_level++;
        return true;
    }

    if (!push()) {
        virt_level++;
        return false;
    }

    setNode(attr);
    setAttrOfs(getLevelOfs());
    return true;
}

bool YamlTreeWalker::toNextElmt()
{
    const struct YamlNode* node = getNode();
    if (!virt_level && (node->type == YDT_ARRAY)) {

        if (getElmts() >= node->u._array.elmts - 1)
            return false;

        incElmts();
        rewind();
    }

    return true;
}

bool YamlTreeWalker::isElmtEmpty(uint8_t* data)
{
    const struct YamlNode* node = getNode();
    if (!virt_level && (node->type == YDT_ARRAY) && data) {

        uint32_t bit_ofs = ((uint32_t)getElmts()) * ((uint32_t)getNode()->size)
            + getLevelOfs();

        return !node->u._array.is_active
            // assume structs aligned on 8bit boundaries
            || !node->u._array.is_active(data + (bit_ofs >> 3));
    }

    return false;
}

void YamlTreeWalker::toNextAttr()
{
    const struct YamlNode* attr = getAttr();
    unsigned int attr_bit_ofs = getAttrOfs();

    if (attr->type == YDT_ARRAY)
        attr_bit_ofs += ((uint32_t)attr->u._array.elmts * (uint32_t)attr->size);
    else
        attr_bit_ofs += (uint32_t)attr->size;

    setAttrOfs(attr_bit_ofs);
    incAttr();
}
