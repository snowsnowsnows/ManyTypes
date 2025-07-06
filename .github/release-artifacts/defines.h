typedef          __int64 ll;
typedef unsigned __int64 ull;

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

typedef          char   int8;
typedef   signed char   sint8;
typedef unsigned char   uint8;
typedef          short  int16;
typedef   signed short  sint16;
typedef unsigned short  uint16;
typedef          int    int32;
typedef   signed int    sint32;
typedef unsigned int    uint32;
typedef ll              int64;
typedef ll              sint64;
typedef ull             uint64;

#define _BYTE  uint8
#define _WORD  uint16
#define _DWORD uint32
#define _QWORD uint64
#if !defined(_MSC_VER)
#define _LONGLONG __int128
#endif

typedef int8 _BOOL1;
typedef int16 _BOOL2;
typedef int32 _BOOL4;

#ifndef _WINDOWS_
typedef int8 BYTE;
typedef int16 WORD;
typedef int32 DWORD;
typedef int32 LONG;
typedef int BOOL;       // uppercase BOOL is usually 4 bytes
#endif
typedef int64 QWORD;
#ifndef __cplusplus
typedef int bool;       // we want to use bool in our C programs
#endif

#ifndef NULL
#define NULL 0
#endif

#define _UNKNOWN char

#define MT_PACKED_ALIGN(x) __attribute__((packed, aligned(x)))
#define MT_ALIGN(x) __attribute__((aligned(x)))