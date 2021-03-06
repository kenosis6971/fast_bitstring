//
// fast_bitstring.cpp
//
// Copyright (C) 2017-2020 Ken Hilton, all rights reserved.
//
// The contents of this source code is protected by trade secret law and may not be viewed,
// studied, compiled or otherwise utilized in any manner with out executing a binding non-
// disclosure agreement including written permission of permissible use from Ken Hilton.
// Furthermore, this source code contains both intellectual property and trade
// secrets that are the exclusive propery of Ken Hilton.
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
//      - The number of bytes the bit string has been encoded into.
//      - If the "encoding" argument is non-null then a pointer to the encoded bits
//	encoded as a byte array is passed back via this pararmeter. The caller takes
//	ownership of the allocated memory and is responsible for freeing it.
//
size_t fast_bitstring::run_length_encode(byte **encoding, size_t n_bits) const {

	if (this->blength == 0) {
		if (encoding) *encoding = NULL;
		printf("Empty FBS\n");
		return 0;
	}

	const size_t len = n_bits ? n_bits - 1 : this->blength - 1;	// RLE looks one ahead, so stop before last bit.
	byte *  bits = this->barray;	    				// Bits being run-length encoded.
	size_t  b = 0;			  				// Index of current RLE byte
	size_t  run_len;						// Length of the current run.
	fbs     verbatim_bits(32);	      				// Current run of verbatim bits (32 bytes).
	size_t  v;			  				// Index into current run of verbatim bits.
	size_t  h;			      				// Start of next segment being analyzed.
	size_t	i;

	// Worst case analysis: We could get an RLE of 9 followed by a verbatim string of
	// 8, followed by a RLE of 9 then another verb str of 8, which leads to the true
	// worst case, I think.  This could be approximated as VS's of 8 bits, which is
	// then 3 bytes where 1 would have sufficed, which is then 3 * the # of 8 bit VS's.
	size_t worst_case_rle_len = 3 * ((this->blength / 8) + 1);

	// If encoding not requested then return # of bytes needed to store encoding.
	if (!encoding) return worst_case_rle_len;

	byte *rle_bytes = (byte *)calloc(1, worst_case_rle_len);

	if (FBS_TRACE) printf("Worst case REL len: %lu\n", worst_case_rle_len);

#define APPEND_VERBATIM_BITS							\
										\
if (FBS_DEBUG) printf("AV: %3lu v's\n", v);					\
assert(v <= 256);								\
										\
/* Append verbatim bits sentinal. */						\
rle_bytes[b++] = 128;							   	\
										\
/* Store bytes at [b + 1] to leave room for the actual byte count */	    	\
/* byte to come after the 128 sentinal and before the verbatim bits. */	 	\
size_t y = verbatim_bits.to_bytes(&rle_bytes[b + 1], 0, v);		     	\
assert(y == ((v / 8) + ((v % 8) ? 1 : 0)));				     	\
										\
/* map bit count of 1..256 to 0..255, insert after sentinal and before bytes */	\
rle_bytes[b++] = (byte)(v - 1);							\
b += y;									 	\
assert(b <= worst_case_rle_len);						\
v = 0;

	for (i = v = 0; i < len; ++i) {

		// Store starting index when checking for a new run, so if the run is too short
		// to be encoded as a run we know from where to start copying verbatim bits.
		h = i;

		// Calculate the current run length: a sequence of contiguous 1's or 0's.
		for (run_len = 1; i < len && bits[i] == bits[i + 1] && run_len < 127; ++i) {
			run_len += 1;
		}

		if (FBS_TRACE) printf("Gathered run of length %lu\n", run_len);

		// If run is sufficently long RLE compress it, if not add it to the verbatim
		// bits so as to ammortize out the memory cost of verbatim bits accounting bytes.
		if (run_len > 8) {

			assert(v <= 256);

			// Store any residual verbatim bits before appending the new run.
			if (v > 0) {
				if (FBS_TRACE) printf("Appending previous verbatim bits: %lu verbatim bits\n", v);
				assert(v <= 256);

				APPEND_VERBATIM_BITS
			}

			// Append run encoded as a single byte: < 128 = run of 0's, > 128 = run of 1's
			if (FBS_DEBUG) printf("AR: %3lu %c's\n", run_len, bits[h] ? '1' : '0');
			if (FBS_TRACE) printf("Appending run of %lu %c's\n", run_len, bits[h] ? '1' : '0');

			assert(run_len < 128);
			rle_bytes[b] = run_len;
			if (bits[h]) rle_bytes[b] += 128;
			b++;
			assert(b <= worst_case_rle_len);

		} else {
			// Append verbatim bits to the verbatim bits fbs.
			assert(run_len > 0);

			if (FBS_TRACE) printf("Accumulating %lu verbatim bits\n", run_len);

			while (run_len-- > 0) {
				assert(v <= 256);
				if (v == 256) {
					// verbatim bits is full so append them to the rle bytes.
					if (FBS_TRACE) printf("VFBS full: appending %lu verbatim bits.\n", v);

					APPEND_VERBATIM_BITS
				}
				verbatim_bits[v++] = bits[h++];
			}
		}
	}
tail:
	while (i < this->blength ) {
		assert(v <= 256);
		if (v == 256) {
			// verbatim bits is full so append them to the rle bytes.
			if (FBS_TRACE) printf("VFBS full: appending %lu verbatim bits.\n", v);

			APPEND_VERBATIM_BITS
		}
		verbatim_bits[v++] = bits[i++];
	}

	if (v > 0) {
		assert(v <= 256);
		// Finally, append any residual verbatim bits, which occurs if the tail
		// of the bit string did not end on a run or 0's or 1's.
		if (FBS_TRACE) printf("Appending %lu residual verbatim bits.\n", v);

		APPEND_VERBATIM_BITS
	}

	if (FBS_DEBUG) printf("EI: %lu\n", i);

	if (encoding)
		*encoding = rle_bytes;
	else
		free(rle_bytes);

	assert(b <= worst_case_rle_len);
	return b;
}

