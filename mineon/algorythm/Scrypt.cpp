#include "stdafx.h"
#include "Scrypt.hpp"
#include "Sha2Utils.hpp"
#include "Job.hpp"
#include "Statistic.hpp"

//scrypt (1024x, 512bit) algorithm implementation

static void HMAC_SHA256_80_init (const uint32_t *key, __m256i& tstate, __m256i& ostate) {
	__m256i pad[2] = {
		_mm256_setr_epi32 (
			key[16], key[17], key[18], key[19],
			0x80000000, 0, 0, 0 //keypad head
		), _mm256_setr_epi32 (
			0, 0, 0, 0, 0, 0, 0, 0x00000280 //keypad tail
		)
	};

	/* tstate is assumed to contain the midstate of key */
	Sha2Utils::Sha256TransformAvx2 (&tstate, pad, false);
	
	const __m256i ihash = tstate;

	ostate = Sha2Utils::Sha256InitAvx2 ();
	pad[0] = _mm256_xor_si256 (ihash, _mm256_set1_epi32 (0x5c5c5c5c));
	pad[1] = _mm256_set1_epi32 (0x5c5c5c5c);
	Sha2Utils::Sha256TransformAvx2 (&ostate, pad, false);

	tstate = Sha2Utils::Sha256InitAvx2 ();
	pad[0] = _mm256_xor_si256 (ihash, _mm256_set1_epi32 (0x36363636));
	pad[1] = _mm256_set1_epi32 (0x36363636);
	Sha2Utils::Sha256TransformAvx2 (&tstate, pad, false);
}

static void PBKDF2_SHA256_80_128 (const __m256i tstate, const __m256i ostate, const uint32_t *salt, __m256i output[4]) {
	__m256i ibuf[2] = {
		_mm256_setr_epi32 (
			0, 0, 0, 0, 0,
			0x80000000, 0, 0 //innerpad head
		), _mm256_setr_epi32 (
			0, 0, 0, 0, 0, 0, 0, 0x000004a0 //innerpad tail
		)
	};

	__m256i obuf[2] = {
		_mm256_setzero_si256 ()
		, _mm256_setr_epi32 (
			0x80000000, 0, 0, 0, 0, 0, 0, 0x00000300 //outerpad
		)
	};

	__m256i istate = tstate;
	Sha2Utils::Sha256TransformAvx2 (&istate, (const __m256i*) salt, false);

	*(__m128i*) &ibuf[0] = _mm_loadu_si128 ((const __m128i*) &salt[16]);

	const __m256i swab = _mm256_setr_epi8 (
		0x03, 0x02, 0x01, 0x00,
		0x07, 0x06, 0x05, 0x04,
		0x0b, 0x0a, 0x09, 0x08,
		0x0f, 0x0e, 0x0d, 0x0c,

		0x03, 0x02, 0x01, 0x00,
		0x07, 0x06, 0x05, 0x04,
		0x0b, 0x0a, 0x09, 0x08,
		0x0f, 0x0e, 0x0d, 0x0c
	);

	//Step 1
	obuf[0] = istate;
	__m256i ostate2 = ostate;
	ibuf[0].m256i_u32[4] = 1;

	Sha2Utils::Sha256TransformAvx2 (obuf, ibuf, false);
	Sha2Utils::Sha256TransformAvx2 (&ostate2, obuf, false);
	output[0] = _mm256_shuffle_epi8 (ostate2, swab);

	//Step 2
	obuf[0] = istate;
	ostate2 = ostate;
	ibuf[0].m256i_u32[4] = 2;

	Sha2Utils::Sha256TransformAvx2 (obuf, ibuf, false);
	Sha2Utils::Sha256TransformAvx2 (&ostate2, obuf, false);
	output[1] = _mm256_shuffle_epi8 (ostate2, swab);

	//Step 3
	obuf[0] = istate;
	ostate2 = ostate;
	ibuf[0].m256i_u32[4] = 3;

	Sha2Utils::Sha256TransformAvx2 (obuf, ibuf, false);
	Sha2Utils::Sha256TransformAvx2 (&ostate2, obuf, false);
	output[2] = _mm256_shuffle_epi8 (ostate2, swab);

	//Step 4
	obuf[0] = istate;
	ostate2 = ostate;
	ibuf[0].m256i_u32[4] = 4;

	Sha2Utils::Sha256TransformAvx2 (obuf, ibuf, false);
	Sha2Utils::Sha256TransformAvx2 (&ostate2, obuf, false);
	output[3] = _mm256_shuffle_epi8 (ostate2, swab);
}

