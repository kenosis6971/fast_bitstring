/*
 *	fast_bitstring.h
 *
 *	Fast bit string converts a bit string packed into an array of bytes
 *	into an array of bytes, one byte for each bit of the of the source
 *	array of bits.  This representation is designed for two purposes only:
 *	speed and convenience of use, i.e., KISS.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define FBS_DEBUG

/*
 * If we encapsulate bit in a struct then we can overload = to ensure
 * value is 0 or 1.
 */

class fast_bitstring {

public:
        typedef unsigned char byte;

        // Construct bit array of all zero bits.
        fast_bitstring(const size_t length_in_bytes) : BITS_PER_BYTE(8) {
                blength = length_in_bytes * BITS_PER_BYTE;
                barray = (byte *)calloc(blength, 1);
        }

        // Construct bit array from given bit string backed in byte_array.
        fast_bitstring(const byte byte_array[], const size_t length_in_bytes) : BITS_PER_BYTE(8) {
                explode(byte_array, length_in_bytes);
        }

	// Length of bit string in bits.
        inline size_t length() const { return blength; }

	// Length of bit string in bits.
        inline void clear() { memset((void *)barray, 0, blength); }

	// Array opperator to access bit[i].
        inline byte &operator [](const size_t i) const {
                return barray[i];
        }

	// Convert internal byte per bit representation back to bits packed into
	// given byte array.
        size_t to_bitstring(byte *bytes, size_t offset=0, size_t num_bits=0) {

                if (num_bits == 0 || num_bits > blength)
                        num_bits = blength;

                const size_t end = offset + num_bits;
                register byte b;
                register byte *ba_ptr = barray;
                register byte *bp = bytes;

                for (size_t i = offset; i < end; i += BITS_PER_BYTE) {
                        b  = (*ba_ptr++ & 1) << 7;
                        b |= (*ba_ptr++ & 1) << 6;
                        b |= (*ba_ptr++ & 1) << 5;
                        b |= (*ba_ptr++ & 1) << 4;
                        b |= (*ba_ptr++ & 1) << 3;
                        b |= (*ba_ptr++ & 1) << 2;
                        b |= (*ba_ptr++ & 1) << 1;
                        b |= (*ba_ptr++ & 1);
                        *bp++ = b;
                }

                return num_bits;
        }

#ifdef FBS_DEBUG
        void dump(void) const {
                printf("Dumping...\n");
                for (int i = 0; i < blength; ++i)
                        printf ("%u ", barray[i]);
				printf("\n");
				fflush(stdout);
        }
#endif

protected:

        /*
         * Given a byte array containing a packed string of bits, explode the bits
         * into an array of bytes, one bit per byte.  Yes, this is an 8x increase
         * in memory utlization but that is the classic time/space trade off for you.
         */
        void explode(const byte byte_array[], const size_t length) {

                register byte b, mask, *ba;
                int i, blen;

                blength = length * BITS_PER_BYTE;
                barray = (byte *)calloc(blength, 1);

                for (i = 0, ba = barray; i < length; ++i) {
                        b = byte_array[i];
                        for (mask = 0x80; mask != 0x0; mask >>= 1) {
                                if (b & mask) *ba = 0x1;
								++ba;
                        }
                }
        }

private:

        fast_bitstring() : BITS_PER_BYTE(8) {}

		const size_t BITS_PER_BYTE;
        size_t blength;
        byte *barray;   // Array of bits, one per bit.

};

