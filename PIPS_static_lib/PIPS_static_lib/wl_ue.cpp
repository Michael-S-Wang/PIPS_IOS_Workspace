#include "stdafx.h"
#include "math.h"
#include "string.h"
#include "wl_config.h"
#include "wl_alg.h"
#include "wl_ue.h"

/*
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "math.h"
#include "wl_config.h"

extern void wl_get_node(wl_ue_scan_st *adv_node);
extern bool wl_get_position(double *x, double *y, double *z);
extern void wl_alg_init(void);
extern char* wl_m2f(double x);
*/

wl_ue_scan_st	g_ue_scan_active[WL_MAX_SCAN_NODES];
wl_ue_scan_st	*g_ue_scan;
wl_ue_scan_st	*g_room[WL_MAX_SCAN_NODES+1];
wl_elapse_time	g_timer;
wl_user_info	g_user;

void wl_timer(double elapse_ms)
{
	int m;

	if(g_timer.current_ms == 0) {
		g_timer.current_ms = 1;
	}
	else {
		g_timer.current_ms += elapse_ms;
	}

	g_timer.dt_ms = elapse_ms;

	if(g_timer.timer_due == 1) {
		g_timer.timer_due = 0;
	}

	if(g_timer.start_ms == 0) {
		g_timer.start_ms = g_timer.current_ms;
		g_timer.timer_due = 0;
	}
	else if( (g_timer.current_ms - g_timer.start_ms) >= WL_MEASURE_WIN_MS ) {
		g_timer.timer_due = 1;
		g_timer.start_ms = g_timer.current_ms;
	}
	else {
		//g_timer.timer_due = 0;
	}

	g_timer.hr = (int) (g_timer.current_ms/3600000);
	m = (int) (g_timer.current_ms/60000);
	g_timer.min = m%60;
	m = (int) (g_timer.current_ms/1000);
	g_timer.sec = m%60;
	g_timer.ms = (long int)g_timer.current_ms%1000;

	sprintf_s(g_timer.str, "%3dh %02dm %02ds %03dms", g_timer.hr, g_timer.min, g_timer.sec, g_timer.ms);

}


void wl_ue_init(void) 
{
	uint16_t i, j;
	
	g_ue_scan = NULL;
	for(i=0; i<WL_MAX_SCAN_NODES; i++) {
		wl_init_scan_data(&g_ue_scan_active[i]);
		g_ue_scan_active[i].rpt_rssi_cbuf = 0;
		g_ue_scan_active[i].wpt_rssi_cbuf = 0;
		g_ue_scan_active[i].deep_rssi_cbuf = 0;
		for(j=0; j < WL_RSSI_CBUF_SIZE; j++) {
		}
	}
	wl_room_init();
	wl_alg_init();

	g_user.x = 0;
	g_user.y = 0;
	g_user.z = 0;
	g_user.new_pos_ready = 0;
	g_user.new_msg_ready = 0;
	g_user.ueMode = UE_SEARCH;
	g_user.max_rssi_id = 0;

	g_timer.current_ms = 0;
	g_timer.dt_ms = 0;
	g_timer.hr = 0;
	g_timer.min = 0;
	g_timer.sec = 0;
	g_timer.ms = 0;
	g_timer.start_ms = 0;
	g_timer.timer_due = 0;

}

void wl_ue_for_next_pos(void)
{
	int i;

	g_ue_scan = NULL;
	for(i=0; i<WL_MAX_SCAN_NODES; i++) {
		g_ue_scan_active[i].acc_cnt = 0;
		g_ue_scan_active[i].current_cnt = 0;
		g_ue_scan_active[i].frssi = 0;
	}	
	wl_room_init();
	wl_alg_init();
}


