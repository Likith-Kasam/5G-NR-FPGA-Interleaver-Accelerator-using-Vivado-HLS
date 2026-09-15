#include "header.h"

void mini3_inConfig(cnStream &outData){
#pragma HLS INTERFACE axis port = outData
#pragma HLS interface ap_ctrl_none port=return

	datau64b configTemp;
	configTemp.range(7,0) = 140;
	configTemp.range(17,8) = 512;
	configTemp.range(28,18) = 1728;
	configTemp.range(44,29) = 65535;
	configTemp.range(60,45) = 65519;
	outData.write(configTemp);
}
