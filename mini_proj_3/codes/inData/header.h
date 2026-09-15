#ifndef HEADER_H
#define HEADER_H
#include <hls_stream.h>
#include <ap_int.h>
#include <stdio.h>
#include <hls_math.h>
#include <fstream>
#include <iostream>

using namespace std;
using namespace hls;

typedef ap_uint<1> datau1b;
typedef ap_uint<32> datau32b;
typedef ap_uint<128> datau128b;


//Defining data type for output stream interface
struct data_axi
{
	ap_uint<128> data;
	ap_uint<1> last;
};

//defining the stream interface for input and output ports

typedef stream<data_axi> dataStream;

//Defining the function
void mini3_inData(dataStream &inData);

#endif
