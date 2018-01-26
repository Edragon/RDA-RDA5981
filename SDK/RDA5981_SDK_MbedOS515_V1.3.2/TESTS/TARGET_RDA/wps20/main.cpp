#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "wps_supplicant.h"

extern unsigned char rda_mac_addr[6];
extern void *app_msgQ;

void *key_sem = NULL;

InterruptIn key(PB_6); //for HDK of RDA5981A_HDK_V1.1

void key_up()
{
    //mbed_error_printf("Key Up\r\n");
}

void key_down()
{
	rda_sem_release(key_sem);
}

int main() {
    WiFiStackInterface wifi;
	const char *SSID = NULL;
    unsigned int msg;
    rda5981_scan_result *bss_list = NULL;
	rda5981_scan_result *bss_list1 = NULL;
	u8 i = 0;
	s32 ret;
    void *main_msgQ = rda_msgQ_create(5);
	struct wps_context *wps = NULL;
	key_sem = rda_sem_create(0);

    key.rise(&key_up);
    key.fall(&key_down);

	printf("start wps test!\r\n");
	
    wifi.set_msg_queue(main_msgQ);

	int scan_res = wifi.scan(SSID, 0);

    bss_list = (rda5981_scan_result *)malloc(scan_res * sizeof(rda5981_scan_result));
    memset(bss_list, 0, scan_res * sizeof(rda5981_scan_result));
    if (bss_list == NULL) {
        printf("malloc buf fail\r\n");
    }
	
    rda5981_get_scan_result(bss_list, scan_res);
    for (i=0; i<scan_res; ++i) {
        printf("ssid:%s:%s\r\n", bss_list[i].SSID, SSID);
     }
	
	printf("mac_addr: %x:%x:%x:%x:%x:%x\r\n", rda_mac_addr[0],rda_mac_addr[1],rda_mac_addr[2],\
				rda_mac_addr[3],rda_mac_addr[4],rda_mac_addr[5]);

	printf("wps test wait for key to be push down!\r\n");
    ret = rda_sem_wait(key_sem, osWaitForever);

	wps = wpas_wps_init(rda_mac_addr);

	if(wps==NULL) {
		printf("wpas_wps_init fail\r\n");
	}

	scan_res = wifi.scan(SSID, 0);
	u8 is_found = 0;
	u8 try_cnt = 1;
	do {
	    bss_list1 = (rda5981_scan_result *)malloc(scan_res * sizeof(rda5981_scan_result));
	    memset(bss_list1, 0, scan_res * sizeof(rda5981_scan_result));
	    if (bss_list1 == NULL) {
		   printf("malloc buf fail\r\n");
	    }
	    rda5981_get_scan_result(bss_list1, scan_res);
	    for (i=0; i<scan_res; i++) {	   
		   if(wpas_wps_ssid_wildcard_ok(bss_list1[i].ie, bss_list1[i].ie_length) == 1){
			   printf("wps registrar is found: channel=%d\r\n", bss_list1[i].channel);
			   printf("---ssid:%s, bssid=%x:%x:%x:%x:%x:%x\r\n", bss_list1[i].SSID, \
			   		bss_list1[i].BSSID[0],bss_list1[i].BSSID[1],bss_list1[i].BSSID[2],\
			   		bss_list1[i].BSSID[3],bss_list1[i].BSSID[4],bss_list1[i].BSSID[5]);
			   is_found = 1; 
			   memcpy(gssid, bss_list1[i].SSID, 32+1); 
			   gchannel = bss_list1[i].channel;
			   memcpy(gbssid,bss_list1[i].BSSID, 6);
		   	}
		}

		if(is_found == 1)
			break;
		else {
			free(bss_list1);
			wait_ms(1000);

			scan_res = wifi.scan(SSID, 0);
			try_cnt++;
			printf("try %d times\r\n", try_cnt);
		}
		
	} while(is_found == 0);

	int a = wps_connect(wps);

	while(1) {
        rda_msg_get(main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
			case WLAND_WPS_START:
				wps_sm_start(wps);
				printf("WLAND_WPS_START\r\n");
				break;
			case WLAND_RECONNECT:
				printf("WLAND_RECONNECT\r\n");
				memcpy(gssid, wps->ssid,wps->ssid_len);
				memcpy(gpass, wps->network_key, wps->network_key_len);
				wifi.connect((const char *)gssid, (const char *)gpass, NULL, NSAPI_SECURITY_NONE);
				break;
			default:
				break;
        }
	}
}
