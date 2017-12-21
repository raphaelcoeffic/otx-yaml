#ifndef _model_h_
#define _model_h_

#include <stdint.h>

#define PACK( __Declaration__ )      __Declaration__ __attribute__((__packed__))

#define LEN_EXPOMIX_NAME  6
#define LEN_MODEL_NAME   10

#define MAX_MIXERS  4
#define MAX_EXPOS   8

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

PACK(struct CurveRef {
  uint8_t type;
  int8_t  value;
});

PACK(struct MixData {
  int16_t  weight:11;       // GV1=-1024, -GV1=1023
  uint16_t destCh:5;
  uint16_t srcRaw:10;       // srcRaw=0 means not used
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
});

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

PACK(struct Model {
    char     name[LEN_MODEL_NAME];
    MixData  mixData[MAX_MIXERS];
    ExpoData expoData[MAX_EXPOS];
});

extern const struct YamlNode modelNode;

#endif
