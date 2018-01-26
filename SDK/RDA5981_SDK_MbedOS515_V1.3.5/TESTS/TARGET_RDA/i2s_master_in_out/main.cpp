#include "mbed.h"
#include "rda_i2s_api.h"

#define r_IOMUX_CTR1       (*((volatile uint32_t *)0x40001048))
#define r_CONTROL          (*((volatile uint32_t *)0x40001000))
#define r_CLK_GATE_CTRL3   (*((volatile uint32_t *)0x40000014))
#define r_PHY_SEL1         (*((volatile uint32_t *)0x40024004))

#define BUFFER_SIZE 240
uint32_t rdata[BUFFER_SIZE] = {0};
uint32_t tdata[BUFFER_SIZE] = {0xF000F000};

i2s_t obj = {0};
i2s_cfg_t cfg;

rtos::Thread i2s_rx_thread;
rtos::Thread i2s_tx_thread;

void rx_thread(void)
{
    while (true) {
        rda_i2s_int_recv(&obj, &rdata[0], BUFFER_SIZE);
        if (I2S_ST_BUSY == obj.sw_rx.state) {
            rda_i2s_sem_wait(i2s_rx_sem, osWaitForever);
        }
    }
}

void tx_thread(void)
{
    while (true) {
        rda_i2s_int_send(&obj, &tdata[0], BUFFER_SIZE);
        if (I2S_ST_BUSY == obj.sw_tx.state) {
            rda_i2s_sem_wait(i2s_tx_sem, osWaitForever);
        }
    }
}

int main() {
    printf("i2s_master_in test begin\n");

    for (uint32_t i = 1; i < BUFFER_SIZE; i++) {
        tdata[i] = tdata[i - 1] + 0x01000100;
    }

    cfg.mode              = I2S_MD_MASTER_RX;
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

    rda_i2s_init(&obj, I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD, NC, NC, I2S_RX_SD, PB_5);
    rda_i2s_set_ws(&obj, 16000, 256);
    rda_i2s_set_tx_channel(&obj, 2);
    rda_i2s_set_rx_channel(&obj, 2);
    rda_i2s_format(&obj, &cfg);

    rda_i2s_enable_rx(&obj);
    rda_i2s_enable_tx(&obj);
    //rda_i2s_out_mute(&obj);

    i2s_rx_thread.start(rx_thread);
    i2s_tx_thread.start(tx_thread);

    rtos::Thread::wait(osWaitForever);
}

