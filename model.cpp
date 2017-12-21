#include <stdlib.h>

#include "model.h"
#include "yaml_node.h"

static const struct YamlIdStr srcRawEnum[] = {
    { MIXSRC_NONE, "None" },
    { MIXSRC_Rud,  "Rud"  },
    { MIXSRC_Ele,  "Ele"  },
    { MIXSRC_Thr,  "Thr"  },
    { MIXSRC_Ail,  "Ail"  },
    { MIXSRC_S1,   "S1"   },
    { MIXSRC_6POS, "6POS" },
    { MIXSRC_S2,   "S2"   },

    // mandatory last entry with default value
    { MIXSRC_NONE, NULL   }
};

static const struct YamlNode curveRefNodes[] = {
    YAML_UNSIGNED( "type",   8),
    YAML_SIGNED(   "value",  8),
    YAML_END
};

static const struct YamlNode mixerItems[] = {

    YAML_SIGNED(   "weight",     11 ),
    YAML_UNSIGNED( "destCh",      5 ),
    YAML_ENUM(     "srcRaw",     10,   srcRawEnum ),
    YAML_UNSIGNED( "carryTrim",   1 ),
    YAML_UNSIGNED( "mixWarn",     2 ),
    YAML_UNSIGNED( "mltpx",       2 ),
    YAML_PADDING(  1 ),
    YAML_SIGNED(   "offset",     14 ),
    YAML_SIGNED(   "swtch",       9 ),
    YAML_SIGNED(   "flightModes", 9 ),

    YAML_STRUCT( "curve", CurveRef, curveRefNodes ),

    YAML_UNSIGNED(  "delayUp",    8 ),
    YAML_UNSIGNED(  "delayDown",  8 ),
    YAML_UNSIGNED(  "speedUp",    8 ),
    YAML_UNSIGNED(  "speedDown",  8 ),

    YAML_STRING( "name", LEN_EXPOMIX_NAME ),
    YAML_END
};

static const struct YamlNode modelItems[] = {
    YAML_STRING( "name", LEN_MODEL_NAME ),
    YAML_ARRAY(  "mixers", MixData, MAX_MIXERS, mixerItems ),
    YAML_END
};

const struct YamlNode modelNode = YAML_ROOT( modelItems );
