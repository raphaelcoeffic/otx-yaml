#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "model.h"
#include "yaml_node.h"
#include "yaml_parser.h"
#include "yaml_bits.h"

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

// static bool mixer_active(uint8_t* data)
// {
//     struct MixData* mix_data = (struct MixData*)data;
//     return mix_data->srcRaw != 0;
// }

// static bool input_active(uint8_t* data)
// {
//     struct ExpoData* expo_data = (struct ExpoData*)data;
//     return expo_data->srcRaw != 0;
// }

static bool is_custFnData_play(uint8_t* data)
{
    CustomFunctionData* cust_data =
        (CustomFunctionData*)(data - offsetof(CustomFunctionData,all));
    return cust_data->func == FUNC_PLAY_SOUND
        || cust_data->func == FUNC_PLAY_TRACK
        || cust_data->func == FUNC_PLAY_SCRIPT;
}

static bool is_custFnData_all(uint8_t* data)
{
    CustomFunctionData* cust_data =
        (CustomFunctionData*)(data - offsetof(CustomFunctionData,all));
    return !yaml_is_zero(data, sizeof(CustomFunctionData::all)<<3)
        && !is_custFnData_play(data)
        && (cust_data->func != FUNC_INSTANT_TRIM);
}

static bool is_custFnData_clear(uint8_t* data)
{
    return false;
}

// static bool cust_fn_active(uint8_t* data)
// {
//     CustomFunctionData* cust_data = (CustomFunctionData*)data;
//     return cust_data->swtch;
// }

// static bool string_active(uint8_t* data)
// {
//     return *data != '\0';
// }

#if defined(MANUAL_STRUCTS)

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

static const struct YamlIdStr custFn[] = {
    { FUNC_OVERRIDE_CHANNEL, "override-ch" },
    { FUNC_TRAINER,          "trainer"  },
    { FUNC_INSTANT_TRIM,     "inst-trim"  },
    { FUNC_RESET,            "reset"  },
    { FUNC_VARIO,            "vario"  },

    // mandatory last entry with default value
    { FUNC_MAX,              NULL   } // ???
};

static const struct YamlNode custFnData_play[] = {
    YAML_STRING( "name", LEN_FUNCTION_NAME ),
    YAML_END
};

static const struct YamlNode custFnData_all[] = {
    YAML_SIGNED(   "val",  16 ),
    YAML_UNSIGNED( "mode",  8 ),
    YAML_UNSIGNED( "param", 8 ),
    YAML_END
};

static const struct YamlNode custFnData_clear[] = {
    YAML_SIGNED(   "val1", 32 ),
    YAML_SIGNED(   "val2", 16 ),
    YAML_END
};

static const struct YamlNode anon_union_0[] = {
    YAML_STRUCT( "play",  CustomFunctionData::play,  custFnData_play, is_custFnData_play ),
    YAML_STRUCT( "all",   CustomFunctionData::all,    custFnData_all, is_custFnData_all ),
    YAML_STRUCT( "clear", CustomFunctionData::clear, custFnData_clear, is_custFnData_clear ),
    YAML_END
};

static const struct YamlNode custFnItems[] = {
    YAML_IDX,
    YAML_SIGNED(  "swtch",     9 ),
    YAML_ENUM(    "func",      7, custFn ),
    YAML_UNION(   "",         48, anon_union_0, NULL),
    YAML_UNSIGNED("active",    8 ),
    YAML_END
};

static const struct YamlNode struct_unsigned_8[] = {
  YAML_UNSIGNED( "val", 8 ),
  YAML_END
};

static const struct YamlNode struct_ModelHeader[] = {
  YAML_STRING("name", 15),
  YAML_ARRAY("modelId", 8, 2, struct_unsigned_8, NULL),
  YAML_STRING("bitmap", 10),
  YAML_END
};

static const struct YamlNode struct_string_32[] = {
  YAML_IDX,
  YAML_STRING("val", 4),
  YAML_END
};

static const struct YamlNode modelItems[] = {
    YAML_STRUCT( "header", ModelHeader, struct_ModelHeader, NULL),
    YAML_ARRAY(  "mixers", MixData,  MAX_MIXERS, mixerItems, NULL /*mixer_active*/ ),
    YAML_ARRAY(  "customFn", CustomFunctionData, MAX_SPECIAL_FUNCTIONS, custFnItems, NULL /*cust_fn_active*/ ),
    YAML_ARRAY(  "inputNames", char[4], 32, struct_string_32, NULL /*string_active*/),
    YAML_ARRAY(  "inputs", ExpoData, MAX_EXPOS,  inputItems, NULL /*input_active*/),
    YAML_END
};

const struct YamlNode modelNode = YAML_ROOT( modelItems );

#else

#include "model_gen.cpp"
const struct YamlNode modelNode = YAML_ROOT( struct_Model );

#endif

