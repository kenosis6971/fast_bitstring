/*
 *	test.cpp
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fast_bitstring.h"


int test_create() {

	printf("\tTest create...\n");

	// Bitstring of all zeros.
	{
                fast_bitstring fbs(3);
                assert(fbs.length() == 8 * 3);
	}

	// Bit string initialized with user data, no offset.
	{
		fast_bitstring::byte bytes[] = {0xFF, 0xFF, 0xFF};
       	        fast_bitstring fbs(bytes, sizeof(bytes));
       	        assert(fbs.length() == 8 * sizeof(bytes));

		for (int i = 0; i < 8 * sizeof(bytes); ++i)
			assert(fbs[i] == 1);
	}

	// Bitsting initialized with offset into user data.
	{
		fast_bitstring::byte bytes[] = {0x7F, 0xFF, 0xFE};
       	        fast_bitstring fbs(bytes, 1, sizeof(bytes) * 8 - 1);
       	        assert(fbs.length() == 8 * sizeof(bytes) - 1);

		for (int i = 0; i < 8 * sizeof(bytes) - 2; ++i)
			assert(fbs[i] == 1);

		assert(fbs[fbs.length() - 1] == 0);
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

	fast_bitstring::byte bytes[] = {0xFF, 0x55, 0x00, 0x55, 0xFF};
	fast_bitstring fbs(bytes, sizeof(bytes));

	fbs.write();

	fast_bitstring::byte out_bytes[sizeof(bytes)];
	size_t num_bits = fbs.to_bitstring(out_bytes);

	assert(num_bits == fbs.length());
	assert(strncmp((const char *)bytes, (const char *)out_bytes, sizeof(bytes)) == 0);

	return 1;
}


int unit_test() {

	printf("Running unit tests...\n");
	assert(test_create());
	assert(test_bits());
	assert(test_to_bitstring());

	return 0;
}

