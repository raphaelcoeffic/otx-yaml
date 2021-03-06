#ifndef _model_h_
#define _model_h_

#include <stdint.h>

#define PACK( __Declaration__ )      __Declaration__ __attribute__((__packed__))

#if defined(YAML_GENERATOR)

/* private definitions */
#define _yaml_note(label) #label
#define _yaml_attribute(attr) __attribute__((annotate(attr)))

/* public definitions */
#define ENUM(label) _yaml_attribute("enum:" _yaml_note(label))
#define USE_IDX     _yaml_attribute("idx:true")
#define FUNC(name)  _yaml_attribute("func:" _yaml_note(name))
#define CUST(read,write)                        \
  _yaml_attribute("read:" _yaml_note(read))     \
  _yaml_attribute("write:" _yaml_note(write))

#else

#define ENUM(label)
#define USE_IDX
#define FUNC(name)
#define CUST(read,write)

#endif


#define LEN_EXPOMIX_NAME   6
#define LEN_MODEL_NAME    10
#define LEN_FUNCTION_NAME  6

#define MAX_MIXERS            4
#define MAX_EXPOS             8
#define MAX_SPECIAL_FUNCTIONS 8

enum MixSources {
    MIXSRC_NONE=0,
    MIXSRC_Rud,
    MIXSRC_Ele,
    MIXSRC_Thr,
    MIXSRC_Ail,
    MIXSRC_S1,
    MIXSRC_6POS,
    MIXSRC_S2
};

enum Functions {
  // first the functions which need a checkbox
  FUNC_OVERRIDE_CHANNEL,
  FUNC_TRAINER,
  FUNC_INSTANT_TRIM,
  FUNC_RESET,
  FUNC_SET_TIMER,
  FUNC_ADJUST_GVAR,
  FUNC_VOLUME,
  FUNC_SET_FAILSAFE,
  FUNC_RANGECHECK,
  FUNC_BIND,
  // then the other functions
  FUNC_FIRST_WITHOUT_ENABLE,
  FUNC_PLAY_SOUND = FUNC_FIRST_WITHOUT_ENABLE,
  FUNC_PLAY_TRACK,
  FUNC_PLAY_VALUE,
  FUNC_RESERVE4,
  FUNC_PLAY_SCRIPT,
  FUNC_RESERVE5,
  FUNC_BACKGND_MUSIC,
  FUNC_BACKGND_MUSIC_PAUSE,
  FUNC_VARIO,
  FUNC_HAPTIC,
  FUNC_LOGS,
  FUNC_BACKLIGHT,
#if defined(PCBTARANIS)
  FUNC_SCREENSHOT,
#endif
#if defined(DEBUG)
  FUNC_TEST, // should remain the last before MAX as not added in Companion
#endif
  FUNC_MAX
};

PACK(struct CurveRef {
  uint8_t type;
  int8_t  value;
});

PACK(struct MixData {
  int16_t  weight:11 CUST(in_read_weight,in_write_weight); // GV1=-1024, -GV1=1023
  uint16_t destCh:5;
  uint16_t srcRaw:10 ENUM(MixSources);       // srcRaw=0 means not used
  uint16_t carryTrim:1;
  uint16_t mixWarn:2;       // mixer warning
  uint16_t mltpx:2;         // multiplex method: 0 means +=, 1 means *=, 2 means :=
  uint16_t spare:1;
  int32_t  offset:14;
  int32_t  swtch:9;
  uint32_t flightModes:9;
  CurveRef curve;
  uint8_t  delayUp;
  uint8_t  delayDown;
  uint8_t  speedUp;
  uint8_t  speedDown;
  char     name[LEN_EXPOMIX_NAME];
}) USE_IDX;

PACK(struct ExpoData {
  uint16_t mode:2;
  uint16_t scale:14;
  uint16_t srcRaw:10;
  int16_t  carryTrim:6;
  uint32_t chn:5;
  int32_t  swtch:9;
  uint32_t flightModes:9;
  int32_t  weight:8;
  int32_t  spare:1;
  char     name[LEN_EXPOMIX_NAME];
  int8_t   offset;
  CurveRef curve;
});

PACK(struct CustomFunctionData {
  int16_t  swtch:9;
  uint16_t func:7;
  PACK(union {
    PACK(struct {
      char name[LEN_FUNCTION_NAME];
    }) play FUNC(is_custFnData_play);

    PACK(struct {
      int16_t val;
      uint8_t mode;
      uint8_t param;
      int16_t spare;
    }) all FUNC(is_custFnData_all);

    PACK(struct {
      int32_t val1;
      int16_t val2;
    }) clear FUNC(is_custFnData_clear);
  });
  uint8_t active;
}) USE_IDX;

#define LEN_SCRIPT_FILENAME            6
#define LEN_SCRIPT_NAME                6
#define MAX_SCRIPT_INPUTS              6
#define MAX_SCRIPTS                    9

typedef uint16_t source_t;

union ScriptDataInput {
  int16_t value;
  source_t source;
};

PACK(struct ScriptData {
  char            file[LEN_SCRIPT_FILENAME];
  char            name[LEN_SCRIPT_NAME];
  ScriptDataInput inputs[MAX_SCRIPT_INPUTS];
});

#define NUM_MODULES 2

PACK(struct ModelHeader {
  char      name[LEN_MODEL_NAME]; // must be first for eeLoadModelName
  uint8_t   modelId[NUM_MODULES];
});

PACK(struct Model {
    ModelHeader        header;
    MixData            mixData[MAX_MIXERS];
    CustomFunctionData customFn[MAX_SPECIAL_FUNCTIONS];
    char               inputNames[4][32] USE_IDX;
    ExpoData           expoData[MAX_EXPOS];
    ScriptData scriptsData[MAX_SCRIPTS];
});

extern const struct YamlNode modelNode;

#endif
