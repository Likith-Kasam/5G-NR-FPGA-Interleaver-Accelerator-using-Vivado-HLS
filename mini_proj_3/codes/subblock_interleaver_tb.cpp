#include "header.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std;

int main()
{
    dataStream inDataFIFO;
    cnStream cnDataFIFO;
    dataStream outDataFIFO;

    datau64b cndata;
    data_axi indata, outdata;

    int parameters[10][5] =
    {
        {23,128,108,17,120},
        {70,256,216,0,0},
        {12,512,432,10698,62289},
        {24,128,108,14343,14343},
        {71,256,216,23456,62351},
        {128,256,216,43748,55858},
        {120,512,864,1007,0},
        {115,512,1728,2500,4111},
        {140,512,1728,65535,65519},
        {135,512,864,25258,34576}
    };

    for(int test = 1; test <= 10; test++)
    {
        // ================= CONFIG =================
        datau10b N = parameters[test-1][1];
        cndata.range(17, 8) = N;
        cnDataFIFO.write(cndata);

        int burst = N / 128;

        // ================= INPUT =================
        string inFileName = "test_case_" + to_string(test) + "_in_sbi";
        ifstream input_file(inFileName);

        for (int i = 0; i < burst; i++)
        {
            char buffer[32];
            input_file.read(buffer, 32);

            string s = "0x";
            for (int j = 0; j < 32; j++)
                s += buffer[j];

            datau128b dataTemp;
            stringstream ss(s);
            ss >> hex >> dataTemp;

            indata.data = dataTemp;
            indata.last = (i == burst - 1);

            inDataFIFO.write(indata);

            char dummy;
            input_file.read(&dummy, 1);
        }
        input_file.close();

        // ================= CALL =================
        sub_block_interleaver(inDataFIFO, cnDataFIFO, outDataFIFO);

        // ================= OUTPUT =================
        ofstream output_file("output_data");

        for(int i = 0; i < burst; i++)
        {
            outdata = outDataFIFO.read();
            datau128b outTemp = outdata.data;

            string s = "";
            for(int j = 3; j >= 0; j--)
            {
                datau32b temp = outTemp.range(32*j+31, 32*j);
                unsigned int val = (unsigned int)temp;

                stringstream ss;
                ss << setw(8) << setfill('0') << uppercase << hex << val;
                s += ss.str();
            }

            output_file << s << endl;
        }
        output_file.close();

        // ================= VERIFY =================
        string cmd = "diff -w output_data test_case_" + to_string(test) + "_out_sbi";

        if (system(cmd.c_str()))
        {
            cout << "Test " << test << " FAILED\n";
            return 1;
        }
        else
        {
            cout << "Test " << test << " PASSED\n";
        }
    }

    return 0;
}
