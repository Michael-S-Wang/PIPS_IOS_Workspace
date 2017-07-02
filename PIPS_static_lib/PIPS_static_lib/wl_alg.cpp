#include "stdafx.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "math.h"
#include "wl_config.h"
#include "wl_alg.h"

ue		g_ue;
node	g_nd[WL_MAX_SELECTED_NODES_PER_ROOM];
int		g_node_ind;

double wl_fix_to_double(uint8_t dh, uint8_t dl)
{
	double x;
	
	x = (double) (dh & 0x7F) + (0.00390625 * (dl & 0xFF) );
	if (dh & 0x80) {
		x = -x;
	}
		
	return x;
}


void wl_alg_init(void)
{	
	g_node_ind = 0;
	ResetNodes();
	ResetUE();
}

bool wl_get_4d_position(double *x, double *y, double *z)
{
	bool pass;
	
	if( (g_node_ind >= 4)&&(g_node_ind < WL_MAX_SELECTED_NODES_PER_ROOM) ) {
		pass = wl_pips_4d(g_node_ind, x, y, z);
		return pass;
	}

	return false;
}

bool wl_pips_4d(int num_nodes, double *x, double *y, double *z)
{
	Cal4DRedundentNodePos(num_nodes);
	if(g_ue.numRdntMeasure >= 1) {
		*x = g_ue.x;
		*y = g_ue.y;
		*z = g_ue.z;
	}
	else {
		return false;
	}
		
	return true;
}

int Cal4DRedundentNodePos(int NUM_NODES)
{
	int i, j, k, l, m=0, N=0;
	double mx=0, my=0, mz=0;
	node *ndIn[4];
	ue ue1;
	
	ue1.dim = 4;
	
	// calculate a ue position by all nodes
	for(i=0; i<NUM_NODES; i++) {
		for(j=i+1; j<NUM_NODES; j++) {
			for(k=j+1; k<NUM_NODES; k++) {
				for(l=k+1; l<NUM_NODES; l++) {

					// select 3 nodes
					ndIn[0] = &g_nd[i];
					ndIn[1] = &g_nd[j];
					ndIn[2] = &g_nd[k];
					ndIn[3] = &g_nd[l];

					m++;
				
					/*
					if(cfg.LOG_SINGLE_UE_POS) {
						fprintf(pf1, "\n%2d  nd1=(%5.2f, %5.2f, %5.2f)m, nd2=(%5.2f, %5.2f, %5.2f)m, nd3=(%5.2f, %5.2f, %5.2f)m:\n", m, ndIn[0]->x, ndIn[0]->y, ndIn[0]->z, ndIn[1]->x, ndIn[1]->y, ndIn[1]->z, ndIn[2]->x, ndIn[2]->y, ndIn[2]->z);
						fprintf(pf1, "   ideal: xyz=(%5.2f, %5.2f, %5.2f)m <=> ri=(%5.2f, %5.2f, %5.2f)m <=> rss=(%5.2e, %5.2e, %5.2e)mW <=> rssi=(%5.2f, %5.2f, %5.2f)dBm, RxSNR=(99.9dB, 99.9dB, 99.9dB), Err(dr, eq)=(%3.0f, %5.2f)cm\n", 
								  cfg.x, cfg.y, cfg.z, cfg.r[i], cfg.r[j], cfg.r[k], cfg.rss[i], cfg.rss[j], cfg.rss[k], cfg.rssi[i], cfg.rssi[j], cfg.rssi[k], 0, 0 );
					}
					*/

					// calculate ue position by RSS, in mWatt linear power
					if(!Cal4DSinglePos(ndIn[0], ndIn[1], ndIn[2], ndIn[2], &ue1) ) {
						N++;
						mx += ue1.x;
						my += ue1.y;
						mz += ue1.z;
					
						DBG_FILE(pf_alg, "\n    N = %d, nodes = (#%d, #%d, #%d), (x,y,z) = (%4.2f, %4.2f, %4.2f)", N, ndIn[0]->id, ndIn[1]->id, ndIn[2]->id, ue1.x, ue1.y, ue1.z);

						/*
						if(cfg.LOG_SINGLE_UE_POS) {
							drErr = 100 * sqrt( sq(ue1->x - cfg.x) + sq(ue1->y - cfg.y) + sq(ue1->z - cfg.z) );
							fprintf(pf1, "     rss: xyz=(%5.2f, %5.2f, %5.2f)m <=> ri=(%5.2f, %5.2f, %5.2f)m <=> rss=(%5.2e, %5.2e, %5.2e)mW <=> rssi=(%5.2f, %5.2f, %5.2f)dBm, RxSNR=(%4.1fdB, %4.1fdB, %4.1fdB), Err(dr, eq)=(%3.0f, %5.2f)cm\n", 
								  ue1->x, ue1->y, ue1->z, ndIn[0]->r, ndIn[1]->r, ndIn[2]->r, ndIn[0]->rss, ndIn[1]->rss, ndIn[2]->rss, powTodB(ndIn[0]->rss), powTodB(ndIn[1]->rss), powTodB(ndIn[2]->rss), ndIn[0]->snr, ndIn[1]->snr, ndIn[2]->snr, drErr, ue1->eqErr);
						}
						*/
					}
				}
			}
		}
	}

	if(N) {
		g_ue.x = mx/N;
		g_ue.y = my/N;
		g_ue.z = mz/N;
	}

	g_ue.numRdntMeasure = N;

	return 0;
}


