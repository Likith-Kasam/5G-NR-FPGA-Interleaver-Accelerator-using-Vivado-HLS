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

typedef ap_uint<64> datau64b;

typedef stream<datau64b> cnStream; //control/Config interface

//Defining the function
void mini3_inConfig(cnStream &outData);

#endif
