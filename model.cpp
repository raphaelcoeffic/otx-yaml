#include <stdlib.h>

#include "model.h"
#include "yaml_node.h"

#define NODE(t, s, n, str, child)               \
    { t, (unsigned char)(s), n, YAML_TAG(str), child }

#define END_NODE                                \
    { YDT_NONE, 0, 0, 0, NULL, NULL }

#define PADDING_NODE(s)                         \
    { YDT_PADDING, s, 1, 0, NULL, NULL }

#define LEAF(t, s, n, str)                      \
    NODE(t, s, n, str, NULL)


const struct YamlNode curveRefNodes[] = {
    LEAF(YDT_UNSIGNED, 8, 1, "type"),
    LEAF(YDT_SIGNED,   8, 1, "value")
};

const struct YamlNode mixerItems[] = {

    LEAF(YDT_SIGNED,   11, 1, "weight"      ),
    LEAF(YDT_UNSIGNED,  5, 1, "destCh"      ),
    LEAF(YDT_UNSIGNED, 10, 1, "srcRaw"      ),
    LEAF(YDT_UNSIGNED,  1, 1, "carryTrim"   ),
    LEAF(YDT_UNSIGNED,  2, 1, "mixWarn"     ),
    LEAF(YDT_UNSIGNED,  2, 1, "mltpx"       ),
    PADDING_NODE(1),
    LEAF(YDT_SIGNED,   14, 1, "offset"      ),
    LEAF(YDT_SIGNED,    9, 1, "swtch"       ),
    LEAF(YDT_SIGNED,    9, 1, "flightModes" ),

    NODE(YDT_ARRAY, sizeof(CurveRef), 1, "curve", curveRefNodes ),

    LEAF(YDT_UNSIGNED,  8, 1, "delayUp"     ),
    LEAF(YDT_UNSIGNED,  8, 1, "delayDown"   ),
    LEAF(YDT_UNSIGNED,  8, 1, "speedUp"     ),
    LEAF(YDT_UNSIGNED,  8, 1, "speedDown"   ),

    LEAF(YDT_STRING, LEN_EXPOMIX_NAME, 1, "name"),
    END_NODE
};

const struct YamlNode modelItems[] = {
    LEAF(YDT_STRING, LEN_MODEL_NAME, 1, "name" ),
    NODE(YDT_ARRAY, sizeof(MixData), MAX_MIXERS, "mixers", mixerItems),
    END_NODE
};

const struct YamlNode modelNode = NODE(YDT_ARRAY, sizeof(Model), 1, "", modelItems);