void wl_init_scan_data(wl_ue_scan_st *wl_ue_scan_data)
{
	wl_ue_scan_data->active = false;
	wl_ue_scan_data->acc_cnt = 0;
	wl_ue_scan_data->current_cnt = 0;
	
	/* for debug only */
	uint16_t i;
	
	for(i=0; i<WL_X_BYTES; i++) {
		wl_ue_scan_data->crn_id[i] = 0;
	}
	//for(i=0; i<WL_SCAN_RSSI_RANG; i++) {
	//	wl_ue_scan_data->rssi[i] = 0;
	//}
	for(i=0; i<WL_SCAN_RSSI_RANG; i++) {
		wl_ue_scan_data->x[i] = 0;
		wl_ue_scan_data->y[i] = 0;
		wl_ue_scan_data->z[i] = 0;
	}
	wl_ue_scan_data->tx_pow = 0;
	wl_ue_scan_data->ch = 0;
	wl_ue_scan_data->frssi = 0;
	wl_ue_scan_data->iir_frssi = 0;
	wl_ue_scan_data->frssi_sd = 0;
	wl_ue_scan_data->max_rssi_id = 0;
	wl_ue_scan_data->max_r = 0;
}


void wl_room_init(void)
{
	uint8_t i;
	
	for(i=0; i<WL_MAX_SCAN_NODES; i++) {
		g_room[i] = NULL;
	}
}

void wl_new_scanned_data(uint8_t rssi, uint8_t *adv_data, int len) 
{
	uint16_t i, j, flag;
	
	g_ue_scan = NULL;
	for(i=0; i<WL_MAX_SCAN_NODES; i++) {
		if(g_ue_scan_active[i].active == true) {
			flag = 0;
			for(j=0; j<WL_CRN_BYTES; j++) {
				if(adv_data[WL_CRN_INDEX + j] != g_ue_scan_active[i].crn_id[j]) {
					flag = 1;
				}
			}
			if(flag == 0) {
				g_ue_scan = &g_ue_scan_active[i];
				break;
			}
		}
	}
	
	if(g_ue_scan == NULL) {
		for(i=0; i<WL_MAX_SCAN_NODES; i++) {
			if(g_ue_scan_active[i].active == false) {
				g_ue_scan = &g_ue_scan_active[i];
				g_ue_scan->active = true;
						
				DBG_FILE(pf_rssi, "\n%s: new Node #%d, (x, y, z)=(%3.1f, %3.1f, 53.1f), BLE_CH=%d", g_timer.str, adv_data[WL_NODE_INDEX],
									wl_fix_to_double(adv_data[WL_X_INDEX], adv_data[WL_X_INDEX+1]), 
									wl_fix_to_double(adv_data[WL_Y_INDEX], adv_data[WL_Y_INDEX+1]), 
									wl_fix_to_double(adv_data[WL_Z_INDEX], adv_data[WL_Z_INDEX+1]),  adv_data[WL_CH_INDEX]);				
				break;
			}
		}
	}
	
}

void wl_proc_rooms(void)
{
	uint16_t i;
	uint16_t node_ind, valid_node_num=0;
	static int measure=0;

	if(g_timer.timer_due == 0) {
		return;
	}
	g_user.new_pos_ready = 1;


	measure++;
	DBG_FILE(pf_log, "\n\n------------ measurement #%d at %s --------------\n", measure, g_timer.str);
	DBG_FILE(pf_alg, "\n\n------------ measurement #%d at %s --------------\n", measure, g_timer.str);
	DBG_FILE(pf_rssi, "\n\n------------ measurement #%d at %s --------------\n", measure, g_timer.str);
	DBG_FILE(pf_api_out, "\n\n------------ measurement #%d at %s --------------", measure, g_timer.str);
	
	 
	//for(i=1; i<2; i++) {
	for(i=0; i<WL_MAX_SCAN_NODES; i++) {
		if(g_ue_scan_active[i].active == true) {
			if(wl_rssi_filter(&g_ue_scan_active[i])) {
				if(g_ue_scan_active[i].frssi >= rssi_lin[WL_SCAN_RSSI_RANG-1]) {
					node_ind = g_ue_scan_active[i].crn_id[6];				
					if( (node_ind >= 1) && ( node_ind <= WL_MAX_SELECTED_NODES_PER_ROOM) ) {
						g_room[node_ind] = &g_ue_scan_active[i];
						valid_node_num++;
					}
					else {
						node_ind = 0;
						g_room[node_ind] = &g_ue_scan_active[i];
					}		
				}
			}
		}
	}
	
	#if (WL_PIPS == PIPS_2D)
		if(valid_node_num >= 2) {
			cal_pos_2d();
		}
	#endif
	
	#if (WL_PIPS == PIPS_3D)
		if(valid_node_num >= 3) {
			cal_pos_3d();
		}
	#endif

	#if (WL_PIPS == PIPS_4D)
		if(valid_node_num >= 4) {
			cal_pos_4d();
		}
	#endif

}

