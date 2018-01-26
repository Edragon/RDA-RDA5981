#include "mbed.h"
#include "rda_i2s_api.h"

#define BUFFER_SIZE 240

i2s_t obj = {0};
i2s_cfg_t cfg;

typedef struct {
    uint32_t *pbuff;
    uint32_t length;
} message_t;

rtos::Queue<message_t, 10> queue;
rtos::MemoryPool<message_t, 10> mpool;
rtos::Thread i2s_tx_thread;
rtos::Thread i2s_rx_thread;

#if defined (TBF)
mbed::DigitalOut PA_CTRL(GPIO_PIN0);
#endif

void tx_thread(void)
{
    while (true) {
        osEvent evt = queue.get(osWaitForever);
        if (evt.status == osEventMessage) {
            message_t *message = (message_t*)evt.value.p;
            if (message->pbuff) {
                rda_i2s_int_send(&obj, message->pbuff, message->length);
                if (I2S_ST_BUSY == obj.sw_tx.state) {
                    rda_i2s_sem_wait(i2s_tx_sem, osWaitForever);
                }
                free(message->pbuff);
            }
            mpool.free(message);
        }
    }
}

void rx_thread(void)
{
    while (true) {
        message_t *message = mpool.calloc();
        if (NULL != message) {
            message->pbuff = (uint32_t *) malloc(sizeof(uint32_t) * BUFFER_SIZE);
            if (NULL != message->pbuff) {
                message->length = BUFFER_SIZE;
                if (!i2s_rx_enabled) {
                    rda_i2s_enable_rx(&obj);
                }
                rda_i2s_int_recv(&obj, message->pbuff, message->length);
                if (I2S_ST_BUSY == obj.sw_rx.state) {
                    rda_i2s_sem_wait(i2s_rx_sem, osWaitForever);
                }
                queue.put(message);
            } else {
                printf("pbuff malloc fail\n");
                if (i2s_rx_enabled) {
                    rda_i2s_disable_rx(&obj);
                }
            }
        } else {
            printf("mpool calloc fail\n");
            if (i2s_rx_enabled) {
                rda_i2s_disable_rx(&obj);
            }
            rtos::Thread::wait(50);
        }
    }
}

int main() {
    printf("i2s_master_out_slave_in test begin\n");

    cfg.mode              = I2S_MD_M_TX_S_RX;
    cfg.rx.fs             = I2S_64FS;
    cfg.rx.ws_polarity    = I2S_WS_NEG;
    cfg.rx.std_mode       = I2S_STD_M;
    cfg.rx.justified_mode = I2S_RIGHT_JM;
    cfg.rx.data_len       = I2S_DL_16b;
    cfg.rx.msb_lsb        = I2S_MSB;

    cfg.tx.fs             = I2S_64FS;
    cfg.tx.ws_polarity    = I2S_WS_NEG;
    cfg.tx.std_mode       = I2S_STD_M;
    cfg.tx.justified_mode = I2S_RIGHT_JM;
    cfg.tx.data_len       = I2S_DL_16b;
    cfg.tx.msb_lsb        = I2S_MSB;
    cfg.tx.wrfifo_cntleft = I2S_WF_CNTLFT_8W;

    rda_i2s_init(&obj, I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD, I2S_RX_BCLK, I2S_RX_WS, I2S_RX_SD, NC);
    rda_i2s_set_ws(&obj, 16000, 256);
    rda_i2s_set_tx_channel(&obj, 2);
    rda_i2s_set_rx_channel(&obj, 2);
    rda_i2s_format(&obj, &cfg);
#if defined (TBF)
    PA_CTRL = 1;
#endif
    rda_i2s_enable_rx(&obj);
    rda_i2s_enable_tx(&obj);

    i2s_rx_thread.start(rx_thread);
    i2s_tx_thread.start(tx_thread);

    rtos::Thread::wait(osWaitForever);
}

