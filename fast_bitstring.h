/*
 *	fast_bitstring.h
 *
 *	Fast bit string converts a bit string packed into an array of bytes
 *	into an array of bytes, one byte for each bit of the of the source
 *	array of bits.  This representation is designed for two purposes only:
 *	speed and convenience of use, i.e., KISS.
 */

#ifndef _FAST_BITSTRING_H
#define _FAST_BITSTRING_H

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define FBS_DEBUG

/*
 * If we encapsulate bit in a struct then we can overload = to ensure
 * value is 0 or 1.
 */

class fast_bitstring {

protected:
	fast_bitstring() : BITS_PER_BYTE(8) {
                blength = 0;
                barray = NULL;
        }

        typedef fast_bitstring fbs;

public:
	typedef unsigned char byte;

	// Construct bit array of all zero bits.
	fast_bitstring(const size_t length_in_bytes) : BITS_PER_BYTE(8) {
		blength = length_in_bytes * BITS_PER_BYTE;
		barray = (byte *)calloc(blength, 1);
	}

	// Construct bit array from given bit string backed in byte_array.
	fast_bitstring(const byte byte_array[], const size_t length_in_bytes) : BITS_PER_BYTE(8) {
		explode(byte_array, 0, length_in_bytes * BITS_PER_BYTE);
	}

	// Construct bit array from given bit string backed in byte_array.
	fast_bitstring(const byte byte_array[], const size_t offset_in_bits, const size_t length_in_bits) : BITS_PER_BYTE(8) {
		explode(byte_array, offset_in_bits, length_in_bits);
	}

	~fast_bitstring() {
		free(barray);
		barray = NULL;
	}

	// Length of bit string in bits.
	inline size_t length() const { return blength; }

	// Length of bit string in bits.
	inline void clear() { memset((void *)barray, 0, blength); }

	// Array opperator to access bit[i].
	inline byte &operator [](const size_t i) const {
		return barray[i];
	}

        size_t resize(size_t new_size, bool clear) {

                if (new_size == this->blength) {
                        if (clear) memset(this->barray, 0, this->blength);
                        return this->blength;
                }

                this->barray = (byte *) realloc(this->barray, new_size);
                this->blength = new_size;
                if (clear) memset(this->barray, 0, this->blength);

                return this->blength;
        }

        // TODO: Unit test needed.
        int compare(const fast_bitstring &that) const {

                if (this->blength < that.blength)
                        return -1;
                if (this->blength > that.blength)
                        return 1;

                for (size_t i = 0; i < this->blength; ++i) {
                        if (!(this->barray[i] && that.barray[i]))
                                return false;
                }

                return true;
        }

        // TODO: Unit test needed.
        byte to_byte(size_t i) const {

                size_t byte = 0;
                size_t mask = 0x7;
                size_t j = i + 8;

                if (j >= this->blength) j = this->blength;

                for (; i < j; ++i, mask >>= 1) {
                        if (this->barray[i])
                                byte |= mask;
                }

                return byte;
        }

	// Convert internal byte per bit representation back to bits packed into
	// the given byte array.
	size_t to_bytes(byte *bits, size_t offset=0, size_t num_bits=0) const {

		if (num_bits == 0 || num_bits > blength)
			num_bits = blength;

		const size_t end = offset + num_bits;
		register byte b;
		register byte *bap = barray;
		register byte *bp = bits;
                size_t n = 0;

		for (size_t i = offset; i < end;) {
                        b  = 0;
			if (i++ < end) b |= (*bap++ & 1) << 7;
                        if (i++ < end) b |= (*bap++ & 1) << 6;
			if (i++ < end) b |= (*bap++ & 1) << 5;
			if (i++ < end) b |= (*bap++ & 1) << 4;
			if (i++ < end) b |= (*bap++ & 1) << 3;
			if (i++ < end) b |= (*bap++ & 1) << 2;
			if (i++ < end) b |= (*bap++ & 1) << 1;
			if (i++ < end) b |= (*bap++ & 1) << 0;
			*bp++ = b;
                        n++;
		}

		return n;
	}

	size_t to_file(FILE *f = NULL, size_t n = ~0) const {

		if (!f) f = stdout;

		for (size_t i = 0; i < blength && i < n; ++i) {
			fprintf (f, "%u ", (unsigned int)barray[i]);
		}
		fprintf(f, "\n");

		fflush(f);

		return blength;
	}

	size_t save(const char *filename) const {

		size_t len = blength / 8;
		byte *bits = (byte *)malloc(len);
		to_bytes(bits);

		int fd = open(filename, O_CREAT | O_WRONLY);
		size_t n = write(fd, bits, len);
		free(bits);
		return n;
	}

