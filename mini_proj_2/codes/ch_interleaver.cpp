#include "header.h"

/*
 * channel_interleaver.cpp
 * -----------------------------------------------------------------------
 * 5G-NR Channel Interleaver for PUCCH  [3GPP TS 38.212, clause 5.4.1.3]
 *
 * Algorithm summary
 * -----------------
 *  Given E input bits (e[0..E-1]):
 *   1. Find smallest T s.t. T*(T+1)/2 >= E.
 *   2. Write e[] into an isosceles-triangular array row-wise
 *      (outer loop i=0..T-1, inner j=0..T-1-i  =>  v[j][i] = e[k++]).
 *   3. Read the array column-wise
 *      (outer loop j=0..T-1, inner i=0..T-1-j  =>  f[k++] = v[j][i]).
 *
 * Key insight (no 2-D array needed)
 * ----------------------------------
 *  Write index for cell (j, i) in read-order:
 *      k_src(j,i) = i*T - i*(i-1)/2 + j
 *  This lets us directly copy e[k_src] to f[k_dst] with no matrix storage.
 *
 * Latency and resource design notes
 * ----------------------------------
 *  LATENCY FIX: The original nested loop (LOOP_COL outer, LOOP_ROW inner)
 *  caused HLS to compute: 32 outer * 37 worst-inner = 1184 cycles (12.7 us).
 *  HLS multiplies outer_trips * worst_inner_latency because the outer loop
 *  is NOT pipelined and it ignores that inner trips DECREASE with j.
 *  Fix: one flat LOOP_INTERLEAVE with T*(T+1)/2 iterations, pipelined II=1.
 *  -> HLS now sees ~531 cycles max (6.2 us at 100 MHz).
 *
 *  RESOURCE FIX: Using plain 'int' for flat_i, flat_j, k_dst, step caused
 *  HLS to generate 32x32-bit multipliers (2 DSP48E, many LUTs) for the
 *  k_src formula flat_i*T - flat_i*(flat_i-1)/2 + flat_j.
 *  Fix: use ap_uint<6> for flat_i/flat_j/T (range 0..32) and ap_uint<10>
 *  for step/k_dst/total_cells (range 0..528). This makes all multiplies
 *  6x6-bit -> pure LUT, and narrows all adders from 39 LUTs to ~15 LUTs.
 *
 * Latency estimate (worst case E=512, T=32, @ 100 MHz)
 * -----------------------------------------------------
 *  LOOP_T          :  ~32 cycles
 *  LOOP_INIT       :    4 cycles
 *  LOOP_READ       :    4 cycles
 *  LOOP_INTERLEAVE : ~531 cycles  (528 + pipeline depth, II=1)
 *  LOOP_WRITE      :    4 cycles
 *  Total           : ~575 cycles  =>  ~5.75 us  (< 11 us target)
 * -----------------------------------------------------------------------
 */

