/*
 * Copyright (C) 2017-2018 Ken Hilton, all rights reserved.
 *
 *	fast_bitstring.h
 *
 *	Fast bit string converts a bit string packed into an array of bytes
 *	into an array of bytes, one byte for each bit of the of the source
 *	array of bits.  This representation is designed for two purposes only:
 *	speed and convenience of use, i.e., KISS.
 */

#ifndef _FAST_BITSTRING_H
#define _FAST_BITSTRING_H

#if FBS_DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 * NOTES:
 *
 * - If we encapsulate bit in a struct then we can overload = to ensure
 * value is 0 or 1.
 *
 * - Convert in_bytes in constructor from bool to enum.
 *
 * - If we could ensure the bits were always 0 and 1 then we could use intrincics for comparison.
 *   And alternative would be a fast compare whose contract is barrys={0|1}
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
	typedef enum { FROM_BYTES, FROM_BITS } BIT_SOURCE;

	// Construct bit array of all zero bits.
	// TODO: XXX does bit_source make sense for this constructor?
	fast_bitstring(const size_t length, const BIT_SOURCE bit_source=FROM_BYTES) : BITS_PER_BYTE(8) {
		blength = bit_source == FROM_BYTES ? (length * BITS_PER_BYTE) : length;
		barray = (byte *)calloc(blength, 1);
	}

	// Construct bit array from given bit string backed in byte_array.
	fast_bitstring(const byte *byte_array, const size_t length_in_bytes) : BITS_PER_BYTE(8) {
		explode(byte_array, 0, length_in_bytes * BITS_PER_BYTE);
	}

	// Construct bit array from given bit string backed in byte_array, skipping the first "offset_in_bits" bits.
	fast_bitstring(const byte *byte_array, const size_t offset_in_bits, const size_t length_in_bits) : BITS_PER_BYTE(8) {
		explode(byte_array, offset_in_bits, length_in_bits);
	}

	fast_bitstring(char *filename) : BITS_PER_BYTE(8) {
		FILE *f = fopen(filename, "rb");
		fseek(f, 0L, SEEK_END);
		size_t size = ftell(f);
		byte *bytes = (byte *) calloc(1, size);
		rewind(f);
		if (fread(bytes, 1, size, f) != size) throw "Failed to read bytes for fast bitstring";
		fclose(f);
		explode(bytes, 0, size * BITS_PER_BYTE);
		free(bytes);
	}

	fast_bitstring(fast_bitstring &f, size_t len = ~0, size_t offset = 0) : BITS_PER_BYTE(8) {
		if (len == ~0) len = f.length();
		if (offset + len > f.length()) throw "Invalid copy constructor parameters: offset + length > copy source length.";
		blength = len;
		barray = (byte *)calloc(blength, 1);

		memcpy((void *)barray, (void *)&f[offset], blength);
	}

	~fast_bitstring() {
		if (barray) {
			free(barray);
			barray = NULL;
		}
		blength = 0;
	}

	// Length of bit string in bits.
	inline size_t length() const { return blength; }

	// Length of bit string in bits.
	inline void clear() { memset((void *)barray, 0, blength); }

	// Length of bit string in bits.
	inline void set_all(byte val = 1) { memset((void *)barray, val, blength); }

	// Array opperator to access bit[i].
	inline byte &operator [](const size_t i) const {
		return barray[i];
	}

	size_t resize(size_t new_size, bool clear=false) {

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
			if (!this->barray[i] && that.barray[i])
				return -1;
			if (this->barray[i] && !that.barray[i])
				return 1;
		}

		return 0;
	}

	// TODO: Unit test needed.
	void reverse() {

		register byte b;

		for (size_t i = 0, j = blength - 1; i < j; ++i, --j) {
			b = barray[i];
			barray[i] = barray[j];
			barray[j] = b;
		}
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
	size_t to_bytes(byte *bytes, size_t offset=0, size_t num_bits=0) const {

		if (num_bits == 0 || num_bits > blength)
			num_bits = blength;

		if (!bytes) {
			return (num_bits / 8) + ((num_bits < 8 || num_bits % 8) ? 1 : 0);
		}

		const size_t end = offset + num_bits;
		register byte b;
		register byte *bap = barray;
		register byte *bp = bytes;
		size_t n = 0;

		for (size_t i = offset; i < end; ++n) {
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
		}

		return n;
	}

	size_t to_ascii(FILE *f = NULL, size_t n = ~0, bool csv=false) const {

		if (!f) f = stdout;

		if (n == ~0 || n > blength) n = blength;

		for (size_t i = 0; i < n; ++i) {
			if (csv) {
				fprintf (f, "%u", (unsigned int)barray[i]);
				if (i < (n - 1)) fputc(',', f);
			} else
				fprintf (f, "%u ", (unsigned int)barray[i]);
		}
		fprintf(f, "\n");

		fflush(f);

		return n;
	}

	typedef	struct {
		size_t length;
		byte *bytes;
	} save_header;

	static size_t bit_count_to_byte_count(size_t n);

	int save(const char *filename, size_t n_bits = 0, save_header *header=NULL) const {

		size_t n = 0;

		if (n_bits == 0) n_bits = blength;

		size_t len = bit_count_to_byte_count(n_bits);
		byte *bytes = (byte *)malloc(len);
		to_bytes(bytes);

		unlink(filename);
		int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0666);
		if (fd < 1) return errno;

		if (header) n += write(fd, header->bytes, header->length);

		n += write(fd, bytes, len);

		close(fd);
		free(bytes);

		return n;
	}

	size_t run_length_encode(byte **encoding) const;

	static fast_bitstring *run_length_decode(const byte *rle_bytes, const size_t num_bytes);

	// Append n bits from FBS "bits" onto this, starting at this[offset].
	//
	// TODO: add an offset into bits.
	// TODO: needs unit test
	//
	size_t append(size_t offset, fast_bitstring &bits, size_t n = 0) {

		// If n == 0 then append all bits from "bits".
		if (n == 0) n = bits.length();

		// Check that we have capacity
		if (offset + n > this->blength) {
			// TODO: nice to have: expand to accommodate extra bits?
			printf("off/len/blen: %lu, %lu, %lu\n", offset, n, this->blength);
			assert(offset + n <= this->blength);
			throw "Insufficient room to append bits.";
		}

		size_t i = 0, j = offset;
		for (i = 0, j = offset; i < n; ) {
			barray[j++] = bits[i++];
		}

		return i;
	}


/*
	For future use (untested)

	class iter {
		private:
			iter() {}
			fast_bitstring *f;
			size_t i;

		public:
			iter(fast_bitstring &bs, size_t starting_index) {
				f = &bs;
				i = starting_index;
			}

			iter& operator += (const bool b) {
				(*f)[i++] = b ? 1 : 0;
				return *this;
			}

			byte operator ()() {
				return (*f)[i++];
			}

			iter& next(byte &bit) {
				bit = (*f)[i++];
				return *this;
			}
	};
 */

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
				// TODO: double check this offset skip logic, it may be defective.
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

