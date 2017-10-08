//
//
//
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fast_bitstring.h"


int test_create() {

        printf("\tTest create...\n");

	{
        fast_bitstring fbs(3);
        assert(fbs.length() == 8 * 3);
	}

	{
        static fast_bitstring::byte bits[] = {0xFF, 0xFF, 0xFF};
        fast_bitstring fbs(bits, sizeof(bits));
        assert(fbs.length() == 8 * sizeof(bits));

		for (int i = 0; i < 8 * sizeof(bits); ++i)
			assert(fbs[i] == 1);
	}

	return 1;
}


int test_bits() {

        printf("\tTest bits...\n");

        fast_bitstring fbs(16);

        assert(fbs[0] == 0);
        fbs[0] = 1;
        assert(fbs[0] == 1);
        fbs[0] = !fbs[0];
        assert(fbs[0] == 0);

        return 1;
}


int test_to_bitstring() {

        printf("\tTest to_bitstring...\n");

        static fast_bitstring::byte bits[3] = {0x55, 0x55, 0x55};
        static fast_bitstring::byte out_bits[3];

        fast_bitstring fbs(bits, sizeof(bits));

#ifdef FBS_DEBUG
        fbs.dump();
#endif
		fbs.to_bitstring(out_bits, 0, 8 * fbs.length());
        assert(strncmp((const char *)bits, (const char *)out_bits, sizeof(bits)) == 0);

        return 1;
}


int unit_test() {
    printf("Running unit tests...\n");
	assert(test_create());
	assert(test_bits());
	assert(test_to_bitstring());
	return 0;
}

