#include "mbed.h"
#include "rda_i2s_api.h"
#include "pcm_1k.h"

i2s_t obj = {0};
i2s_cfg_t cfg;
rtos::Thread i2s_tx_thread;

#if defined (TBF)
mbed::DigitalOut PA_CTRL(GPIO_PIN0);
#endif

void tx_thread(void)
{
    while (true) {
        rda_i2s_int_send(&obj, (uint32_t *)&pcmData[0], PCM_SIZE / sizeof(uint32_t));
        if (I2S_ST_BUSY == obj.sw_tx.state) {
            rda_i2s_sem_wait(i2s_tx_sem, osWaitForever);
        }
    }
}

int main() {
    printf("i2s_master_out test begin\n");

    cfg.mode              = I2S_MD_MASTER_TX;
    cfg.tx.fs             = I2S_64FS;
    cfg.tx.ws_polarity    = I2S_WS_NEG;
    cfg.tx.std_mode       = I2S_STD_M;
    cfg.tx.justified_mode = I2S_RIGHT_JM;
    cfg.tx.data_len       = I2S_DL_16b;
    cfg.tx.msb_lsb        = I2S_MSB;
    cfg.tx.wrfifo_cntleft = I2S_WF_CNTLFT_8W;

    rda_i2s_init(&obj, I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD, NC, NC, NC, NC);
    rda_i2s_set_ws(&obj, 16000, 256);
    rda_i2s_set_tx_channel(&obj, 2);
    rda_i2s_format(&obj, &cfg);
#if defined (TBF)
    PA_CTRL = 1;
#endif
    rda_i2s_enable_tx(&obj);

    i2s_tx_thread.start(tx_thread);

    rtos::Thread::wait(osWaitForever);
}

