#include "header.h"

void sub_block_interleaver(dataStream &inData, cnStream &cnData, dataStream &outData)
{
#pragma HLS INTERFACE axis port=inData
#pragma HLS INTERFACE axis port=outData
#pragma HLS INTERFACE axis port=cnData
#pragma HLS INTERFACE ap_ctrl_none port=return

    // ================= CONFIG =================
    datau64b config = cnData.read();
    datau10b N = config.range(17, 8);

    const datau32b P[32] = {
        0,1,2,4,3,5,6,7,
        8,16,9,17,10,18,11,19,
        12,20,13,21,14,22,15,23,
        24,25,26,28,27,29,30,31
    };

    // ================= MEMORY =================
    datau512b input_seq  = 0;
    datau512b output_seq = 0;

    datau3b burst = (N + 127) >> 7;   // divide by 128

    // ================= INPUT READ =================
    READ_INPUT:
    for (datau3b i = 0; i < burst; i++)
    {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=4

        data_axi temp = inData.read();

        if(i == 0) input_seq.range(127,0)   = temp.data;
        else if(i == 1) input_seq.range(255,128) = temp.data;
        else if(i == 2) input_seq.range(383,256) = temp.data;
        else if(i == 3) input_seq.range(511,384) = temp.data;
    }

    // ================= INTERLEAVING =================
    datau10b loop_limit = N >> 2;

    datau5b shift;
    datau10b mask;

    if (N == 128) {
        shift = 2; mask = 3;
    } else if (N == 256) {
        shift = 3; mask = 7;
    } else {
        shift = 4; mask = 15;
    }

    INTERLEAVE:
    for (datau10b k = 0; k < loop_limit; k++)
    {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=32 max=128

        UNROLL_4:
        for (datau3b m = 0; m < 4; m++)
        {
#pragma HLS UNROLL

            datau10b n = (k << 2) | m;

            datau10b i_idx = n >> shift;
            datau10b j = (P[i_idx] << shift) | (n & mask);

            datau10b in_index  = ((j >> 7) << 7) | (127 - (j & 127));
            datau10b out_index = ((n >> 7) << 7) | (127 - (n & 127));

            output_seq[out_index] = input_seq[in_index];
        }
    }

    // ================= OUTPUT WRITE =================
    WRITE_OUTPUT:
    for (datau3b i = 0; i < burst; i++)
    {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=4

        data_axi temp;

        if(i == 0) temp.data = output_seq.range(127,0);
        else if(i == 1) temp.data = output_seq.range(255,128);
        else if(i == 2) temp.data = output_seq.range(383,256);
        else if(i == 3) temp.data = output_seq.range(511,384);
        else temp.data = 0;

        temp.last = (i == burst - 1);
        outData.write(temp);
    }
}
