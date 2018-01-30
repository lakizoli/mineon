#pragma once

class Sha2Utils {
	static const uint32_t sha256_h[8];
	static const uint32_t sha256_k[64];
	static const uint32_t sha256d_hash1[16];

	static void BigEndianUInt32Encode (uint8_t dest[4], uint32_t value);
	static uint32_t BigEndianUInt32Decode (const uint8_t value[4]);

public:
	static __m256i Sha256InitAvx2 () {
		return _mm256_setr_epi32 (
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
		);
	}

	static void Sha256Init (uint32_t state[8]) {
		memcpy (state, sha256_h, 8 * sizeof (uint32_t));
	}

	/*
	* SHA256 block compression function (using AVX2 intrinsics).
	* The 256-bit state is transformed via the 512-bit input block to produce a new state.
	*/
	static void Sha256TransformAvx2 (__m256i state[1], const __m256i block[2], bool swap);

	/*
	* SHA256 block compression function (without intrinsics).
	* The 256-bit state is transformed via the 512-bit input block to produce a new state.
	*/
	static void Sha256Transform (uint32_t state[8], const uint32_t block[16], bool swap);

	static void Sha256d (uint8_t* hash, const uint8_t* data, int32_t len);
};