void channel_interleaver(dataStream &inData, cnStream &cnData, dataStream &outData)
{
    // ---------- HLS interface directives ----------
    #pragma HLS INTERFACE axis port=inData
    #pragma HLS INTERFACE axis port=outData
    #pragma HLS INTERFACE axis port=cnData
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // ---------- 1. Read configuration ----------
    datau64b config = cnData.read();
    ap_uint<10> E = (ap_uint<10>)config.range(11, 0);   // bits[11:0] = E (max 512 = 10 bits)

    // ---------- 2. Derive T (smallest T s.t. T*(T+1)/2 >= E) ----------
    // ap_uint<6>: range 1..32 fits in 6 bits.
    // Keeping T narrow ensures the k_src multiplications (T * flat_i) stay
    // as 6x6-bit LUT multiplies instead of 32x32-bit DSP48E multiplies.
    ap_uint<6> T = 1;
    LOOP_T: for (ap_uint<6> t = 1; t <= (ap_uint<6>)T_MAX; t++) {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=32 avg=20
        ap_uint<11> tri = (ap_uint<11>)t * ((ap_uint<11>)t + 1) / 2;
        if (tri >= E) {
            T = t;
            break;
        }
    }

    ap_uint<3> num_bursts = (ap_uint<3>)((E + BUS_WIDTH - 1) / BUS_WIDTH);  // ceil(E/128), max 4

    // ---------- 3. Input and output bit buffers ----------
    // MAX_BURSTS=4: only 4 x 128-bit = 512-bit storage needed -> fits in registers (no BRAM)
    datau128b e_buf[MAX_BURSTS];
    datau128b f_buf[MAX_BURSTS];
    #pragma HLS ARRAY_PARTITION variable=e_buf complete
    #pragma HLS ARRAY_PARTITION variable=f_buf complete

    // Initialise f_buf to zero (zero-pads unused trailing bits in last word)
    LOOP_INIT: for (ap_uint<3> b = 0; b < MAX_BURSTS; b++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=4 max=4 avg=4
        f_buf[b] = 0;
    }

    // ---------- 4. Receive all input bursts ----------
    LOOP_READ: for (ap_uint<3> burst = 0; burst < num_bursts; burst++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4 avg=2

        data_axi pkt = inData.read();
        e_buf[burst] = pkt.data;
    }

    // ---------- 5. Channel interleaving (single flat loop, II=1) ----------
    //
    // We advance (flat_j, flat_i) through the triangle in READ order:
    //   outer j=0..T-1, inner i=0..T-1-j  (column-wise scan)
    // using a register-based odometer instead of a nested loop, so HLS
    // sees exactly one pipelined loop of T*(T+1)/2 iterations.
    //
    // Source index formula (write order -> read order mapping):
    //   k_src = flat_i * T - flat_i*(flat_i-1)/2 + flat_j
    //
    // All loop variables are narrow ap_uint types to keep arithmetic small:
    //   flat_i, flat_j, T  : ap_uint<6>  (0..32)
    //   step, k_dst        : ap_uint<10> (0..528)
    //   k_src              : ap_uint<10> (0..511)

    ap_uint<6>  flat_j = 0;   // column in read-order scan (0..T-1)
    ap_uint<6>  flat_i = 0;   // row    in read-order scan (0..T-1-j)
    ap_uint<10> k_dst  = 0;   // output bit index (0..E-1)
    ap_uint<10> total_cells = (ap_uint<10>)T * ((ap_uint<10>)T + 1) / 2;  // T*(T+1)/2

    LOOP_INTERLEAVE: for (ap_uint<10> step = 0; step < (ap_uint<10>)CELLS_MAX; step++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=36 max=528 avg=282

        if (step >= total_cells) break;

        // k_src = flat_i*T - flat_i*(flat_i-1)/2 + flat_j
        // All operands are ap_uint<6>, so multiplies are 6x6-bit -> LUT only
        ap_uint<6>  fi       = flat_i;
        ap_uint<6>  fj       = flat_j;
        ap_uint<11> fi_T     = (ap_uint<11>)fi * (ap_uint<11>)T;        // 6x6 -> 11-bit
        ap_uint<11> fi_fi1   = (ap_uint<11>)fi * (ap_uint<11>)(fi - 1); // 6x6 -> 11-bit
        ap_uint<10> k_src    = (ap_uint<10>)(fi_T - fi_fi1 / 2 + fj);

        if (k_src < E) {
            // Read bit from e_buf, MSB-first packing: bit 0 is at position 127
            ap_uint<2>  sw  = k_src >> 7;           // word index  (k_src / 128), max 3
            ap_uint<7>  sb  = 127 - k_src.range(6, 0); // bit position within word
            ap_uint<1>  bit = e_buf[sw][sb];

            // Write bit to f_buf, same MSB-first packing
            ap_uint<2>  dw  = k_dst >> 7;
            ap_uint<7>  db  = 127 - k_dst.range(6, 0);
            f_buf[dw][db]   = bit;

            k_dst++;
        }

        // Advance triangular odometer
        if (flat_i < (ap_uint<6>)(T - 1 - flat_j)) {
            flat_i++;
        } else {
            flat_i = 0;
            flat_j++;
        }
    }

    // ---------- 6. Write output bursts ----------
    LOOP_WRITE: for (ap_uint<3> burst = 0; burst < num_bursts; burst++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4 avg=2

        data_axi out_pkt;
        out_pkt.data = f_buf[burst];
        out_pkt.last = (burst == (ap_uint<3>)(num_bursts - 1)) ? (ap_uint<1>)1 : (ap_uint<1>)0;
        outData.write(out_pkt);
    }
}
