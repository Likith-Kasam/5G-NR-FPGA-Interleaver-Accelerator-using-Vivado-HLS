#include "header.h"

void mini3_inData(dataStream &inData)
{
#pragma HLS INTERFACE axis port=inData
#pragma HLS INTERFACE ap_ctrl_none port=return

    // Each 128-bit value split into high 64 bits and low 64 bits
    datau128b temp_data[4];
    temp_data[0] = (datau128b(0x2AE3D27A1FD95558ULL) << 64) | datau128b(0xECC424623D7CB83DULL);
    temp_data[1] = (datau128b(0x29434B4094F144EAULL) << 64) | datau128b(0x45CE17F21CFE0325ULL);
    temp_data[2] = (datau128b(0x7181EF7E78875460ULL) << 64) | datau128b(0xD1C07F003C44DF63ULL);
    temp_data[3] = (datau128b(0x8DDE89BB0C50BA2DULL) << 64) | datau128b(0x8735B36FE2399B84ULL);

    datau1b temp_last[4] = {0, 0, 0, 1};

    data_axi output;
    LOOP_I: for (int i = 0; i < 4; i++) {
#pragma HLS PIPELINE
        output.data = temp_data[i];
        output.last = temp_last[i];
        inData.write(output);
    }
}
