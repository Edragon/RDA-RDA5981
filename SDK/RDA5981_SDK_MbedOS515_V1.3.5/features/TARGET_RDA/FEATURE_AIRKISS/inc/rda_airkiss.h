#ifndef AIRKISS_H_
#define AIRKISS_H_

//#define RDA_AIRKISS_DEBUG

typedef enum {
    AIRKISS_STATE_STOPED = 0,
    AIRKISS_STATE_IDLE,
    AIRKISS_STATE_SRC_LOCKED,
    AIRKISS_STATE_MAGIC_CODE_COMPLETE,
    AIRKISS_STATE_PREFIX_CODE_COMPLETE,
    AIRKISS_STATE_COMPLETE
} AIR_KISS_STATE;

#define MAX_GUIDE_RECORD    4
typedef struct {
    unsigned short  length_record[MAX_GUIDE_RECORD + 1];
    unsigned short  length_record_ldpc[MAX_GUIDE_RECORD + 1];
} guide_code_record;

#define MAX_MAGIC_CODE_RECORD    4
typedef struct {
    unsigned short record[MAX_MAGIC_CODE_RECORD + 1];
} magic_code_record;

#define MAX_PREFIX_CODE_RECORD    4
typedef struct {
    unsigned short record[MAX_PREFIX_CODE_RECORD + 1];
} prfix_code_record;

#define MAX_SEQ_CODE_RECORD    6
typedef struct {
    unsigned short record[MAX_SEQ_CODE_RECORD + 1];
} seq_code_record;

#define PASSWORD_MAX_LEN          32
#define ESSID_MAX_LEN             32
#define USR_DATA_BUFF_MAX_SIZE    (PASSWORD_MAX_LEN + 1 + ESSID_MAX_LEN)
#define AIRKISS_RESPON_PORT       10000

union airkiss_data {
    guide_code_record guide_code;
    magic_code_record magic_code;
    prfix_code_record prefix_code;
    seq_code_record   seq_code;
};

typedef struct {
    char* pwd;
    char* ssid;
    unsigned char pswd_len;
    unsigned char ssid_len;
    unsigned char random_num;
    unsigned char ssid_crc; //reserved used as ssid_crc
    //above is airkiss_context_t
    unsigned char usr_data[USR_DATA_BUFF_MAX_SIZE];
    AIR_KISS_STATE airkiss_state;
    unsigned char src_mac[6];
    unsigned char need_seq;
    unsigned char base_len;
    unsigned char total_len;
    unsigned char pswd_lencrc;
    unsigned char recv_len;
    unsigned short seq_success_map;
    unsigned short seq_success_map_cmp;
    char ldpc;
    union airkiss_data data;
} wland_airkiss;

/*
 * TODO: support AIRKISS_ENABLE_CRYPT
 */
#ifndef AIRKISS_ENABLE_CRYPT
#define AIRKISS_ENABLE_CRYPT 0
#endif

typedef struct {
    int dummyap[26];
    int dummy[32];
} airkiss_context_t;

typedef enum {
    AIRKISS_STATUS_CONTINUE = 0,

    /* wifi channel is locked */
    AIRKISS_STATUS_CHANNEL_LOCKED = 1,

    /* get result success*/
    AIRKISS_STATUS_COMPLETE = 2

} airkiss_status_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * function: get airkiss version
 */
const char* airkiss_version(void);

/*
 * function: get ssid and pw from airkiss
 * return: 0:success, else:fail
 */
int get_ssid_pw_from_airkiss(char *ssid, char *pw);
int rda5981_stop_airkiss();

/*
 * function: send response to host
 * @stack: wifi stack
 * return: 0:success, else:fail
 */

int airkiss_sendrsp_to_host(void *stack);

#ifdef __cplusplus
}
#endif

#endif

