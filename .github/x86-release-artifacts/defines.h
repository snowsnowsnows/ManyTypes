typedef __int64 ll;
typedef unsigned __int64 ull;

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;

typedef char int8;
typedef char int8_t;
typedef signed char sint8;
typedef signed char sint8_t;
typedef unsigned char uint8;
typedef unsigned char uint8_t;
typedef short int16;
typedef short int16_t;
typedef signed short sint16;
typedef signed short sint16_t;
typedef unsigned short uint16;
typedef unsigned short uint16_t;
typedef int int32;
typedef int int32_t;
typedef signed int sint32;
typedef signed int sint32_t;
typedef unsigned int uint32;
typedef unsigned int uint32_t;
typedef long long int64;
typedef long long int64_t;
typedef long long sint64;
typedef long long sint64_t;
typedef unsigned long long uint64;
typedef unsigned long long uint64_t;

typedef int8 _BOOL1;
typedef int16 _BOOL2;
typedef int32 _BOOL4;

typedef int8 BYTE;
typedef int16 WORD;
typedef int32 DWORD;
typedef int32 LONG;
typedef int BOOL;
typedef int64 QWORD;

#define MT_PACKED_ALIGN(x) __attribute__((packed, aligned(x)))
#define MT_ALIGN(x) __attribute__((aligned(x)))