static void PBKDF2_SHA256_128_32 (__m256i& tstate, __m256i& ostate, const __m256i salt[4], uint32_t *output) {
	Sha2Utils::Sha256TransformAvx2 (&tstate, salt, true);
	Sha2Utils::Sha256TransformAvx2 (&tstate, salt + 2, true);

	__m256i finalblk[2] = {
		_mm256_setr_epi32 (
			0x00000001, 0x80000000, 0, 0, 0, 0, 0, 0
		), _mm256_setr_epi32 (
			0, 0, 0, 0, 0, 0, 0, 0x00000620
		)
	};

	Sha2Utils::Sha256TransformAvx2 (&tstate, finalblk, false);

	__m256i buf[2] = {
		tstate
		, _mm256_setr_epi32 (
			0x80000000, 0, 0, 0, 0, 0, 0, 0x00000300 //outerpad
		)
	};

	Sha2Utils::Sha256TransformAvx2 (&ostate, buf, false);

	const __m256i swab = _mm256_setr_epi8 (
		0x03, 0x02, 0x01, 0x00,
		0x07, 0x06, 0x05, 0x04,
		0x0b, 0x0a, 0x09, 0x08,
		0x0f, 0x0e, 0x0d, 0x0c,

		0x03, 0x02, 0x01, 0x00,
		0x07, 0x06, 0x05, 0x04,
		0x0b, 0x0a, 0x09, 0x08,
		0x0f, 0x0e, 0x0d, 0x0c
	);

	*(__m256i*) output =  _mm256_shuffle_epi8 (ostate, swab);
}

static __m256i speedupSalsaCalcX[16];

static void sp_prepare_salsa8_parallel (__m256i input[2 * SCRYPT_THREAD_COUNT], __m256i output[2 * SCRYPT_THREAD_COUNT], uint32_t threadLen) {
	//8x input[0] -> x00..08 (xorX[thread*2 + 0]), input[1] -> x09..x15 (xorX[thread*2 + 1])
	//__m256i xorX[16] = {
		uint8_t i = SCRYPT_THREAD_COUNT;
		while (i--) {
			output[i * threadLen + 0] = _mm256_xor_si256 (output[i * threadLen + 0], input[i * threadLen + 0]);
			output[i * threadLen + 1] = _mm256_xor_si256 (output[i * threadLen + 1], input[i * threadLen + 1]);
		}
	//};

	//Transpose matrix (calcX[i] = xorX[0].m256i_u32[i] <= i=0..7, calcX[i] = xorX[1].m256i_u32[i-8] <= i=8..15)
	const __m256i vindex = _mm256_setr_epi32 (0, threadLen * 8, 2 * threadLen * 8, 3 * threadLen * 8, 4 * threadLen * 8, 5 * threadLen * 8, 6 * threadLen * 8, 7 * threadLen * 8);
	const int* calcX = (const int*) output;

	i = 16;
	while (i--) {
		speedupSalsaCalcX[i] = _mm256_i32gather_epi32 (calcX + i, vindex, 4);
	}
}

static void sp_salsa8_step_tile (__m256i& res1, __m256i& res2, __m256i& res3, __m256i& res4, 
	__m256i fadd1, __m256i fadd2, __m256i fadd3, __m256i fadd4,
	__m256i sadd1, __m256i sadd2, __m256i sadd3, __m256i sadd4,
	uint32_t shift)
{
	__m256i add1 = _mm256_add_epi32 (fadd1, sadd1);
	__m256i add2 = _mm256_add_epi32 (fadd2, sadd2);
	__m256i add3 = _mm256_add_epi32 (fadd3, sadd3);
	__m256i add4 = _mm256_add_epi32 (fadd4, sadd4);

	__m256i shl1 = _mm256_slli_epi32 (add1, shift);
	__m256i shl2 = _mm256_slli_epi32 (add2, shift);
	__m256i shl3 = _mm256_slli_epi32 (add3, shift);
	__m256i shl4 = _mm256_slli_epi32 (add4, shift);

	__m256i shr1 = _mm256_srli_epi32 (add1, 32 - shift);
	__m256i shr2 = _mm256_srli_epi32 (add2, 32 - shift);
	__m256i shr3 = _mm256_srli_epi32 (add3, 32 - shift);
	__m256i shr4 = _mm256_srli_epi32 (add4, 32 - shift);

	__m256i or1 = _mm256_or_si256 (shl1, shr1);
	__m256i or2 = _mm256_or_si256 (shl2, shr2);
	__m256i or3 = _mm256_or_si256 (shl3, shr3);
	__m256i or4 = _mm256_or_si256 (shl4, shr4);

	res1 = _mm256_xor_si256 (res1, or1);
	res2 = _mm256_xor_si256 (res2, or2);
	res3 = _mm256_xor_si256 (res3, or3);
	res4 = _mm256_xor_si256 (res4, or4);
}

