#ifndef CHANNEL_INTERLEAVER_H
#define CHANNEL_INTERLEAVER_H

#include <hls_stream.h>
#include <ap_int.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

using namespace std;

// Bit-width typedefs
typedef ap_uint<10>  datau10b;   // enough to index up to 512 bits (E_max)
typedef ap_uint<12>  datau12b;   // E field in cnData: bits[11:0]
typedef ap_uint<32>  datau32b;
typedef ap_uint<64>  datau64b;
typedef ap_uint<128> datau128b;

// AXI bus width in bits (128-bit for MP2)
#define BUS_WIDTH    128

// E = 16*k, k=1..32  =>  E_max = 512
// Maximum T: T*(T+1)/2 >= 512  =>  T=32  (32*33/2 = 528)
#define E_MAX        512
#define T_MAX        32
#define CELLS_MAX    528  // T_MAX*(T_MAX+1)/2  -- flat loop bound for LOOP_INTERLEAVE
#define MAX_BURSTS   4    // ceil(512/128) = 4 AXI words max

// AXI-Stream packet for inData / outData (128-bit data + last)
struct data_axi {
    ap_uint<128> data;
    ap_uint<1>   last;
};

// Stream type aliases  (same pattern as MP1)
typedef hls::stream<data_axi>  dataStream;  // inData / outData
typedef hls::stream<datau64b>  cnStream;    // cnData  (E in bits[11:0])

// Top-level function declaration
void channel_interleaver(dataStream &inData, cnStream &cnData, dataStream &outData);

#endif // CHANNEL_INTERLEAVER_H