int Cal4DSinglePos(node *nd1, node *nd2, node *nd3, node *nd4, ue *ue1)
{
	int i, j, n, rt, m=0;
	double rssi, rssj;
	node *nd[4], nda, ndb, ndc;

	ue1->dim = 4;

	nd[0] = nd1;
	nd[1] = nd2;
	nd[2] = nd3;
	nd[3] = nd4;

	for (i=0; i<4; i++) {
		for(j=i+1; j<4; j++) {
			rssi = nd[i]->rss;
			rssj = nd[j]->rss;

			if( fabs(rssi - rssj) > 0.01 * rssi ) {
				m++;
				if( m == 1 ) {
					nda.x = (rssi * nd[i]->x - rssj * nd[j]->x)/(rssi - rssj);
					nda.y = (rssi * nd[i]->y - rssj * nd[j]->y)/(rssi - rssj);
					nda.z = nd[i]->z;
					nda.pr = ( sq(nd[i]->x - nd[j]->x) + sq(nd[i]->y - nd[j]->y) + sq(nd[i]->z - nd[j]->z) ) * rssi * rssj/sq(rssi - rssj);
					//fprintf(pf1, "            m=%d, (rssi-rssj) = %06.2e, nda_xyz=(%6.2f, %6.2f, %6.2f), (rssi*rssj)/sq(rssi-rssj)=%6.2e, nda_pr=%6.2f\n", m, (rssi - rssj), nda.x, nda.y, nda.z, rssi * rssj/sq(rssi - rssj), nda.pr);
				}
				else if( m == 2 ) {
					ndb.x = (rssi * nd[i]->x - rssj * nd[j]->x)/(rssi - rssj);
					ndb.y = (rssi * nd[i]->y - rssj * nd[j]->y)/(rssi - rssj);
					ndb.z = nd[i]->z;
					ndb.pr = ( sq(nd[i]->x - nd[j]->x) + sq(nd[i]->y - nd[j]->y) + sq(nd[i]->z - nd[j]->z) ) *rssi * rssj/sq(rssi - rssj);
					//fprintf(pf1, "            m=%d, (rssi-rssj) = %06.2e, ndb_xyz=(%6.2f, %6.2f, %6.2f), (rssi*rssj)/sq(rssi-rssj)=%6.2e, ndb_pr=%6.2f\n", m, (rssi - rssj), ndb.x, ndb.y, ndb.z, rssi * rssj/sq(rssi - rssj), ndb.pr);
				}
 				else if( m == 3 ) {
					ndc.x = (rssi * nd[i]->x - rssj * nd[j]->x)/(rssi - rssj);
					ndc.y = (rssi * nd[i]->y - rssj * nd[j]->y)/(rssi - rssj);
					ndc.z = nd[i]->z;
					ndc.pr = ( sq(nd[i]->x - nd[j]->x) + sq(nd[i]->y - nd[j]->y) + sq(nd[i]->z - nd[j]->z) ) *rssi * rssj/sq(rssi - rssj);
					//fprintf(pf1, "            m=%d, (rssi-rssj) = %06.2e, ndc_xyz=(%6.2f, %6.2f, %6.2f), (rssi*rssj)/sq(rssi-rssj)=%6.2e, ndc_pr=%6.2f\n", m, (rssi - rssj), ndc.x, ndc.y, ndc.z, rssi * rssj/sq(rssi - rssj), ndc.pr);
					if(rt == 0) {
						for(n=0; n<4; n++) {
							nd[n]->r = sqrt( sq(ue1->x - nd[n]->x) + sq(ue1->y - nd[n]->y) + sq(ue1->z - nd[n]->z) );
						}
						break;
					}
					else {
						m = 2;
						//fprintf(pf1, "            Cal3DSinglePos() failed at ue1.pass=%d\n", n);
					}
				}
			}
		}
		if(m == 3)
			break;
	}

	if(m < 3 )
		return 1;

	return 0;
}