static void sp_salsa8_parallel () {
	__m256i* calcX = speedupSalsaCalcX;

	uint32_t stepCount = 4;
	while (stepCount--) {
		//First tile block
		sp_salsa8_step_tile (
			calcX[ 4], calcX[ 9], calcX[14], calcX[ 3],
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			calcX[12], calcX[ 1], calcX[ 6], calcX[11],
			7);

		sp_salsa8_step_tile (
			calcX[ 8], calcX[13], calcX[ 2], calcX[ 7],
			calcX[ 4], calcX[ 9], calcX[14], calcX[ 3],
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			9);

		sp_salsa8_step_tile (
			calcX[12], calcX[ 1], calcX[ 6], calcX[11],
			calcX[ 8], calcX[13], calcX[ 2], calcX[ 7],
			calcX[ 4], calcX[ 9], calcX[14], calcX[ 3],
			13);

		sp_salsa8_step_tile (
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			calcX[12], calcX[ 1], calcX[ 6], calcX[11],
			calcX[ 8], calcX[13], calcX[ 2], calcX[ 7],
			18);

		//Second tile block
		sp_salsa8_step_tile (
			calcX[ 1], calcX[ 6], calcX[11], calcX[12],
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			calcX[ 3], calcX[ 4], calcX[ 9], calcX[14],
			7);

		sp_salsa8_step_tile (
			calcX[ 2], calcX[ 7], calcX[ 8], calcX[13],
			calcX[ 1], calcX[ 6], calcX[11], calcX[12],
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			9);

		sp_salsa8_step_tile (
			calcX[ 3], calcX[ 4], calcX[ 9], calcX[14],
			calcX[ 2], calcX[ 7], calcX[ 8], calcX[13],
			calcX[ 1], calcX[ 6], calcX[11], calcX[12],
			13);

		sp_salsa8_step_tile (
			calcX[ 0], calcX[ 5], calcX[10], calcX[15],
			calcX[ 3], calcX[ 4], calcX[ 9], calcX[14],
			calcX[ 2], calcX[ 7], calcX[ 8], calcX[13],
			18);
	}
}

static void sp_postprocess_salsa8_parallel (__m256i output[2 * SCRYPT_THREAD_COUNT], uint32_t threadLen) {
	//Transpose back (gather thread results -> xX[i] = calcX[0..7].m256i_u32[i], and xX[i + 8] = calcX[8..15].m256i_u32[i])
	const __m256i vindex = _mm256_setr_epi32 (0, 8, 16, 24, 32, 40, 48, 56);
	const int* calcX = (const int*) speedupSalsaCalcX;

	//Calculate output
	uint8_t thread = SCRYPT_THREAD_COUNT;
	while (thread--) {
		output[thread * threadLen + 0] = _mm256_add_epi32 (output[thread * threadLen + 0], _mm256_i32gather_epi32 (calcX + 0 * 8 + thread, vindex, 4));
		output[thread * threadLen + 1] = _mm256_add_epi32 (output[thread * threadLen + 1], _mm256_i32gather_epi32 (calcX + 8 * 8 + thread, vindex, 4));
	}
}

static __m256i speedupScryptV[1024 * 4 * SCRYPT_THREAD_COUNT];

static void sp_scrypt_prepare_pass1_step (uint32_t cycle, __m256i X[4 * SCRYPT_THREAD_COUNT]) {
	uint8_t thread = SCRYPT_THREAD_COUNT;
	while (thread--) {
		speedupScryptV[(thread * 1024 + cycle) * 4 + 0] = X[thread * 4 + 0];
		speedupScryptV[(thread * 1024 + cycle) * 4 + 1] = X[thread * 4 + 1];
		speedupScryptV[(thread * 1024 + cycle) * 4 + 2] = X[thread * 4 + 2];
		speedupScryptV[(thread * 1024 + cycle) * 4 + 3] = X[thread * 4 + 3];
	}
}

