//
// test.cpp
//
// Copyright (C) 2017-2018 Ken Hilton, all rights reserved.
//
// The contents of this source code is protected by trade secret law and may not be viewed,
// studied, compiled or otherwise utilized in any manner with out executing a binding non-
// disclosure agreement including written permission of permissabe from both Ken Hilton and
// Day Goodwin.  Furthermore, this source code contains both intellectual property and trade
// secrets that are the exclusive propery of Ken Hilton and Day Goodwin, respectively.
//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fast_bitstring.h"


static size_t file_size(const char *filename) {
	FILE *f = fopen(filename, "rb");
	fseek(f, 0L, SEEK_END);
	size_t size = ftell(f);
	fclose(f);
	return size;
}


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

	// Bitstring compare
	{
		fast_bitstring::byte bytes[] = {0xFF, 0xFF, 0xFF};
       	        fast_bitstring fbs(bytes, sizeof(bytes));
       	        assert(fbs.compare(fbs) == 0);
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


int test_to_ascii() {

	printf("\tTest to_ascii...\n");

	fast_bitstring::byte bytes[] = {0xFF, 0x55, 0x00, 0x55, 0xFF};
	fast_bitstring fbs(bytes, sizeof(bytes));

	fbs.to_ascii();

	return 1;
}


int test_save() {

	printf("\tTest save...\n");

	fast_bitstring::byte bytes[] = {0xFF, 0x55, 0x00, 0x55, 0xFF};
	fast_bitstring fbs(bytes, sizeof(bytes));

        unlink("./foo.out");
        fbs.save("./foo.out");
        assert(unlink("./foo.out") == 0);

	return 1;
}


int test_to_byte() {

	printf("\tTest to_byte...\n");

	fast_bitstring::byte bytes[] = {0xFF, 0x55, 0x00, 0x55, 0xFF};
	fast_bitstring fbs(bytes, sizeof(bytes));

	fast_bitstring::byte out_bytes[sizeof(bytes)];
	size_t num_bytes = fbs.to_bytes(out_bytes);

	assert(num_bytes == sizeof(bytes));
	assert(strncmp((const char *)bytes, (const char *)out_bytes, sizeof(bytes)) == 0);

	return 1;
}


int test_to_bytes() {

	printf("\tTest to_bytes...\n");

	fast_bitstring::byte bytes[] = {0xFF, 0x55, 0x00, 0x55, 0xFF};
	fast_bitstring fbs(bytes, sizeof(bytes));

	fast_bitstring::byte out_bytes[sizeof(bytes)];
	size_t num_bytes = fbs.to_bytes(out_bytes);

	assert(num_bytes == sizeof(bytes));
	assert(strncmp((const char *)bytes, (const char *)out_bytes, sizeof(bytes)) == 0);

	return 1;
}


int test_rle() {

	printf("\tTest rle...\n");

        if (1) {
	        fast_bitstring::byte bytes[] = {0x00};
	        fast_bitstring fbs(bytes, sizeof(bytes));
                fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                printf("# RLE bytes: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes == 3);
        }

        if (1) {
                // A byte that is not a run of 0's or 1's
	        fast_bitstring::byte bytes[] = {0x75};
	        fast_bitstring fbs(bytes, sizeof(bytes));
                fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                printf("# RLE bytes: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes == 3);
        }

        if (1) {
                // A 9 bit run: should still fit in one RLE byte.
	        fast_bitstring::byte bytes[] = {0xFF, 0xFF};
	        fast_bitstring fbs(bytes, 0, 9);
                fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                printf("# RLE bytes: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes == 1);
        }

        if (1) {
	        fast_bitstring::byte bytes[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	        fast_bitstring fbs(bytes, 0, (sizeof(bytes) * 8) - 1);
                fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                printf("# RLE bytes: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes == 1);
        }

        if (1) {
		// 1, 2+1 + 1 + 2+1
	        fast_bitstring::byte bytes[] = {0xFF, 0xFF, 0xF5, 0x00, 0x00, 0x00, 0x00, 0x55};
	        fast_bitstring fbs(bytes, sizeof(bytes));
                fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                if (FBS_DEBUG || FBS_TRACE) printf("* # RLE bytes: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes == 8);

	        fast_bitstring *rld = fast_bitstring::run_length_decode(rle_bytes, num_bytes);
                assert(fbs.compare(*rld) == 0);
        }

        if (1) {
	        fast_bitstring fbs((char *)"./test.bin");
	        size_t worst_case_num_bytes = fbs.run_length_encode(NULL);
                printf("* Worst case # of RLE bytes needed: %lu\n", worst_case_num_bytes);
		fast_bitstring::byte *rle_bytes = NULL;
	        size_t num_bytes = fbs.run_length_encode(&rle_bytes);
                printf("* # of RLE bytes actually used: %lu\n", num_bytes);
	        assert(rle_bytes != NULL);
                assert(num_bytes <= worst_case_num_bytes);

		size_t fsz = file_size((char *)"./test.bin");
		printf("Compression achieved: %lu/%lu = %0.4f\n", num_bytes, fsz, 100.0f * ((float)num_bytes / (float) fsz));

	        fast_bitstring *rld = fast_bitstring::run_length_decode(rle_bytes, num_bytes);
                assert(fbs.compare(*rld) == 0);

		free(rle_bytes);
        }

	return 1;
}

int test_reverse() {

	printf("\tTest reverse...\n");

	fast_bitstring::byte bytes[] = {0xAA, 0xAA, 0xAA, 0xAA};
	fast_bitstring::byte rbytes[] = {0x55, 0x55, 0x55, 0x55};
	fast_bitstring fbs(bytes, sizeof(bytes));
	fast_bitstring rfbs(rbytes, sizeof(rbytes));

	fbs.reverse();
	assert(fbs.compare(rfbs) == 0);

	return 1;
}


int unit_test() {

	printf("Running unit tests...\n");

	assert(test_create());
	assert(test_bits());
	assert(test_save());
	assert(test_to_ascii());
        // TODO: more comprehensive test_to_byte?
	assert(test_to_byte());
	assert(test_to_bytes());
	assert(test_rle());
	assert(test_reverse());

	return 0;
}