bool wl_get_2d_position(double *x, double *y)
{
	if( (g_node_ind >= 2)&&(g_node_ind < WL_MAX_SELECTED_NODES_PER_ROOM) ) {
		//Cal2DRedundentNodePos(g_node_ind);
		Cal2DRedundentNodePos(2);
		if(g_ue.numRdntMeasure >= 1) {
			*x = g_ue.x;
			*y = g_ue.y;
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

int Cal2DRedundentNodePos(int NUM_NODES)
{
	int i, j, m=0, N=0;
	double mx=0, my=0;
	node *ndIn[2];
	ue ue1;
	
	ue1.dim = 3;
	
	// calculate a ue position by all nodes
	for(i=0; i<NUM_NODES; i++) {
		for(j=i+1; j<NUM_NODES; j++) {

			// select 3 nodes
			ndIn[0] = &g_nd[i];
			ndIn[1] = &g_nd[j];

			m++;
			
			/*
			if(cfg.LOG_SINGLE_UE_POS) {
				fprintf(pf1, "\n%2d  nd1=(%5.2f, %5.2f, %5.2f)m, nd2=(%5.2f, %5.2f, %5.2f)m, nd3=(%5.2f, %5.2f, %5.2f)m:\n", m, ndIn[0]->x, ndIn[0]->y, ndIn[0]->z, ndIn[1]->x, ndIn[1]->y, ndIn[1]->z, ndIn[2]->x, ndIn[2]->y, ndIn[2]->z);
				fprintf(pf1, "   ideal: xyz=(%5.2f, %5.2f, %5.2f)m <=> ri=(%5.2f, %5.2f, %5.2f)m <=> rss=(%5.2e, %5.2e, %5.2e)mW <=> rssi=(%5.2f, %5.2f, %5.2f)dBm, RxSNR=(99.9dB, 99.9dB, 99.9dB), Err(dr, eq)=(%3.0f, %5.2f)cm\n", 
						  cfg.x, cfg.y, cfg.z, cfg.r[i], cfg.r[j], cfg.r[k], cfg.rss[i], cfg.rss[j], cfg.rss[k], cfg.rssi[i], cfg.rssi[j], cfg.rssi[k], 0, 0 );
			}
			*/

			// calculate ue position by RSS, in mWatt linear power
			if(!Cal2DSinglePos(ndIn[0], ndIn[1], &ue1) ) {
				N++;
				mx += ue1.x;
				my += ue1.y;
				
				//DBG_FILE(pf_alg, "\n    N = %d, nodes = (#%d, #%d), (x, y) = (%4.1f, %4.1f)", N, ndIn[0]->id, ndIn[1]->id, ue1.x, ue1.y);
				DBG_FILE(pf_alg, "\n    nodes = (#%d, #%d), (x, y) = (%4.1f, %4.1f)", ndIn[0]->id, ndIn[1]->id, ue1.x, ue1.y);

				/*
				if(cfg.LOG_SINGLE_UE_POS) {
					drErr = 100 * sqrt( sq(ue1->x - cfg.x) + sq(ue1->y - cfg.y) + sq(ue1->z - cfg.z) );
					fprintf(pf1, "     rss: xyz=(%5.2f, %5.2f, %5.2f)m <=> ri=(%5.2f, %5.2f, %5.2f)m <=> rss=(%5.2e, %5.2e, %5.2e)mW <=> rssi=(%5.2f, %5.2f, %5.2f)dBm, RxSNR=(%4.1fdB, %4.1fdB, %4.1fdB), Err(dr, eq)=(%3.0f, %5.2f)cm\n", 
						  ue1->x, ue1->y, ue1->z, ndIn[0]->r, ndIn[1]->r, ndIn[2]->r, ndIn[0]->rss, ndIn[1]->rss, ndIn[2]->rss, powTodB(ndIn[0]->rss), powTodB(ndIn[1]->rss), powTodB(ndIn[2]->rss), ndIn[0]->snr, ndIn[1]->snr, ndIn[2]->snr, drErr, ue1->eqErr);
				}
				*/
			}
		}
	}

	if(N) {
		g_ue.x = mx/N;
		g_ue.y = my/N;
	}

	g_ue.numRdntMeasure = N;

	return 0;
}

int Cal2DSinglePos(node *nd1, node *nd2, ue *ue1)
{
	// only for case of node1 xy=(0, 0); node2 xy = (0, d)
	double rss1, rss2, pr1, pr2, u;

	rss1 = nd1->rss;
	rss2 = nd2->rss;

	pr1 = nd1->E/rss1;
	pr2 = nd2->E/rss2;

	nd1->pr = pr1;
	nd2->pr = pr2;

	ue1->y = (pr1 - pr2 + (nd2->y)*(nd2->y))/(2 * nd2->y);
	u = pr1 - (ue1->y)*(ue1->y);
	if(u < 0)
		u = 0;
	ue1->x = sqrt(u);

	return 0;
}


void ResetUE(void)
{
	g_ue.dim = 0;
	g_ue.eqErr = 0;
	g_ue.id = 0;
	g_ue.numRdntMeasure = 0;
	g_ue.pass = 0;
	g_ue.x = 0;
	g_ue.y = 0;
	g_ue.z = 0;
	g_ue.ueMode = UE_SEARCH;
	g_ue.max_rssi_id = 0;
}


void ResetNodes(void)
{
	int i;

	for(i=0; i<WL_MAX_SELECTED_NODES_PER_ROOM; i++) {
		g_nd[i].id = 0;
		g_nd[i].pr = 0;
		g_nd[i].r = 0;		
		g_nd[i].x = 0;
		g_nd[i].y = 0;
		g_nd[i].z = 0;
		g_nd[i].rss = 0;
		g_nd[i].E = 0;
		g_nd[i].max_r = 0;
		g_nd[i].tx_pow = 0;
		g_nd[i].ble_ch = 0;
		g_nd[i].pass = 0;
	}
	
}

