#include <soc-sim/_soc.h>

#define MBOX_SZ		32
#define MBOX_NO		8
#define MBOX_BSZ	(sizeof(void*) * MBOX_SZ)

#define CFG_MAC		{0x22,0x00,0x0a,0x90,0x92,0xb1}

#define CFG_MTU		1500
#define CFG_ETH_IRQ	IRQ_ETH
#define CFG_ETH_PRI	5
#define CFG_ETH_STACK	2024

