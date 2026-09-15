#include "header.h"

/*
 * channel_interleaver_tb.cpp
 * -----------------------------------------------------------------------
 * Testbench for the 5G-NR Channel Interleaver (Mini Project 2)
 *
 * Test vector format
 * ------------------
 *  Each file (e.g. "E_32_in", "E_32_out") contains 20 test instances.
 *  Every instance occupies ceil(E/128) lines; each line is a 128-bit
 *  value printed as 32 uppercase hex characters, MSB first.
 *  Unused trailing bits in the last word are zero-padded.
 *
 * Test cases
 * ----------
 *  16 files (one per E value): E in {32,48,64,80,96,112,128,144,
 *                                    176,208,288,320,352,384,480,512}
 *  Each file contains 20 instances -> 320 total comparisons.
 * -----------------------------------------------------------------------
 */

int main()
{
    // All 16 valid E values (E = 16*k, k=1..32, test vectors available)
    int E_vals[16] = {
        32, 48, 64, 80, 96, 112, 128,
        144, 176, 208,
        288, 320, 352, 384,
        480, 512
    };

    int total_pass = 0;
    int total_fail = 0;

    // ----------------------------------------------------------------
    // Iterate over each E value (one test-vector file pair per value)
    // ----------------------------------------------------------------
    for (int tc = 0; tc < 16; tc++) {

        int E          = E_vals[tc];
        int num_bursts = (E + BUS_WIDTH - 1) / BUS_WIDTH;   // ceil(E/128)
        int instances  = 20;                                 // per file

        // Open input and expected-output files
        char in_filename[64], out_filename[64];
        sprintf(in_filename,  "E_%d_in",  E);
        sprintf(out_filename, "E_%d_out", E);

        ifstream in_file(in_filename);
        ifstream ref_file(out_filename);

        if (!in_file.is_open()) {
            cout << "E=" << E << ": ERROR - cannot open " << in_filename << "\n";
            total_fail++;
            continue;
        }
        if (!ref_file.is_open()) {
            cout << "E=" << E << ": ERROR - cannot open " << out_filename << "\n";
            total_fail++;
            continue;
        }

        int inst_pass = 0;
        int inst_fail = 0;

        // ------------------------------------------------------------
        // Run all 20 instances for this E value
        // ------------------------------------------------------------
        for (int inst = 0; inst < instances; inst++) {

            // --- Build AXI streams for this instance ---
            dataStream inDataFIFO;
            cnStream   cnDataFIFO;
            dataStream outDataFIFO;

            // Write configuration word: E in bits[11:0]
            datau64b cn_word = 0;
            cn_word.range(11, 0) = E;
            cnDataFIFO.write(cn_word);

            // Read ceil(E/128) 128-bit words from the input file
            for (int burst = 0; burst < num_bursts; burst++) {
                char line[33] = {0};          // 32 hex chars + null terminator
                in_file.read(line, 32);
                in_file.ignore(1);            // consume the newline

                // Parse 32 hex chars as a 128-bit big-endian value
                // Split into two 64-bit halves for portability
                char hi_buf[17] = {0}, lo_buf[17] = {0};
                strncpy(hi_buf, line,      16);
                strncpy(lo_buf, line + 16, 16);

                unsigned long long hi_val = 0, lo_val = 0;
                sscanf(hi_buf, "%llx", &hi_val);
                sscanf(lo_buf, "%llx", &lo_val);

                datau128b word;
                word.range(127, 64) = hi_val;
                word.range(63,  0)  = lo_val;

                data_axi in_pkt;
                in_pkt.data = word;
                in_pkt.last = (burst == num_bursts - 1) ? 1 : 0;
                inDataFIFO.write(in_pkt);
            }

            // --- Call the DUT ---
            channel_interleaver(inDataFIFO, cnDataFIFO, outDataFIFO);

            // --- Compare output with reference ---
            bool instance_ok = true;

            for (int burst = 0; burst < num_bursts; burst++) {
                // Read DUT output word
                data_axi out_pkt;
                outDataFIFO >> out_pkt;
                datau128b got = out_pkt.data;

                // Read reference word from file
                char ref_line[33] = {0};
                ref_file.read(ref_line, 32);
                ref_file.ignore(1);   // consume newline

                char hi_buf[17] = {0}, lo_buf[17] = {0};
                strncpy(hi_buf, ref_line,      16);
                strncpy(lo_buf, ref_line + 16, 16);

                unsigned long long hi_val = 0, lo_val = 0;
                sscanf(hi_buf, "%llx", &hi_val);
                sscanf(lo_buf, "%llx", &lo_val);

                datau128b expected;
                expected.range(127, 64) = hi_val;
                expected.range(63,  0)  = lo_val;

                if (got != expected) {
                    instance_ok = false;
                    // Detailed mismatch output (first burst only to keep logs clean)
                    if (burst == 0) {
                        cout << "  E=" << E
                             << " inst=" << inst
                             << " burst=" << burst
                             << " MISMATCH\n";
                        // Print got vs expected as hex
                        cout << "    got:      ";
                        for (int k = 3; k >= 0; k--) {
                            datau32b chunk = got.range(32*k+31, 32*k);
                            cout << hex << uppercase << setw(8) << setfill('0')
                                 << (unsigned int)chunk;
                        }
                        cout << "\n    expected: ";
                        for (int k = 3; k >= 0; k--) {
                            datau32b chunk = expected.range(32*k+31, 32*k);
                            cout << hex << uppercase << setw(8) << setfill('0')
                                 << (unsigned int)chunk;
                        }
                        cout << dec << "\n";
                    }
                }
            }

            if (instance_ok) inst_pass++; else inst_fail++;
        }

        // Summary for this E value
        if (inst_fail == 0) {
            cout << "E=" << setw(4) << E << ": " << inst_pass << "/20 PASS\n";
            total_pass++;
        } else {
            cout << "E=" << setw(4) << E << ": "
                 << inst_pass << " pass, " << inst_fail << " FAIL\n";
            total_fail++;
        }

        in_file.close();
        ref_file.close();
    }

    // ----------------------------------------------------------------
    // Final summary
    // ----------------------------------------------------------------
    cout << "\n===============================\n";
    cout << "Results: " << total_pass << "/16 E-values PASSED\n";
    if (total_fail == 0)
        cout << "ALL TEST CASES PASSED!\n";
    else
        cout << total_fail << " E-value(s) FAILED\n";
    cout << "===============================\n";

    return (total_fail == 0) ? 0 : 1;
}
