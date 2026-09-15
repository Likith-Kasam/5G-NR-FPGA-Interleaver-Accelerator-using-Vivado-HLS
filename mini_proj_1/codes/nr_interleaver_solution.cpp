#include "header.h"

void NR_Interleaver(dataStream &inData, cnStream &cnData, dataStream &outData) {
	// hls interface pragmas
	#pragma HLS INTERFACE axis port=inData
	#pragma HLS INTERFACE axis port=outData
	#pragma HLS INTERFACE axis port=cnData
	#pragma HLS INTERFACE ap_ctrl_none port=return

	// 1. Read Configuration
	datau128b config = cnData.read();

	int G   = (int)(datau15b)config.range(14, 0);  // bits[14:0]  = G
	int Qm  = (int)(datau4b)config.range(18, 15);  // bits[18:15] = Qm

	int columns   = G / Qm;
	int num_bursts = (G + BUS_WIDTH - 1) / BUS_WIDTH; // ceil

	// 2. Internal Bit Buffers (no complete partition — stored in BRAM)
	datau96b e_buf[171];
	datau96b f_buf[171];

	// Initialize f_buf
	LOOP_INIT: for (int i = 0; i < 171; i++) {
		#pragma HLS PIPELINE II=1
		#pragma HLS LOOP_TRIPCOUNT min=171 max=171 avg=171
		f_buf[i] = 0;
	}

	// 3. Receive all input bursts into e_buf
	LOOP_READ: for (int burst = 0; burst < num_bursts; burst++) {
		#pragma HLS PIPELINE II=1
		#pragma HLS LOOP_TRIPCOUNT min=1 max=171 avg=86

		data_axi pkt = inData.read();
		e_buf[burst] = pkt.data;
	}

	// 4. Interleaver Algorithm
	// LOOP_BITS is pipelined (II=1) instead of unrolled
	// This avoids complete array partition and the massive FF/LUT explosion
	LOOP_OUT_BURST: for (int ob = 0; ob < num_bursts; ob++) {
		#pragma HLS LOOP_TRIPCOUNT min=1 max=171 avg=86

		datau96b out_word = 0;

		LOOP_BITS: for (int b = 0; b < BUS_WIDTH; b++) {
			#pragma HLS PIPELINE II=1

			int dst = ob * BUS_WIDTH + b;
			if (dst < G) {
				int i = dst % Qm;
				int j = dst / Qm;
				int src = i * columns + j;

				int src_word    = src / BUS_WIDTH;
				int src_bit_pos = BUS_WIDTH - 1 - (src % BUS_WIDTH);
				ap_uint<1> bit  = e_buf[src_word][src_bit_pos];

				int out_bit_pos = BUS_WIDTH - 1 - b;
				out_word[out_bit_pos] = bit;
			}
		}
		f_buf[ob] = out_word;
	}

	// 5. Write f_buf to output AXI stream
	LOOP_WRITE: for (int burst = 0; burst < num_bursts; burst++) {
		#pragma HLS PIPELINE II=1
		#pragma HLS LOOP_TRIPCOUNT min=1 max=171 avg=86

		data_axi out_pkt;
		out_pkt.data = f_buf[burst];
		out_pkt.last = (burst == num_bursts - 1) ? 1 : 0;
		outData.write(out_pkt);
	}
}