static void sp_scrypt_prepare_pass2_step (__m256i X[4 * SCRYPT_THREAD_COUNT]) {
	uint8_t thread = SCRYPT_THREAD_COUNT;
	while (thread--) {
		uint32_t j = 4 * (X[thread * 4 + 2].m256i_u32[0] & (1024 - 1));

		__m256i* XPtr = &X[thread * 4];
		XPtr[0] = _mm256_xor_si256 (XPtr[0], speedupScryptV[thread * 1024 * 4 + j + 0]);
		XPtr[1] = _mm256_xor_si256 (XPtr[1], speedupScryptV[thread * 1024 * 4 + j + 1]);
		XPtr[2] = _mm256_xor_si256 (XPtr[2], speedupScryptV[thread * 1024 * 4 + j + 2]);
		XPtr[3] = _mm256_xor_si256 (XPtr[3], speedupScryptV[thread * 1024 * 4 + j + 3]);
	}
}

static void sp_scrypt_core (__m256i X[4 * SCRYPT_THREAD_COUNT]) {
	uint16_t step = 1024;
	while (step--) {
		sp_scrypt_prepare_pass1_step ((uint32_t) 1024 - step - 1, X);

		sp_prepare_salsa8_parallel (&X[2], &X[0], 4);
		sp_salsa8_parallel ();
		sp_postprocess_salsa8_parallel (&X[0], 4);

		sp_prepare_salsa8_parallel (&X[0], &X[2], 4);
		sp_salsa8_parallel ();
		sp_postprocess_salsa8_parallel (&X[2], 4);
	}

	step = 1024;
	while (step--) {
		sp_scrypt_prepare_pass2_step (X);

		sp_prepare_salsa8_parallel (&X[2], &X[0], 4);
		sp_salsa8_parallel ();
		sp_postprocess_salsa8_parallel (&X[0], 4);

		sp_prepare_salsa8_parallel (&X[0], &X[2], 4);
		sp_salsa8_parallel ();
		sp_postprocess_salsa8_parallel (&X[2], 4);
	}
}

static void sp_scrypt_1024_1_1_256 (const uint32_t *input, uint32_t *output, const __m256i midstate) {
	__m256i ostate[SCRYPT_THREAD_COUNT];
	__m256i X[4 * SCRYPT_THREAD_COUNT];

	__m256i tstate[SCRYPT_THREAD_COUNT] = {
		midstate, midstate, midstate, midstate, midstate, midstate, midstate, midstate
	};

	uint8_t thread = SCRYPT_THREAD_COUNT;
	while (thread--) {
		HMAC_SHA256_80_init (&input[thread * 20], tstate[thread], ostate[thread]);
		PBKDF2_SHA256_80_128 (tstate[thread], ostate[thread], &input[thread * 20], &X[thread * 4]);
	}

	sp_scrypt_core (X);

	thread = SCRYPT_THREAD_COUNT;
	while (thread--) {
		PBKDF2_SHA256_128_32 (tstate[thread], ostate[thread], &X[thread * 4], &output[thread * 8]);
	}
}

//Scrypt class implementation

bool Scrypt::FullTestHash (const uint32_t hash[8]) const {
	bool resCode = true;

	uint32_t idx = 8;
	while (idx--) {
		if (hash[idx] > mTarget[idx]) {
			resCode = false;
			break;
		}

		if (hash[idx] < mTarget[idx]) {
			resCode = true;
			break;
		}
	}

	return resCode;
}

void Scrypt::DiffToTarget (double difficulty) {
	double diff = difficulty / 65536.0;

	int32_t k = 0;
	for (k = 6; k > 0 && diff > 1.0; k--) {
		diff /= 4294967296.0;
	}
	uint64_t m = (uint64_t) (4294901760.0 / diff);
	if (m == 0 && k == 6) {
		memset (&mTarget[0], 0xff, 8 * sizeof (uint32_t));
	} else {
		memset (&mTarget[0], 0, 8 * sizeof (uint32_t));
		mTarget[k] = (uint32_t) m;
		mTarget[k + 1] = (uint32_t) (m >> 32);
	}
}

void Scrypt::PreConditionScan (uint32_t threadIndex, Statistic& statistic) {
	//Do the precondition scan
	uint32_t distNonce = (mEndNonce - mStartNonce) / (SCRYPT_THREAD_COUNT + 1);

	uint32_t initIndex = SCRYPT_THREAD_COUNT;
	while (initIndex--) {
		mData[initIndex * 20 + 19] = initIndex * distNonce;
	}

	sp_scrypt_1024_1_1_256 (mData, mHash, *(const __m256i*) mMidState);

	//Calculate distances
	std::vector<uint32_t> targetDistances (SCRYPT_THREAD_COUNT, 0xffffffffu);
	initIndex = SCRYPT_THREAD_COUNT;
	while (initIndex--) {
		if (mHash[initIndex * 8 + 7] <= mTarget[7]) { //Possibly good result
			targetDistances[initIndex] = 0;
		} else { //Bad result
			targetDistances[initIndex] = mHash[initIndex * 8 + 7] - mTarget[7];
		}
	}

	//Choose optimal start nonce value (maximal distance)
	uint32_t minIndex = 0;
	uint32_t minDistance = 0xffffffffu;

	initIndex = SCRYPT_THREAD_COUNT;
	while (initIndex--) {
		if (targetDistances[initIndex] < minDistance) {
			minDistance = targetDistances[initIndex];
			minIndex = initIndex;
		}
	}

	//Set start nonce value
	mNonce = minIndex * distNonce;
}

