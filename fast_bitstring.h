
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define FBS_DEBUG

class fast_bitstring {

public:
        typedef unsigned char byte;

        // Construct bit array of all zero bits.
        fast_bitstring(const size_t length) {
                // one byte per bit
                blength = length * 8;
                barray = (byte *)calloc(blength, 1);
        }

        // Construct bit array from given bit string.
        fast_bitstring(const byte byte_array[], const size_t length) {
                explode(byte_array, length);
        }

        inline size_t length() const { return blength; }

        inline byte &operator [](const size_t i) const {
                return barray[i];
        }

#ifdef FBS_DEBUG
        void dump(void) const {
                printf("Dumping...\n");
                for (int i = 0; i < blength; ++i)
                        printf ("%X ", barray[i]);
				printf("\n");
				fflush(stdout);
        }
#endif

        size_t to_bitstring(byte *bytes, size_t offset, size_t num_bits) {

                if (num_bits == 0 || num_bits > blength)
                        num_bits = blength;

                const size_t end = offset + num_bits;
                register byte b;
                register byte *ba_ptr = barray;
                byte *bp = bytes;

                for (size_t i = offset; i < end; i += 8) {
                        b  = *barray++ << 7;
                        b |= *barray++ << 6;
                        b |= *barray++ << 5;
                        b |= *barray++ << 4;
                        b |= *barray++ << 3;
                        b |= *barray++ << 2;
                        b |= *barray++ << 1;
                        b |= *barray++;
                        *bp++ = b;
                }

                return  num_bits;
        }

protected:

        /*
         * Given a byte array containing a packed string of bits, explode the bits
         * into an array of bytes, one bit per byte.  Yes, this is an 8x increase
         * in memory utlization but that is the classic time/space trade off.
         */
        void explode(const byte byte_array[], const size_t length) {

                byte b, mask, *ba;
                int i, blen;

                blength = length * 8;
                barray = (byte *)calloc(blength, 1);

                for (i = 0, ba = barray; i < length; ++i) {
                        b = byte_array[i];
                        for (mask = 0x80; mask != 0x0; mask >>= 1) {
                                // TODO: could be made into a look up table, 256 entries long, or just 16 w/two look ups?.
                                // TODO: unwind this loop, at a minimum.
                                if (b & mask) *ba = 0x1;
								++ba;
                        }
                }
        }

        /*
         * Reduce exploded internal byte representation of bit string to a packed bit string in bytes.
         */
        byte *implode() const {

                return NULL;

        }

private:
        fast_bitstring() {}

        size_t blength;
        byte *barray;   // Array of bits, one per bit.

};

