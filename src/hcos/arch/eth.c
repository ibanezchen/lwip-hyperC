#include <stdio.h>
#include <string.h>

#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/stats.h>
#include <netif/etharp.h>

#define _DBG 0
#include <hcos/dbg.h>
#include <hcos/core.h>
#include <hcos/task.h>
#include <hcos/irq.h>
#include <hcos/sem.h>
#include <hcos/io.h>
#include <hcos/cpu/gic.h>
#include <hcos/soc.h>

#include "_soc.h"
#include "soc-cfg.h"

#define BUFSZ		2048

#define R_TX_BUF0	((void*)BASE_ETH+0)
#define R_TX_LEN0	((void*)BASE_ETH+0x07f4)
#define R_TX_GIE0	((void*)BASE_ETH+0x07f8)
#define R_TX_CTRL0	((void*)BASE_ETH+0x07fc)
#define R_TX_BUF1	((void*)BASE_ETH+0x0800)
#define R_TX_LEN1	((void*)BASE_ETH+0x0ff4)
#define R_TX_CTRL1	((void*)BASE_ETH+0x0ffc)

#define R_RX_BUF0	((void*)BASE_ETH+0x1000)
#define R_RX_CTRL0	((void*)BASE_ETH+0x17fc)
#define R_RX_BUF1	((void*)BASE_ETH+0x1800)
#define R_RX_CTRL1	((void*)BASE_ETH+0x1ffc)
#define R_RX_LEN0	((void*)BASE_ETH+0x2000)
#define R_RX_LEN1	((void*)BASE_ETH+0x2004)
#define R_MAX		((void*)BASE_ETH+0x2008)

typedef struct{
	sem_t  sem;
	task_t task;
	unsigned rlen;
	unsigned* bo;
	unsigned* bi;
}eth_t;

static struct netif* _netif;

static void eth_taskf(void* priv)
{
	struct netif* netif = _netif;
	eth_t* eth = (eth_t*)(netif->state);
	struct pbuf *p, *q;
	unsigned r, l;
	dbg("eth: taskf\n");
	while((r = sem_get(&eth->sem, WAIT)) == 0){
		dbg("==eth_get\n");
		p = pbuf_alloc(PBUF_RAW, eth->rlen + ETH_PAD_SIZE, PBUF_POOL);
		if(!p){
			LINK_STATS_INC(link.memerr);
			LINK_STATS_INC(link.drop);
			continue;
		}
		pbuf_header(p, -ETH_PAD_SIZE);
		l = 0;
		for (q = p; q != 0; q = q->next) {
			memcpy((u8_t*)q->payload, ((void*)(eth->bi))+l, q->len);
			l = l + q->len;
		}
		pbuf_header(p, ETH_PAD_SIZE);
		LINK_STATS_INC(link.recv);
		netif->input(p, netif);
	}
	dbg("eth: taskf_exit!!! %d\n", r);
}

static irq_handler(eth_irq)
{
	struct netif* netif = _netif;
	eth_t* eth = (eth_t*)(netif->state);
	unsigned ctrl, i, nw;

	dbg("eth: irq\n");
	if((ctrl = readl(R_RX_CTRL0) & 1) == 0)
		return 0;
	eth->rlen = readl(R_RX_LEN0);
	if(!eth->rlen)
		return 0;
	dbg("eth: in %d\n", eth->rlen);
	memset(eth->bi, 0, BUFSZ);
	nw = (eth->rlen + 3)>>2;
	for( i = 0 ; i < nw ; i++)
		eth->bi[i] = readl(R_RX_BUF0+(i<<2));
#if _DBG
	for(i = 0 ; i < eth->rlen ; i++){
		if(!(i%16))
			dbg("\n");
		dbg("%02x ", ((char*)bi)[i]);
	}
	dbg("\n");
#endif
	writel(8|(ctrl&~1), R_RX_CTRL0);
	dbg("eth: sem_post\n");
	sem_post(&eth->sem);
	irq_eoi(irq);
	return IRQ_DONE;
}

static err_t eth_out(struct netif *netif, struct pbuf *p)
{
	eth_t* eth = (eth_t*)(netif->state);
	struct pbuf *q;
	unsigned ctrl, i, l, nw;
	l = 0;
	memset(eth->bo, 0, BUFSZ);
	pbuf_header(p, -ETH_PAD_SIZE);
	for (q = p; q != 0; q = q->next) {
		memcpy(((void*)eth->bo)+l, (u8_t*) q->payload, q->len);
		l += q->len;
	}
	pbuf_header(p, ETH_PAD_SIZE);
#if _DBG
	for(i = 0 ; i < l ; i++){
		if(!(i%16))
			dbg("\n");
		dbg("%02x ", ((char*)bo)[i]);
	}
	dbg("\n");
#endif
	dbg("eth: out %d\n", l);
	// hw allows word access only
	nw = (l+3)>>2;
	for(i = 0 ; i < nw ; i ++)
		writel(eth->bo[i], R_TX_BUF0+(i<<2));
	writel(l, R_TX_LEN0);
	ctrl = readl(R_TX_CTRL0);
	writel(ctrl | 0x1, R_TX_CTRL0);
	while((readl(ctrl) & 1))
		;
	LINK_STATS_INC(link.xmit);
	return ERR_OK;
}

static void eth_init(char* mac)
{
	unsigned macw[2];
	// enable global interrupt
	writel(0x80000000, R_TX_GIE0);
	// enable rx interrupt
	writel(0x8,  R_RX_CTRL0);
	// program mac addr
	memset(macw, 0, sizeof(macw));
	memcpy(macw, mac, 6);
	writel(macw[0], R_TX_BUF0+0);
	writel(macw[1], R_TX_BUF0+4);
	writel(readl(R_TX_CTRL0) | 0x3, R_TX_CTRL0);
}

err_t ethernetif_init(struct netif *netif) 
{
	char mac[]= CFG_MAC;
	eth_t *eth= _alloc(sizeof(eth_t));
#if LWIP_NETIF_HOSTNAME
	netif->hostname = "lwip";
#endif
	eth->bo = _alloc(BUFSZ);
	eth->bi = _alloc(BUFSZ);
	
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);
	netif->state = eth;
	netif->name[0] = 'h';
	netif->name[1] = 'c';
	netif->output = etharp_output;
	netif->linkoutput = eth_out;
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	memcpy(netif->hwaddr, mac, sizeof(mac));
	netif->mtu = CFG_MTU;
	netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
	_netif = netif;
	irq_init(CFG_ETH_IRQ, eth_irq);
	sem_init(&eth->sem, 0);
	task_init(
		&eth->task, 
		"eth", 
		eth_taskf, 
		CFG_ETH_PRI, 
		_alloc(CFG_ETH_STACK), 
		CFG_ETH_STACK, 
		-1,
		netif);
	eth->rlen = 0;
	eth_init(mac);
	return ERR_OK;
}

