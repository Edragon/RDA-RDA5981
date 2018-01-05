#include "device_controller.h"
#include "baidu_ca_scheduler.h"

using duer::Scheduler;

static bca_status_t get_put_duer_switch(bca_context ctx, bca_msg_t *msg, bca_addr_t *addr)
{
    bca_handler handler = (bca_handler)ctx;
    int msg_code;
    if (handler && msg) {
        if (msg->msg_code == BCA_MSG_REQ_GET) {
            msg_code = BCA_MSG_RSP_CONTENT;
        } else if (msg->msg_code == BCA_MSG_REQ_PUT) {
            msg_code = BCA_MSG_RSP_CHANGED;
        }
        Scheduler::instance().response(msg, msg_code, NULL);
    }
    return BCA_NO_ERR;
}

static bca_status_t get_put_duer_rgb(bca_context ctx, bca_msg_t *msg, bca_addr_t *addr)
{
    bca_handler handler = (bca_handler)ctx;
    int msg_code;
    if (handler && msg) {
        if (msg->msg_code == BCA_MSG_REQ_GET) {
            msg_code = BCA_MSG_RSP_CONTENT;
        } else if (msg->msg_code == BCA_MSG_REQ_PUT) {
            msg_code = BCA_MSG_RSP_CHANGED;
        }
        Scheduler::instance().response(msg, msg_code, NULL);
    }
    return BCA_NO_ERR;
}

static bca_status_t get_put_duer_brightness(bca_context ctx, bca_msg_t *msg, bca_addr_t *addr)
{
    bca_handler handler = (bca_handler)ctx;
    int msg_code;
    if (handler && msg) {
        if (msg->msg_code == BCA_MSG_REQ_GET) {
            msg_code = BCA_MSG_RSP_CONTENT;
        } else if (msg->msg_code == BCA_MSG_REQ_PUT) {
            msg_code = BCA_MSG_RSP_CHANGED;
        }
        Scheduler::instance().response(msg, msg_code, NULL);
    }
    return BCA_NO_ERR;
}

void device_controller_init(void)
{
    bca_res_t res[] = {
        {BCA_RES_MODE_DYNAMIC, BCA_RES_OP_PUT | BCA_RES_OP_GET, "duer_switch", get_put_duer_switch},
        {BCA_RES_MODE_DYNAMIC, BCA_RES_OP_PUT | BCA_RES_OP_GET, "duer_rgb", get_put_duer_rgb},
        {BCA_RES_MODE_DYNAMIC, BCA_RES_OP_PUT | BCA_RES_OP_GET, "duer_brightness", get_put_duer_brightness},
    };
    Scheduler::instance().add_controll_points(res, sizeof(res) / sizeof(res[0]));
}