//
// Decode an RLE encoded byte stream into an fast_bitstring.
//
// See comments for run_length_encode (above) for details.
//
fast_bitstring *fast_bitstring::run_length_decode(const byte *rle_bytes, const size_t num_bytes) {

	size_t	bits_needed = 0,
		b,			// tmp variable
		stride,			// number of bytes to skip to the next RLE guide byte
		nb,			// number of bits
		nvb,			// number of virtual bits
		nby;			// number bytes

	// Calculate total # of bits needed for the decoding.
	for (b = 0; b < num_bytes; ) {
		if (rle_bytes[b] == 128) {
			// Count verbatim bits
			nvb = rle_bytes[b + 1] + 1;
			bits_needed += nvb;
			// Compute stride to next RLE guide byte, ie, skip over encoded verbatim bytes.
			stride = (nvb / 8) + (((nvb < 8) || (nvb % 8)) ? 1 : 0);
			// Stride + 2 to account for guide and count bytes.
			b += (stride + 2);
		} else {
			// Count 0|1 run bits.
			nb = rle_bytes[b] & 0x7F;	// Mask off indicator (high) bit; we only care about the count here.
			bits_needed += nb;
			// Move to next guide byte
			b += 1;
		}
	}
	// Ensure all input bytes have been processed.
	assert(b == num_bytes);

	if (FBS_DEBUG) printf("Bits needed: %lu\n", bits_needed);

	fbs *decoded_fbs = new fbs(bits_needed, FROM_BITS);
	byte value;
	size_t v;	// index to next decoded bit

	// Decode RLE bits into an fbs for continued use.
	for (b = v = 0; b < num_bytes; ) {
		if (rle_bytes[b] == 128) {
			// Decode verbatim bits...
			nvb = rle_bytes[b + 1] + 1;	// verbatim count is stored less 1 to fit all 256 possible lengths.
			fbs verbatim_bits(&rle_bytes[b + 2], 0, nvb);
			size_t n_appended = decoded_fbs->append(v, verbatim_bits);
			assert(n_appended <= 256);
			if (FBS_DEBUG) printf("DV: %3lu (%lu)\n", n_appended, verbatim_bits.length());
			assert(n_appended == nvb);
			v += n_appended;
			// Stride to next RLE guide byte.
			stride = (nvb / 8) + (((nvb < 8) || (nvb % 8)) ? 1 : 0);
			b += (stride + 2);
		} else {
			// Decode 1/0 run...
			nvb = rle_bytes[b];
			if (nvb > 128) {
				nvb &= 0x7F;
				value = 1;
			} else {
				value = 0;
			}
			if (FBS_DEBUG) printf("DR: %3lu %d's\n", nvb, value);
			// Could refactor this with an append_n_bits() method or a memset() call.
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

size_t fast_bitstring::bit_count_to_byte_count(size_t n) {
	return (n / 8) + (((n < 8) || (n % 8)) ? 1 : 0);
}

