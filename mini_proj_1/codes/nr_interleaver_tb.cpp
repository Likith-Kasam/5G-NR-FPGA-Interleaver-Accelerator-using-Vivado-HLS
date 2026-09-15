#include "header.h"

int main() {
	int test_cases[14][2] = {
			{40, 2},
			{672, 2},
			{1360, 2},
			{7200, 2},
			{8400, 4},
			{9840, 4},
			{12888, 4},
			{13120, 4},
			{9258, 6},
			{9336, 6},
			{9582, 6},
			{10800, 6},
			{9496, 8},
			{10000, 8}
	};

	const char* base_path =
	        "D:/fpga_class_assgns/mini_proj_1/solution1/csim/build/";

	int total_pass =0;
	int total_fail=0;

	// running each test case
	for ( int tc =1; tc<= 14; tc++) {
		int G = test_cases[tc-1][0];
		int Qm = test_cases[tc-1][1];
		int num_bursts = (G + BUS_WIDTH-1)/ BUS_WIDTH; // ceil representation

		// stream objects
		dataStream inDataFIFO;
		cnStream cnDataFIFO;
		dataStream outDataFIFO;

		// reading input test vector files
		char inFileNameBuf[300];
		sprintf(inFileNameBuf, "test_case_%d_in", tc);

		ifstream input_file(inFileNameBuf);
		if (!input_file.is_open()) {
			cout << "TC" << tc << ":ERROR - cannot open" << inFileNameBuf << "\n";
			total_fail++;
			continue;
		}

		// writing config data to cnData stream
		datau128b cn_word =0;
		cn_word.range(14,0) = G;
		cn_word.range(18,15) = Qm;
		cnDataFIFO.write(cn_word);


		for (int burst=0; burst< num_bursts; burst++) {
			char buffer[24];
			input_file.read(buffer, 24);

			char hi_buf[13] = {0};
			char lo_buf[13] = {0};
			strncpy(hi_buf, buffer, 12);
			strncpy(lo_buf, buffer +12, 12);

			unsigned long long hi_val=0, lo_val=0;
			sscanf(hi_buf, "%llx", &hi_val);
			sscanf(lo_buf, "%llx", &lo_val);

			datau96b word;
			word.range(95,48) = hi_val;
			word.range(47,0) = lo_val;


			data_axi in_pkt;
			in_pkt.data = word;
			in_pkt.last = (burst == num_bursts-1) ? 1:0;
			inDataFIFO.write(in_pkt);

			char newline[1];
			input_file.read(newline, 1);

		}
		input_file.close();

		// calling the interleaver
		NR_Interleaver(inDataFIFO, cnDataFIFO, outDataFIFO);

		// wrting output to file
		char outFileNameBuf[300];
		sprintf(outFileNameBuf, "%soutput_data", base_path);
		ofstream output_file(outFileNameBuf);

		for ( int burst =0; burst < num_bursts;burst++) {

			data_axi out_pkt;
			outDataFIFO >> out_pkt;
			datau96b word = out_pkt.data;

			string s="";
			for (int j=2; j>=0; j--) {
				datau32b chunk = word.range(32*j+31, 32*j);
				unsigned int u = chunk.to_uint();
				char hex_buf[9];
				sprintf(hex_buf, "%08X", u);
				s+= string(hex_buf);
			}
			output_file << s;
			if (burst != num_bursts-1) output_file << endl;
		}
		output_file.close();

		// comparing output with expected

		char expected_file[300];
		char cmd[600];
		sprintf(expected_file, "%stest_case_%d_out", base_path, tc);
		sprintf(cmd, "diff -w %s %s", outFileNameBuf, expected_file);

		if (system(cmd)) {
			cout << "TC" << tc << "(G=" << G << ", Qm=" << Qm << "): Fail \n";
			total_fail++;
		} else {
			cout << "TC" << tc << "(G=" << G << ", Qm=" << Qm << "): Pass \n";
			total_pass++;
		}
	}
	cout << "\n===============================\n";
	cout << "Results:" << total_pass << "/14 PASSED\n";
	if (total_fail ==0)
		cout << "ALL TEST CASES PASSED!\n";
	else
		cout << " " << total_fail << " test case(s) FAILED\n";
	cout << "===============================\n";

	return (total_fail ==0) ? 0:1 ;
}

