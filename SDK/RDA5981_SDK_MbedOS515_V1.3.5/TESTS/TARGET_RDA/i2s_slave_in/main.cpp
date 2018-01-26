#include "mbed.h"
#include "rda_i2s_api.h"

#define BUFFER_SIZE 240
uint32_t rdata[BUFFER_SIZE] = {0};

i2s_t obj = {0};
i2s_cfg_t cfg;

rtos::Thread i2s_rx_thread;

void rx_thread(void)
{
    while (true) {
        rda_i2s_int_recv(&obj, &rdata[0], BUFFER_SIZE);
        if (I2S_ST_BUSY == obj.sw_rx.state) {
            rda_i2s_sem_wait(i2s_rx_sem, osWaitForever);
        }
    }
}

int main() {
    printf("i2s_slave_in test begin\n");

    cfg.mode              = I2S_MD_SLAVE_RX;
    cfg.rx.fs             = I2S_64FS;
    cfg.rx.ws_polarity    = I2S_WS_NEG;
    cfg.rx.std_mode       = I2S_STD_M;
    cfg.rx.justified_mode = I2S_RIGHT_JM;
    cfg.rx.data_len       = I2S_DL_16b;
    cfg.rx.msb_lsb        = I2S_MSB;

    rda_i2s_init(&obj, NC, NC, NC, I2S_RX_BCLK, I2S_RX_WS, I2S_RX_SD, NC);
    rda_i2s_set_rx_channel(&obj, 2);
    rda_i2s_format(&obj, &cfg);

    rda_i2s_enable_rx(&obj);

    i2s_rx_thread.start(rx_thread);

    rtos::Thread::wait(osWaitForever);
}

