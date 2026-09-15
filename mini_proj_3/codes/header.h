#ifndef HEADER_H
#define HEADER_H

#include <ap_int.h>
#include <hls_stream.h>

// ================= TYPE DEFINITIONS =================
typedef ap_uint<512> datau512b;
typedef ap_uint<128> datau128b;
typedef ap_uint<64>  datau64b;
typedef ap_uint<32>  datau32b;
typedef ap_uint<10>  datau10b;
typedef ap_uint<7>   datau7b;
typedef ap_uint<5>   datau5b;
typedef ap_uint<6>   datau6b;
typedef ap_uint<4>   datau4b;
typedef ap_uint<3>   datau3b;
typedef ap_uint<1>   datau1b;

// ================= AXI STRUCT =================
struct data_axi {
    datau128b data;
    datau1b last;
};

// ================= STREAM TYPES =================
typedef hls::stream<data_axi> dataStream;
typedef hls::stream<datau64b> cnStream;

// ================= TOP FUNCTION =================
void sub_block_interleaver(
    dataStream &inData,
    cnStream &cnData,
    dataStream &outData
);

#endif
