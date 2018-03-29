#include "rte_common.h"
#include "rte_mbuf.h"
#include "rte_meter.h"
#include "rte_red.h"

#include "qos.h"

#include <stdio.h>
#include <assert.h>

/**
 * for srTCM
 */
struct rte_meter_srtcm m[APP_FLOWS_MAX];
struct rte_meter_srtcm_params params[APP_FLOWS_MAX];

/**
 * for WRED
 */
struct rte_red_params red_params[APP_FLOWS_MAX][COLORNUM];
struct rte_red_config red_cfg[APP_FLOWS_MAX][COLORNUM];
struct rte_red red[APP_FLOWS_MAX];
uint32_t queuesize[APP_FLOWS_MAX];


/**
 * srTCM
 */
int
qos_meter_init(void)
{
    FILE *f = fopen("meter.txt", "r");
    assert(f);

    int i;
    for (i = 0; i < APP_FLOWS_MAX; i++) {
        assert(fscanf(f, "%lld %lld %lld \n\n", (long long int*)&params[i].cir, 
        (long long int*)&params[i].cbs, (long long int*)&params[i].ebs));
        
        printf("display: %lld %lld %lld\n", (long long int)params[i].cir, 
        (long long int)params[i].cbs,(long long int) params[i].ebs);
        rte_meter_srtcm_config(&m[i], &params[i]);
    }

    fclose(f);
    printf("fuck\n");
    return 0;
}

enum qos_color
qos_meter_run(uint32_t flow_id, uint32_t pkt_len, uint64_t time)
{
    return rte_meter_srtcm_color_blind_check(&m[flow_id], time, pkt_len);
}


/**
 * WRED
 */

int
qos_dropper_init(void)
{
    FILE *f = fopen("drop.txt", "r");
    printf("\nfuckyou\n");
    assert(f);
    int i, j;
    for (i = 0; i < APP_FLOWS_MAX; i++) {
        for (j = 0; j < COLORNUM; j++) {
            assert(fscanf(f, "%d %d %d %d\n",
            (int *)&red_params[i][j].min_th, (int *)&red_params[i][j].max_th,  
            (int *)&red_params[i][j].maxp_inv, (int *)&red_params[i][j].wq_log2));

            printf("%d %d %d %d\n",
            (int)red_params[i][j].min_th, (int)red_params[i][j].max_th,  
            (int)red_params[i][j].maxp_inv, (int)red_params[i][j].wq_log2);

            assert(rte_red_config_init(&red_cfg[i][j], 
            red_params[i][j].wq_log2, red_params[i][j].min_th, 
            red_params[i][j].max_th, red_params[i][j].maxp_inv) == 0);
        }
        rte_red_rt_data_init(&red[i]);
        queuesize[i] = 0;
        printf("\n");
        //fseek(f, 1, SEEK_CUR);
    }

    memset(queuesize, 0, sizeof(uint32_t) * APP_FLOWS_MAX);
    fclose(f);
    return 0;
}

int
qos_dropper_run(uint32_t flow_id, enum qos_color color, uint64_t time)
{  
    return rte_red_enqueue(&red_cfg[flow_id][color], &red[flow_id], 
        (const unsigned)queuesize[flow_id], time);
}