Scrypt::Scrypt () :
	mState (States::Precondition),
	mBreakScan (false),
	mStartNonce (0),
	mEndNonce (0),
	mNonce (0)
{
}

void* Scrypt::operator new (std::size_t size) {
	void* ptr = _aligned_malloc(size, 32);
	if (ptr) {
		::new(ptr) Scrypt();
	}
	return ptr;
}

void* Scrypt::operator new [] (std::size_t size) {
	void* ptr = _aligned_malloc(size, 32);
	if (ptr) {
		size_t cnt = size / sizeof (Scrypt);
		while (cnt--) {
			::new(((Scrypt*)ptr) + cnt) Scrypt();
		}
	}
	return ptr;
}

void Scrypt::operator delete (void* ptr, std::size_t size) {
	((Scrypt*)ptr)->~Scrypt();
	_aligned_free(ptr);
}

void Scrypt::operator delete [] (void* ptr, std::size_t size) {
	size_t cnt = size / sizeof (Scrypt);
	while (cnt--) {
		((Scrypt*)ptr)[cnt].~Scrypt();
	}
	_aligned_free(ptr);
}

bool Scrypt::Prepare (std::shared_ptr<Job> job, uint32_t nonceStart, uint32_t nonceCount) {
	if (job == nullptr) {
		return false;
	}

	mBreakScan = false;

	if (mJobID != job->jobID) { //New job arrived
		mState = States::Precondition;
		mJobID = job->jobID;
		mStartNonce = nonceStart;
		mEndNonce = nonceStart + nonceCount;
		mNonce = mStartNonce;

		DiffToTarget (job->difficulty);

		uint32_t initIndex = SCRYPT_THREAD_COUNT;
		while (initIndex--) {
			memcpy (mData + initIndex * 20, &job->data[0], 20 * sizeof (uint32_t));
		}

		__m256i* midState = (__m256i*) mMidState;
		*midState = Sha2Utils::Sha256InitAvx2 ();
		Sha2Utils::Sha256TransformAvx2 ((__m256i*) mMidState, (__m256i*)mData, false);
	} else { //Continue the current job
		mStartNonce = mNonce;
	}

	return true;
}

Algorythm::ScanResults Scrypt::Scan (uint32_t threadIndex, Statistic& statistic) {
	ScanResults result;
	result.scanStart = std::chrono::system_clock::now ();

	statistic.ScanStarted (threadIndex, mJobID, result.scanStart, mNonce, mEndNonce);

	if (mState == States::Precondition) {
		PreConditionScan (threadIndex, statistic);
		mState = States::Scan;
	}

	int32_t nonceIncrement = 0;
	if (!result.foundNonce) {
		do {
			uint32_t initIndex = SCRYPT_THREAD_COUNT;
			while (initIndex--) {
				++nonceIncrement;
				mNonce += nonceIncrement % 2 == 0 ? -nonceIncrement : nonceIncrement;
				mData[initIndex * 20 + 19] = mNonce;
			}

			sp_scrypt_1024_1_1_256 (mData, mHash, *(const __m256i*) mMidState);

			statistic.ScanStepEnded (threadIndex, mJobID, mNonce);

			initIndex = SCRYPT_THREAD_COUNT;
			while (!result.foundNonce && initIndex--) {
				if (mHash[initIndex * 8 + 7] <= mTarget[7] && FullTestHash (&mHash[initIndex * 8])) {
					result.foundNonce = true;
					result.nTime = mData[initIndex * 20 + 17];
					result.nonce = mData[initIndex * 20 + 19];
				}
			}
		} while (mNonce < mEndNonce && !result.foundNonce && !mBreakScan);
	}

	result.hashesScanned = nonceIncrement; // mNonce - mStartNonce;
	result.scanDuration = std::chrono::system_clock::now () - result.scanStart;

	statistic.ScanEnded (threadIndex, mJobID, result.scanDuration, result.hashesScanned, result.foundNonce, result.nonce);

	return result;
}

void Scrypt::BreakScan () {
	mBreakScan = true;
}

