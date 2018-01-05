/*
 * 1.enable LWIP_RAW
*/

#include <stdio.h>
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/raw.h"
#include "lwip/timeouts.h"
#include "at.h"
#include "console.h"

extern u32_t os_time;
u32_t ping_delay, ping_data_size, ping_count;
unsigned char ping_console_idx;
int ping_echo_level;
#define PING_ID             0xAFAF
#define ICMP_HEAD_SIZE      (sizeof(struct icmp_echo_hdr))
#define HEAD_SIZE           (ICMP_HEAD_SIZE + PBUF_IP_HLEN)
static u16_t ping_seq_num;
static u32_t ping_time;
static struct raw_pcb *ping_pcb = NULL;
static ip_addr_t ping_dst;

#define PING_PRINT(fmt, ...) do {\
        if (ping_echo_level >= 1) {\
			if(ping_console_idx == 1) { \
            printf(fmt, ##__VA_ARGS__); } else { \
			console_uart0_printf(fmt, ##__VA_ARGS__); } \
        }\
    } while (0)
    

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

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;
    if(p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))){
        iecho = (struct icmp_echo_hdr*)((u8_t*)p->payload + PBUF_IP_HLEN);
        if((iecho->type == ICMP_ER) && (iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))){
            PING_PRINT("ping:recv %d bytes icmp_seq=%d %s time=%d ms\r\n", p->tot_len - HEAD_SIZE, ping_seq_num, inet_ntoa(*addr), sys_now()-ping_time);
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
    size_t ping_size = sizeof(struct icmp_echo_hdr) + ping_data_size;

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if(!p)
        return;

    if((p->len == p->tot_len) && (p->next == NULL)){
        iecho = (struct icmp_echo_hdr*)p->payload;
        ping_prepare_echo(iecho, (u16_t)ping_size);
        raw_sendto(raw, p, addr);

        ping_time = sys_now();
        PING_PRINT("ping:send %d bytes icmp_seq=%d %s\r\n", ping_size - ICMP_HEAD_SIZE, ping_seq_num, inet_ntoa(*addr));
    }
    pbuf_free(p);
}

static void ping_timeout(void *arg)
{
    struct raw_pcb *pcb = (struct raw_pcb*)arg;
    if (ping_seq_num < ping_count) {
        ping_send(pcb, &ping_dst);
        sys_timeout(ping_delay, ping_timeout, pcb);
    } else if (ping_seq_num == ping_count){
        ping_seq_num = 0;
        RESP_OK();
        raw_remove(ping_pcb);
        sys_untimeout(ping_timeout, pcb);
    }
}

static void ping_raw_init()
{
    ping_pcb = raw_new(IP_PROTO_ICMP);
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    sys_timeout(ping_delay, ping_timeout, ping_pcb);
}

void ping_init(const char *ip_dst, u32_t count, u32_t delay_time, u32_t len, u32_t echo_level)
{
    inet_aton(ip_dst, &ping_dst);
    ping_delay = delay_time;
    ping_data_size = len;
    ping_count = count;
    ping_echo_level = echo_level;
    ping_raw_init();
}

