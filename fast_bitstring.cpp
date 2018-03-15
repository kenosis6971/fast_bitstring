//
// Copyright (C) 2017-2018 Ken Hilton, all rights reserved.
//
//	fast_bitstring.cpp
//

#include "fast_bitstring.h"


//
// Adaptive Run Length Encoding
//
// Format:
//      - Byte-level encoded, starting with first byte (no other header).
//      - If byte > 128 then byte represents (byte - 128) 1 bits.
//      - If byte is < 128 then byte represents byte 0 bits.
//      - If byte == 128 then the *next* byte is the count of the following
//	verbatim bits packed into bytes, where the count 1...256 is mapped
//	to 0...255 so we don't loose a bit.
//      - When calculating a run, it must be 8 bits or longer otherwise it is
//	deemed unfit to be a run and is appended to the current run of verbatim
//	bits.
//
// Returns:
//      - The number of bytes the bit string was encoded into.
//      - If the "encoding" argument is non-null then a pointer to the encoded bits
//	encoded as a byte array is passed back via this pararmeter. The caller takes
//	ownership of the allocated memory and is responsible for freeing it.
//
size_t fast_bitstring::run_length_encode(byte **encoding) const {

	if (this->blength == 0) return 0;

	const size_t len = this->blength - 1;   // RLE looks one ahead, so stop before last bit.
	byte *  bits = this->barray;	    // Bits being run-length encoded.
	size_t  b = 0;			  // Index of current RLE byte
	size_t  run_len;			// Length of the current run.
	fbs     verbatim_bits(32);	      // Current run of verbatim bits (32 bytes).
	size_t  v = 0;			  // Index into current run of verbatim bits.
	size_t  h;			      // Start of next segment being analyzed.

	// Worst case length is all of the this bitstring's bits packed into
	// bytes + the "128" sentinal bytes for each 256 bit verbatim segment +
	// the count bytes for all 256 bit verbatim segments + 2 for rounding
	// the divisions.
	size_t  worst_case_rle_len = (this->blength / 8)	 // bits packed into bytes
				   + ((this->blength / 256) * 2) // # of sentinal and count byte
				   + 2;			  // rounding for divisions
	byte *  rle_bytes = (byte *)calloc(1, worst_case_rle_len);

	if (DEBUG) {
		printf("Worst case REL len: %lu\n", worst_case_rle_len);
	}

#define PROCESS_VERBATIM_BITS							   \
										\
rle_bytes[b++] = 128;							   \
										\
/* Store bytes at [b + 1] to leave room for the actual byte count */	    \
/* byte to come after the 128 sentinal and before the verbatim bits. */	 \
size_t y = verbatim_bits.to_bytes(&rle_bytes[b + 1], 0, v);		     \
assert(y == ((v / 8) + ((v % 8) ? 1 : 0)));				     \
										\
rle_bytes[b++] = (byte)(v - 1); /* map from 1...256 to 0...255 */	       \
b += y;									 \
assert(b <= worst_case_rle_len);						\
v = 0;

	// NOTE: the index 'i' is incremented within the body of the loop below, in
	// addition to in this for loop clause.
	for (size_t i = 0; i < len; ++i) {

		// Store starting index when checking for a new run, so if the run is too short
		// to be encoded as a run we know from where to start copying verbatim bits.
		h = i;

		// Calculate the current run length: a sequence of contiguous 1's or 0's.
		for (run_len = 1; bits[i + 1] == bits[i] && run_len < 127 && i < len; ++i) {
			run_len += 1;
		}

		// If run is sufficently long RLE compress it.
		if (run_len >= 8) {

			// Store any residual verbatim bits before appending the new run.
			if (v > 0) {
				if (DEBUG) printf("New run: appending %lu verbatim bits\n", v);
				assert(v <= 256);

				PROCESS_VERBATIM_BITS
			}

			// Append run encoded as a single byte: < 128 = run of 0's, > 128 = run of 1's
			if (DEBUG) printf("Appending run of %lu %c's\n", run_len, bits[h] ? '1' : '0');

			assert(run_len < 128);
			rle_bytes[b] = run_len;
			if (bits[h]) rle_bytes[b] += 128;
			b++;
			assert(b <= worst_case_rle_len);

		} else {
			// Append verbatim bits to the verbatim bits fbs.
			assert(run_len > 0);
			if (DEBUG) printf("Accumulating %lu verbatim bits\n", run_len);

			while (run_len-- > 0) {
				if (v == 256) {
					// verbatim bits is full so append them to the rle bytes.
					if (DEBUG) printf("VFBS full: appending 128 verbatim bits.\n");

					PROCESS_VERBATIM_BITS
				}
				verbatim_bits[v++] = bits[h++];
			}
		}
	}
tail:
	if (v > 0) {
		// Finally, append any residual verbatim bits, which occurs if the tail
		// of the bit string did not end on a run.
		if (DEBUG) printf("Appending %lu residual verbatim bits.\n", v);

		PROCESS_VERBATIM_BITS
	}

	if (encoding)
		*encoding = rle_bytes;
	else
		free(rle_bytes);

	assert(b <= worst_case_rle_len);
	return b;
}

// See comments above for run_length_encode (above) for details.
fast_bitstring *fast_bitstring::run_length_decode(const byte *rle_bytes, const size_t num_bytes) {

	// Calculate # of bits needed.
	size_t bits_needed = 0, b, nvb, nby;

	for (b = 0; b < num_bytes; ) {
		if (rle_bytes[b] == 128) {
			// Count verbatim bits
			nvb = rle_bytes[b + 1];
			bits_needed += nvb;
			// Compute stride to next RLE guide byte.
			nby = (nvb / 8) + (((nvb > 8) && (nvb % 8)) ? 1 : 0);
			// Stride + 2 to account for guard and count bytes.
			b += (nby + 2);
		} else {
			// Count 0|1 run bits.
			nvb = rle_bytes[b] & 0x7F;      // mask off high bit; we only care about the count.
			bits_needed += nvb;
			b += 1;
		}
	}
	// Ensure all input bytes have been processed.
	assert(b == num_bytes);

	fbs *decoded_fbs = new fbs(bits_needed, FROM_BITS);
	byte value;
	size_t v;

	// Decode RLE bits into an fbs for easy continued use.
	for (b = v = 0; b < num_bytes; ) {
		if (rle_bytes[b] == 128) {
			// Decode verbatim bits...
			nvb = rle_bytes[b + 1];
			fbs verbatim_bits(&rle_bytes[b + 2], nvb);
			size_t n_appended = decoded_fbs->append(v, verbatim_bits);
			assert(n_appended == nvb);
			v += n_appended;
			// Stride to next RLE guide byte.
			nby = (nvb / 8) + (((nvb > 8) && (nvb % 8)) ? 1 : 0);
			b += (nby + 2);
		} else {
			// Decode 1/0 run...
			nvb = rle_bytes[b];
			if (nvb > 128) {
				nvb -= 128;
				value = 1;
			} else {
				value = 0;
			}
			// Could optimize with an append_n_bits() method
			for (size_t i = 0; i < nvb; ++i) {
				(*decoded_fbs)[v++] = value;
			}
			// Stride to next guide byte
			b += 1;
		}
	}
	assert(b == num_bytes);

	return decoded_fbs;
}

