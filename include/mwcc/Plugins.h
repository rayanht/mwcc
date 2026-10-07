#include "compiler/common.h"
#include "driver/CLCompilerLinkerDropin_V10.h"
#ifndef MWCC_PLUGINS_H
#define MWCC_PLUGINS_H

/* The driver's plugin objects (C++: CLDropinCallbacks_V10.cpp, CLCompilerLinkerDropin_V10.cpp, CLPluginRequests.cpp).
 * CLPluginRequests creates one per plugin by its type code: 'cldr' a PluginA, 'Pars' a PluginB, 'Comp'/'Link' a
 * PluginC; the class names are ours. */

extern "C" {
}

#pragma options align=mac68k
class PluginA {
public:
    PluginA(UInt32 code, int size);
    /* (user-declared: PluginC's constructor, which calls new after constructing
       its base, keeps `this` in a stack slot) */
    ~PluginA();
    UInt8 pad0[0x9c];
    UInt32 sig;         /* 0x9c */
    UInt32 code;        /* 0xa0 */
    UInt8 pada4[0xc];
    void *fb0;          /* 0xb0 */
    UInt8 padb4[0x50];
    void *f104;         /* 0x104 */
};

class PluginB : public PluginA {
public:
    PluginB();
    UInt32 first;                 /* 0x108 */
    UInt32 second;                /* 0x10c */
    UInt32 third;                 /* 0x110 */
    UInt8 intermediateState[8];   /* 0x114 */
    UInt32 fourth;                /* 0x11c */
    UInt32 fifth;                 /* 0x120 */
    UInt32 sixth;                 /* 0x124 */
    UInt32 seventh;               /* 0x128 */
    UInt32 eighth;                /* 0x12c */
    UInt32 ninth;                 /* 0x130 */
    void *dataReference;          /* 0x134 */
};

class PluginC : public PluginA {
public:
    PluginC();
    SInt32 initialValue;          /* 0x108 */
    UInt8 initialStorage[0x46];   /* 0x10c */
    SInt32 value1;                /* 0x152 */
    SInt32 value2;                /* 0x156 */
    UInt8 byte1;                  /* 0x15a */
    UInt8 byte2;
    UInt8 byte3;
    UInt8 byte4;
    UInt8 byte5;
    UInt8 opaqueByte;
    UInt16 shortValue;            /* 0x160 */
    UInt8 shortStorage[0x10];     /* 0x162 */
    UInt16 opaqueShort;           /* 0x172 */
    SInt32 value3;                /* 0x174 */
    SInt32 value4;
    SInt32 value5;
    SInt32 value6;
    UInt8 opaqueMiddle[0x1c];     /* 0x184 */
    SInt32 value7;                /* 0x1a0 */
    UInt8 trailingStorage[0xa2];  /* 0x1a4 */
    InitializationAuxiliaryState *auxiliary; /* 0x246 */
    UInt8 opaqueTail[0x1c];       /* 0x24a */
    void **callbacks;             /* 0x266 */
};
#pragma options align=reset

#endif
