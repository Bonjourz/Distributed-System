#include "rte_common.h"
#include "rte_mbuf.h"
#include "rte_meter.h"
#include "rte_red.h"
#include <rte_cycles.h>

#include "qos.h"

#include <stdio.h>
#include <assert.h>

/**
 * for srTCM
 */
struct rte_meter_srtcm m[APP_FLOWS_MAX];
struct rte_meter_srtcm_params params[APP_FLOWS_MAX] = {
{.cir = 160000000, .cbs = 160000, .ebs = 480000},
{.cir = 80000000, .cbs = 80000, .ebs = 240000},
{.cir = 40000000, .cbs = 40000, .ebs = 80000},
{.cir = 20000000, .cbs = 20000, .ebs = 60000}
};

/**
 * for WRED
 */
struct rte_red_params red_params[APP_FLOWS_MAX][3];
struct rte_red_config red_cfg[APP_FLOWS_MAX][4];
struct rte_red red[APP_FLOWS_MAX];
uint32_t queuesize[APP_FLOWS_MAX];


/**
 * srTCM
 */
int
qos_meter_init(void)
{   
    int i;
    for (i = 0; i < APP_FLOWS_MAX; i++)
        rte_meter_srtcm_config(&m[i], &params[i]);
        
    return 0;
}

enum qos_color
qos_meter_run(uint32_t flow_id, uint32_t pkt_len, uint64_t time)
{
    return rte_meter_srtcm_color_blind_check(&m[flow_id], 
    (double)time * (double)rte_get_tsc_hz() / (double) 1000000000, pkt_len);
}


/**
 * WRED
 */

int
qos_dropper_init(void)
{
    int i, j;
    struct rte_red_params ele0[3] = {
        {.min_th = 1022, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9},
        {.min_th = 1022, .max_th = 1023, .maxp_inv = 8, .wq_log2 = 9},
        {.min_th = 32, .max_th = 64, .maxp_inv = 7, .wq_log2 = 9}
    };
    memcpy(red_params, ele0, 3 * sizeof(struct rte_red_params));

    struct rte_red_params ele1[3] = {
        {.min_th = 45, .max_th = 85, .maxp_inv = 10, .wq_log2 = 9},
        {.min_th = 40, .max_th = 64, .maxp_inv = 9, .wq_log2 = 9},
        {.min_th = 31, .max_th = 60, .maxp_inv = 8, .wq_log2 = 9}
    };

    memcpy(red_params + 1, ele1, 3 * sizeof(struct rte_red_params));

    struct rte_red_params ele2[3] = {
        {.min_th = 27, .max_th = 57, .maxp_inv = 10, .wq_log2 = 9},
        {.min_th = 13, .max_th = 27, .maxp_inv = 9, .wq_log2 = 9},
        {.min_th = 10, .max_th = 21, .maxp_inv = 8, .wq_log2 = 9}
    };

    memcpy(red_params + 2, ele2, 3 * sizeof(struct rte_red_params));

    struct rte_red_params ele3[3] = {
        {.min_th = 13, .max_th = 27, .maxp_inv = 8, .wq_log2 = 9},
        {.min_th = 7, .max_th = 15, .maxp_inv = 6, .wq_log2 = 9},
        {.min_th = 3, .max_th = 9, .maxp_inv = 4, .wq_log2 = 9},
    };

    memcpy(red_params + 3, ele3, 3 * sizeof(struct rte_red_params));

    for (i = 0; i < APP_FLOWS_MAX; i++) {
        for (j = 0; j < 3; j++) {
            assert(rte_red_config_init(&red_cfg[i][j], 
            red_params[i][j].wq_log2, red_params[i][j].min_th, 
            red_params[i][j].max_th, red_params[i][j].maxp_inv) == 0);
        }
        rte_red_rt_data_init(&red[i]);
    }

    memset(queuesize, 0, sizeof(uint32_t) * APP_FLOWS_MAX);
    return 0;
}

int
qos_dropper_run(uint32_t flow_id, enum qos_color color, uint64_t time)
{  
    static uint64_t last_time = -1;
    if (time != last_time) {
        last_time = time;
        memset(queuesize, 0, sizeof(uint32_t) * APP_FLOWS_MAX);
    }

    int drop = rte_red_enqueue(&red_cfg[flow_id][color], &red[flow_id], 
                    (const unsigned)queuesize[flow_id], time);
    queuesize[flow_id] += drop ? 0 : 1;
    return drop;
}
