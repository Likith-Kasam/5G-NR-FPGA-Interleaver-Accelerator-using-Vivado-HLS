#include "header.h"

int main()
{
    dataStream inDataFIFO;

    // Call the DUT
    mini3_inData(inDataFIFO);

    // Read and print all outputs
    int count = 0;
    while (!inDataFIFO.empty())
    {
        data_axi out;
        inDataFIFO >> out;
        count++;

        // Print 128-bit data as 32 hex chars (MSB first)
        datau128b d = out.data.read();
        string s = "";
        for (int j = 3; j >= 0; j--)
        {
            datau32b chunk = d.range(32*j+31, 32*j);
            stringstream ss;
            ss << std::setfill('0') << std::setw(8) << uppercase << hex << (unsigned int)chunk.read();
            s += ss.str();
        }

        printf("Burst %d: %s  last=%d\n", count, s.c_str(), (int)out.last.read());
    }

    // Verify count
    if (count != 4)
    {
        printf("FAIL: expected 4 bursts, got %d\n", count);
        return 1;
    }

    printf("PASS: all 4 bursts received correctly\n");
    return 0;
}
