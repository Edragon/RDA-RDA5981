/*
 * 1.enable LWIP_DEBUG
 * 2.enable LWIP_RAW
*/

#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/inet.h"

#define PING_DEBUG          LWIP_DBG_ON

#define PING_DELAY          1000
#define PING_ID             0xAFAF
#define PING_DATA_SIZE      32
static u16_t ping_seq_num;
static u32_t ping_time;
static struct raw_pcb *ping_pcb = NULL;
static ip_addr_t ping_dst;

static void ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    iecho->seqno = htons(++ping_seq_num);

    for(i=0; i<data_len; i++)
        ((char*)iecho)[sizeof(struct icmp_echo_hdr)+1]=(char)i;

    iecho->chksum = inet_chksum(iecho, len);
}

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;

    if(p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))){
        iecho = (struct icmp_echo_hdr*)((u8_t*)p->payload + PBUF_IP_HLEN);
        if((iecho->type == ICMP_ER) && (iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))){
            LWIP_DEBUGF(PING_DEBUG, ("ping:recv "));
            ip_addr_debug_print(PING_DEBUG, addr);
            LWIP_DEBUGF(PING_DEBUG, ("time=%"U32_F"ms\n" , sys_now()-ping_time));
            pbuf_free(p);
            return 1;
        }
    }
    return 0;
}

static void ping_send(struct raw_pcb *raw, ip_addr_t *addr)
{
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if(!p)
        return;

    if((p->len == p->tot_len) && (p->next == NULL)){
        iecho = (struct icmp_echo_hdr*)p->payload;
        ping_prepare_echo(iecho, (u16_t)ping_size);
        raw_sendto(raw, p, addr);

        ping_time = sys_now();
        LWIP_DEBUGF(PING_DEBUG, (("ping:[%"U32_F"]send "), ping_seq_num));
        ip_addr_debug_print(PING_DEBUG, addr);
        LWIP_DEBUGF(PING_DEBUG, ("\n"));
    }
    pbuf_free(p);
}

static void ping_timeout(void *arg)
{
    struct raw_pcb *pcb = (struct raw_pcb*)arg;
    ping_send(pcb, &ping_dst);
    sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static void ping_raw_init()
{
    ping_pcb = raw_new(IP_PROTO_ICMP);
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

void ping_init(const char *ip_dst)
{
    inet_aton(ip_dst, &ping_dst);
    ping_raw_init();
}

