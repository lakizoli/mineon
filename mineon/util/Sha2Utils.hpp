#pragma once

class Sha2Utils {
	static const uint32_t sha256_k[64];

public:
	static __m256i Sha256InitAvx2 () {
		return _mm256_setr_epi32 (
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
		);
	}

	/*
	* SHA256 block compression function (using AVX2 intrinsics).
	* The 256-bit state is transformed via the 512-bit input block to produce a new state.
	*/
	static void Sha256TransformAvx2 (__m256i state[1], const __m256i block[2], bool swap);
};
