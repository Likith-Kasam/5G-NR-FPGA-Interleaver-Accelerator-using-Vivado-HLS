#include "header.h"

int main()
{
    cnStream cnDataFIFO;

    // Call DUT
    mini3_inConfig(cnDataFIFO);

    // Read back
    datau64b config = cnDataFIFO.read();

    // Extract each field
    unsigned int field0 = config.range( 7,  0).to_uint(); // 140
    unsigned int N      = config.range(17,  8).to_uint(); // 512
    unsigned int field2 = config.range(28, 18).to_uint(); // 1728
    unsigned int field3 = config.range(44, 29).to_uint(); // 65535
    unsigned int field4 = config.range(60, 45).to_uint(); // 65519

    printf("field0 (exp 140)   = %u\n", field0);
    printf("N      (exp 512)   = %u\n", N);
    printf("field2 (exp 1728)  = %u\n", field2);
    printf("field3 (exp 65535) = %u\n", field3);
    printf("field4 (exp 65519) = %u\n", field4);

    // Verify
    if (field0 != 140 || N != 512 || field2 != 1728 ||
        field3 != 65535 || field4 != 65519)
    {
        printf("FAIL: mismatch!\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