        //
        // Adaptive Run Length Encoding
        //
        // Format:
        //      - Byte-level encoded, starting with first byte (no other header).
        //      - If byte > 128 then byte represents (byte - 128) 1 bits.
        //      - If byte is < 128 then byte represents byte 0 bits.
        //      - If byte == 128 then the *next* byte is the count of the following
        //        verbatim bits packed into bytes, where the count 1...256 is mapped
        //        to 0...255 so we don't loose a bit.
        //      - When calculating a run, it must be 8 bits or longer otherwise it is
        //        deemed unfit to be a run and is appended to the current run of verbatim
        //        bits.
        //
        // Returns:
        //      - The number of bytes the bit string was encoded into.
        //      - If the "encoding" argument is non-null then a pointer to the encoded bits
        //        encoded as a byte array is passed back via this pararmeter. The caller takes
        //        ownership of the allocated memory and is responsible for freeing it.
        //
        size_t run_length_encode(byte **encoding) const {

                if (this->blength == 0) return 0;

                const size_t len = this->blength - 1;   // RLE looks one ahead, so stop before last bit.
                byte *  bits = this->barray;            // Bits being run-length encoded.
                size_t  b = 0;                          // Index of current RLE byte
                size_t  run_len;                        // Length of the current run.
                fbs     verbatim_bits(32);              // Current run of verbatim bits (32 bytes).
                size_t  v = 0;                          // Index into current run of verbatim bits.
                size_t  h;                              // Start of next segment being analyzed.

                // Worst case length is all of the this bitstring's bits packed into
                // bytes + the "128" sentinal bytes for each 256 bit verbatim segment +
                // the count bytes for all 256 bit verbatim segments + 2 for rounding
                // the divisions.
                size_t  worst_case_rle_len = (this->blength / 8)         // bits packed into bytes
                                           + ((this->blength / 256) * 2) // # of sentinal and count byte
                                           + 2;                          // rounding for divisions
                byte *  rle_bytes = (byte *)calloc(1, worst_case_rle_len);

                if (DEBUG) {
                        printf("Worst case REL len: %lu\n", worst_case_rle_len);
                }

#define PROCESS_VERBATIM_BITS                                                           \
                                                                                        \
        rle_bytes[b++] = 128;                                                           \
                                                                                        \
        /* Store bytes at [b + 1] to leave room for the actual byte count */            \
        /* byte to come after the 128 sentinal and before the verbatim bits. */         \
        size_t y = verbatim_bits.to_bytes(&rle_bytes[b + 1], 0, v);                     \
        assert(y == ((v / 8) + ((v % 8) ? 1 : 0)));                                     \
                                                                                        \
        rle_bytes[b++] = (byte)(v - 1); /* map from 1...256 to 0...255 */               \
        b += y;                                                                         \
        assert(b <= worst_case_rle_len);                                                \
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
        static fast_bitstring *run_length_decode(byte *rle_bytes, size_t num_bytes) {

                // Calculate # of bits needed.
                size_t bits_needed = 0, b, nvb, nby;

                for (b = 0; b < num_bytes; ) {
                        if (rle_bytes[b] == 128) {
                                // Count verbatim bits
                                nvb = rle_bytes[b + 1];
                                bits_needed += vb;
                                // Compute stride to next RLE guide byte.
                                nby = (nvb / 8) + ((nvb % 8) ? 1 : 0);
                                b += (nby + 2);
                        } else {
                                vb = rle_bytes[b];
                                if (vb > 128) vb -= 128;
                                bits_needed += vb;
                                b += 1;
                        }
                }
                assert(b == num_bytes);

                fbs *decoded_fbs = new fbs();
                decoded_fbs->resize(bits_needed, true);
                byte value;
                size_t v;

                for (b = 0; b < num_bytes; ) {
                        if (rle_bytes[b] == 128) {
                                // Decode verbatim bits...
                                vb = rle_bytes[b + 1];
                                // TODO do we want to create an append bit/byte method/operator?
                                // Stride to next RLE guide byte.
                                nb = (vb / 8) + ((vb % 8) ? 1 : 0);
                                b += (nb + 2);
                        } else {
                                // Decode 1/0 run...
                                vb = rle_bytes[b];
                                if (vb > 128) {
                                        vb -= 128;
                                        value = 1;
                                } else {
                                        value = 0;
                                }
                                // TODO do we want to create an append bit/byte method/operator?

                                // Stride to next guide byte
                                b += 1;
                        }
                }
                assert(b == num_bytes);

                return decoded_fbs;
        }


protected:

	/*
	 * Given a byte array containing a packed string of bits, explode the bits
	 * into an array of bytes, one bit per byte.  Yes, this is an 8x increase
	 * in memory utlization but that is the classic time/space trade off for you.
	 */
	void explode(const byte byte_array[], const size_t offset_in_bits, const size_t length_in_bits) {

		register byte b, mask, *ba;
		register size_t len = length_in_bits;
		size_t i, o;

		blength = length_in_bits;
		barray = (byte *)calloc(length_in_bits, 1);

		for (i = 0, o = offset_in_bits, ba = barray; len; ) {
			b = byte_array[i++];
			for (mask = 0x80; mask && len; mask >>= 1) {
				// Skip first "offset" bits.
				if (o > 0) { --o; continue; }
				if (b & mask) *ba = 0x1;
				++ba;
				--len;
			}
		}
	}

private:
	const size_t BITS_PER_BYTE;

	size_t blength;	// length of bit array, one byte per bit.
	byte *barray;   // Array of bits, one byte per bit.

};

#endif

