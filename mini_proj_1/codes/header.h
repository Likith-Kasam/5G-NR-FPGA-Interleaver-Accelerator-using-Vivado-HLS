#ifndef HEADER_H
#define HEADER_H
#include <hls_stream.h>
#include <ap_int.h>
#include <stdio.h>
#include <hls_math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>


using namespace std;

// bit-width typedefs
typedef ap_uint<4> datau4b;
typedef ap_uint<8> datau8b;
typedef ap_uint<15> datau15b;
typedef ap_uint<32> datau32b;
typedef ap_uint<64> datau64b;
typedef ap_uint<128> datau128b;
typedef ap_uint<96> datau96b;

#define MAX_G 16384 //maximum supported value for G as per excel i.e., 2^14
#define BUS_WIDTH 96 // axi width in bits

// AXI-stream for indata and outdata
struct data_axi{
	ap_uint<96> data;
	ap_uint<1> last;
};

typedef hls::stream<data_axi> dataStream;
typedef hls::stream<datau128b> cnStream;

// Top Level Function
void NR_Interleaver(dataStream &inData, cnStream &cnData, dataStream &outData);

#endif
