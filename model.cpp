#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "model.h"
#include "yaml_node.h"
#include "yaml_parser.h"

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

    YAML_IDX,
    YAML_SIGNED(   "weight",     11 ),
    YAML_UNSIGNED( "destCh",      5 ),
    YAML_ENUM(     "srcRaw",     10,   srcRawEnum ),
    YAML_UNSIGNED( "carryTrim",   1 ),
    YAML_UNSIGNED( "mixWarn",     2 ),
    YAML_UNSIGNED( "mltpx",       2 ),
    YAML_PADDING(  1 ),
    YAML_SIGNED(   "offset",     14 ),
    YAML_SIGNED(   "swtch",       9 ),
    YAML_UNSIGNED( "flightModes", 9 ),

    YAML_STRUCT( "curve", CurveRef, curveRefNodes, NULL ),

    YAML_UNSIGNED(  "delayUp",    8 ),
    YAML_UNSIGNED(  "delayDown",  8 ),
    YAML_UNSIGNED(  "speedUp",    8 ),
    YAML_UNSIGNED(  "speedDown",  8 ),

    YAML_STRING( "name", LEN_EXPOMIX_NAME ),
    YAML_END
};

static uint32_t in_read_weight(const char* val, uint8_t val_len);
static bool     in_write_weight(uint32_t val, YamlNode::writer_func wf, void* opaque);

static const struct YamlNode inputItems[] = {

    YAML_IDX,
    YAML_UNSIGNED( "mode",        2 ),
    YAML_UNSIGNED( "scale",      14 ),
    YAML_ENUM(     "srcRaw",     10,   srcRawEnum ),
    YAML_SIGNED(   "carryTrim",   6 ),
    YAML_UNSIGNED( "chn",         5 ),
    YAML_SIGNED(   "swtch",       9 ),
    YAML_UNSIGNED( "flightModes", 9 ),
    YAML_SIGNED_CUST( "weight",   8, in_read_weight, in_write_weight ),
    YAML_PADDING(  1 ),
    YAML_STRING(   "name", LEN_EXPOMIX_NAME ),
    YAML_SIGNED(   "offset",      8 ),

    YAML_STRUCT( "curve", CurveRef, curveRefNodes, NULL ),
    YAML_END
};

static bool mixer_active(uint8_t* data);
static bool input_active(uint8_t* data);

static const struct YamlIdStr custFn[] = {
    { FUNC_OVERRIDE_CHANNEL, "override-ch" },
    { FUNC_TRAINER,          "trainer"  },
    { FUNC_INSTANT_TRIM,     "inst-trim"  },
    { FUNC_RESET,            "reset"  },
    { FUNC_VARIO,            "vario"  },

    // mandatory last entry with default value
    { FUNC_MAX,              NULL   } // ???
};

static const struct YamlNode overrideChannelFn[] = {
    YAML_SIGNED(   "val",  16 ),
    YAML_PADDING(/* mode */ 8 ),
    YAML_UNSIGNED( "ch",    8 ),
    YAML_END
};

static const struct YamlNode trainerFn[] = {
    YAML_PADDING(/* val, mode */ 24 ),
    YAML_UNSIGNED( "ch",          8 ),
    YAML_END
};

static const struct YamlNode resetFn[] = {
    YAML_SIGNED( "val", 16 ),
    YAML_END
};

static bool is_override_ch(uint8_t* data)
{
    CustomFunctionData* cust_data =
        (CustomFunctionData*)(data - offsetof(CustomFunctionData,all));

    return cust_data->func == FUNC_OVERRIDE_CHANNEL;
}

static bool is_trainer(uint8_t* data)
{
    CustomFunctionData* cust_data =
        (CustomFunctionData*)(data - offsetof(CustomFunctionData,all));
    return cust_data->func == FUNC_TRAINER;
}

static bool is_reset(uint8_t* data)
{
    CustomFunctionData* cust_data =
        (CustomFunctionData*)(data - offsetof(CustomFunctionData,all));
    return cust_data->func == FUNC_RESET;
}

static const struct YamlNode custFnItems[] = {
    //YAML_IDX,
    YAML_SIGNED( "swtch",     9 ),
    YAML_ENUM(   "func",      7, custFn ),

    YAML_UNION(   "overrideCh", overrideChannelFn, is_override_ch ),
    YAML_UNION(   "trainer",    trainerFn, is_trainer ),
    YAML_UNION(   "reset",      resetFn, is_reset ),
    YAML_PADDING( 9<<3 ), // size of the union

    YAML_UNSIGNED("active",   8 ),
    YAML_END
};

static bool cust_fn_active(uint8_t* data)
{
    CustomFunctionData* cust_data = (CustomFunctionData*)data;
    return cust_data->swtch;
}

static const struct YamlNode modelItems[] = {
    YAML_STRING( "name", LEN_MODEL_NAME ),
    YAML_ARRAY(  "mixers", MixData,  MAX_MIXERS, mixerItems, mixer_active ),
    YAML_ARRAY(  "customFn", CustomFunctionData, MAX_SPECIAL_FUNCTIONS, custFnItems, cust_fn_active ),
    YAML_ARRAY(  "inputs", ExpoData, MAX_EXPOS,  inputItems, input_active ),
    YAML_END
};

const struct YamlNode modelNode = YAML_ROOT( modelItems );


#define GVAR_SMALL 128

static uint32_t in_read_weight(const char* val, uint8_t val_len)
{
    if ((val_len == 4)
        && (val[0] == '-')
        && (val[1] == 'G')
        && (val[2] == 'V')
        && (val[3] >= '1')
        && (val[3] <= '9')) {

        printf("%.*s -> %i\n", val_len, val, GVAR_SMALL - (val[3] - '0'));
        return GVAR_SMALL - (val[3] - '0'); // -GVx => 128 - x
    }

    if ((val_len == 3)
        && (val[0] == 'G')
        && (val[1] == 'V')
        && (val[2] >= '1')
        && (val[2] <= '9')) {

        printf("%.*s -> %i\n", val_len, val, -GVAR_SMALL + (val[2] - '1'));
        return -GVAR_SMALL + (val[2] - '1'); //  GVx => -128 + (x-1)
    }

    return (uint32_t)str2int(val, val_len);
}

static bool in_write_weight(uint32_t val, YamlNode::writer_func wf, void* opaque)
{
    int8_t sval = (int8_t)val;

    if (sval > GVAR_SMALL-11) {
        char n = GVAR_SMALL - sval + '0';
        if (!wf(opaque, "-GV", 3)
            || !wf(opaque, &n, 1)
            || !wf(opaque, "\r\n", 2))
            return false;
        return true;
    }
    else if (sval < -GVAR_SMALL+11) {
        char n = val - GVAR_SMALL + '1';
        if (!wf(opaque, "GV", 2)
            || !wf(opaque, &n, 1)
            || !wf(opaque, "\r\n", 2))
            return false;
        return true;
    }

    char* s = signed2str(val);
    if (!wf(opaque, s, strlen(s))
        || !wf(opaque, "\r\n", 2))
        return false;
    return true;
}

static bool mixer_active(uint8_t* data)
{
    struct MixData* mix_data = (struct MixData*)data;
    return mix_data->srcRaw != 0;
}

static bool input_active(uint8_t* data)
{
    struct ExpoData* expo_data = (struct ExpoData*)data;
    return expo_data->srcRaw != 0;
}
