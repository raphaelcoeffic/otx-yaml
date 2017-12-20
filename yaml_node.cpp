#include "yaml_node.h"
#include "model.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// YamlNode::YamlNode(YamlNode* parent)
//     : parent(parent)
// {}

// void YamlNode::init()
// {
//     if (level) return;
//     node_init();
// }

// void YamlNode::next()
// {
//     if (level) return;
//     node_next();
// }

// YamlNode* YamlNode::get_parent()
// {
//     if(level) {
//         level--;
//         return this;
//     }
//     return parent;
// }

// YamlNode* YamlNode::get_child()
// {
//     if (!child) {
//         level++;
//         return this;
//     }
//     return child;
// }

// void YamlNode::set_attribute(const char * key, int key_len,
//                                const char * value, int value_len)
// {
//     if (level)
//         debug_attr(key,key_len,value,value_len);
//     else
//         node_set_attribute(key,key_len,value,value_len);
// }

// void YamlNode::debug_attr(const char * key, int key_len,
//                           const char * value, int value_len)
// {
//     for(int i=0; i<level;i++) printf("    ");
//     printf("Node::set_attribute(%.*s, %.*s)\n",
//            key_len, key, value_len, value);
// }

// InputsNode::InputsNode(YamlNode * parent, Input* input)
//     : YamlNode(parent),
//       base_input(input),
//       cur_input(NULL)
// {}

// void InputsNode::node_init()
// {
//     printf("InputsNode::init()\n");
//     cur_input = base_input;
// }   

// void InputsNode::node_next()
// {
//     printf("InputsNode::next()\n");
//     ++cur_input;
// }   

// void InputsNode::node_set_attribute(const char * key, int key_len,
//                                const char * value, int value_len)
// {
//     printf("    InputsNode::set_attribute(%.*s, %.*s)\n",
//            key_len, key, value_len, value);
//     assert(cur_input);

//     //TODO...
// }   

// RootNode::RootNode()
//     : YamlNode(NULL)
// {}

// extern InputsNode inputs_node;

// void RootNode::node_set_attribute(const char * key, int key_len,
//                                   const char * value, int value_len)
// {
//     printf("RootNode::set_attribute(%.*s, %.*s)\n",
//            key_len, key, value_len, value);

//     if (!key_len) return;

//     switch (key[0]) {
//     case 'n':
//         if (!strncmp(key, "name", key_len)) {
//             strncpy(model.name, value, value_len);
//             model.name[value_len] = '\0';
//         }
//         break;
//     case 'b':
//         if (!strncmp(key, "bitmap", key_len)) {
//             strncpy(model.bitmap, value, value_len);
//             model.bitmap[value_len] = '\0';
//         }
//         break;
//     case 'i':
//         if (!strncmp(key, "inputs", key_len)) {
//             child = &inputs_node;
//             return;
//         }
//         break;
//     default:
//         break;
//     }

//     child = NULL;
// }

NodeStack::NodeStack()
    : level(NODE_STACK_DEPTH)
{
    memset(nodeStack, 0, sizeof(NodeStackElmt) * NODE_STACK_DEPTH);
}

bool NodeStack::push(const YamlNode* node, unsigned int bit_ofs, unsigned int elmts)
{
    if (full())
        return false;

    level--;
    nodeStack[level].node    = node;
    nodeStack[level].bit_ofs = bit_ofs;
    nodeStack[level].elmts   = elmts;

    return true;
}

bool NodeStack::pop(const YamlNode*& node, unsigned int& bit_ofs, unsigned int& elmts)
{
    if (empty())
        return false;

    node    = nodeStack[level].node;
    bit_ofs = nodeStack[level].bit_ofs;
    elmts   = nodeStack[level].elmts;

    level++;
    return true;
}

// const YamlNode* NodeStack::getNode()
// {
//     if (empty())
//         return NULL;

//     return nodeStack[level].node;
// }

// unsigned int NodeStack::getBitOfs()
// {
//     if (empty())
//         return 0;

//     return nodeStack[level].bit_ofs;
// }


YamlTreeWalker::YamlTreeWalker(const YamlNode* node)
    : current_node(node),
      current_attr(node->child),
      attr_bit_ofs(0),
      level_bit_ofs(0),
      elmts(0),
      level(0)
{
}

// Rewind to the current node's first attribute
// (and reset the bit offset)
void YamlTreeWalker::rewind()
{
    current_attr = current_node->child;
    attr_bit_ofs = 0;
}

// Increment the cursor until a match is found or the end of
// the current collection (node of type YDT_NONE) is reached.
//
// return true if a match has been found.
bool YamlTreeWalker::findNode(const char* tag, uint8_t tag_len)
{
    if (level)
        return false;
    
    rewind();

    while(current_attr->type != YDT_NONE) {

        //printf("->comp(%.*s,%.*s)",tag_len,tag,current_attr->tag_len,current_attr->tag);
        if ((tag_len == current_attr->tag_len)
            && !strncmp(tag, current_attr->tag, tag_len)) {
            return true; // attribute found!
        }

        if (current_attr->type == YDT_ARRAY) {
            attr_bit_ofs +=
                ((uint32_t)current_attr->elmts * (uint32_t)current_attr->size) << 3UL;
        }
        else if (current_attr->type == YDT_STRING) {
            attr_bit_ofs += ((uint32_t)current_attr->size) << 3UL;
        }
        else {
            attr_bit_ofs += (uint32_t)current_attr->size;
        }
            
        current_attr++;
    }

    return false;
}

// Get the current bit offset
unsigned int YamlTreeWalker::getBitOffset()
{
    return level_bit_ofs
        + (elmts ? (elmts-1) * (current_node->size << 3) : 0)
        + attr_bit_ofs;
}

bool YamlTreeWalker::toParent()
{
    if(level) {
        level--;
        return true;
    }

    if (!stack.pop(current_node, level_bit_ofs, elmts)) {
        printf("Stack already empty!!!\n");
        return false;
    }

    return true;
}

bool YamlTreeWalker::toChild()
{
    if (!current_attr || current_attr->type != YDT_ARRAY) {
        level++;
        return true;
    }

    if (!stack.push(current_node, level_bit_ofs, elmts)) {
        printf("Stack overflow!!!\n");
        level++;
        return false;
    }

    current_node  = current_attr;
    level_bit_ofs = attr_bit_ofs;

    return true;
}


bool YamlTreeWalker::nextArrayElmt()
{
    if (!level && (current_node->type == YDT_ARRAY)) {

        if (elmts >= current_node->elmts) {
            printf("max elmts reached!!!\n");
            return false;
        }

        elmts++;
        printf("<%s> elmts = %u; size=%u\n",
               current_node->tag, elmts, current_node->size);
    }

    return true;
}
