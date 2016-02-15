#ifndef _CC06292014
#define _CC06292014

typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned int   u32_t;

typedef char  s8_t;
typedef short s16_t;
typedef int   s32_t;

typedef unsigned mem_ptr_t;

#define U16_F "hu"
#define S16_F "d"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "uz"

#define BYTE_ORDER LITTLE_ENDIAN

#define LWIP_PLATFORM_BYTESWAP 0
#define LWIP_CHKSUM_ALGORITHM	2

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#include <hcos/dbg.h>
void sys_print(char* s);

#define LWIP_PLATFORM_ASSERT(_s)         \
	do{ sys_print(_s);_assert(0);}while(0)

#endif