void cal_pos_2d(void)
{
	uint16_t i;
	double x, y;
	
	for(i=1; i<WL_MAX_SCAN_NODES-1; i += 2) {
		if(g_room[i] != NULL) {
			DBG_FILE(pf_alg, "\nnode #%d, frssi=%3.1f; ", g_room[i]->crn_id[6], 10*log10(g_room[i]->iir_frssi));
			wl_get_node(g_room[i]);
		}

		if(g_room[i+1] != NULL) {
			DBG_FILE(pf_alg, "\nnode #%d, frssi=%3.1f; ", g_room[i+1]->crn_id[6], 10*log10(g_room[i+1]->iir_frssi));
			wl_get_node(g_room[i+1]);
		}

	}
		
	if(wl_get_2d_position(&x, &y)) {
		//g_user.new_pos_ready = 1;
		if( (g_user.x == 0) && (g_user.y == 0) ) {
			g_user.x = x;
			g_user.y = y;
			g_user.z = 0;
		}
		else {
			g_user.x = (1 - WL_POS_ALPHA) * g_user.x + WL_POS_ALPHA * x;
			g_user.y = (1 - WL_POS_ALPHA) * g_user.y + WL_POS_ALPHA * y;
			g_user.z = 0;
		}
		DBG_FILE(pf_alg, "\nMeasured UE position: (x, y) = (%3.1f, %3.1f), filtered pos (x, y) = (%3.1f, %3.1f)\n", x, y, g_user.x, g_user.y);
	}
	
}


void cal_pos_4d(void)
{
	uint16_t i;
	double x, y, z;
	
	for(i=1; i<WL_MAX_SCAN_NODES-1; i += 2) {
		if(g_room[i] != NULL) {
			DBG_FILE(pf_alg, "\nnode #%d, frssi=%3.1f; ", g_room[i]->crn_id[6], 10*log10(g_room[i]->iir_frssi));
			wl_get_node(g_room[i]);
		}

		if(g_room[i+1] != NULL) {
			DBG_FILE(pf_alg, "\nnode #%d, frssi=%3.1f; ", g_room[i+1]->crn_id[6], 10*log10(g_room[i+1]->iir_frssi));
			wl_get_node(g_room[i+1]);
		}

	}
		
	if(wl_get_4d_position(&x, &y, &z)) {
		//g_user.new_pos_ready = 1;
		if( (g_user.x == 0) && (g_user.y == 0) ) {
			g_user.x = x;
			g_user.y = y;
			g_user.z = z;
		}
		else {
			g_user.x = (1 - WL_POS_ALPHA) * g_user.x + WL_POS_ALPHA * x;
			g_user.y = (1 - WL_POS_ALPHA) * g_user.y + WL_POS_ALPHA * y;
			g_user.z = (1 - WL_POS_ALPHA) * g_user.z + WL_POS_ALPHA * z;
		}
		DBG_FILE(pf_alg, "\nMeasured UE position: (x, y, z) = (%3.1f, %3.1f, %3.1f), filtered pos (x, y, z) = (%3.1f, %3.1f, %3.1f)\n", x, y, z, g_user.x, g_user.y, g_user.z);
	}
	
	
}


