//
// Created by Zeno Yang on 2022/11/2.
//

#include <iostream>
#include "containers/bitset.h"
#ifdef __AVX2__
#include <immintrin.h>  // avx2
//#include <nmmintrin.h>  // sse4.2
//#include <smmintrin.h>  // sse4.1
//#include <tmmintrin.h>  // sse3
//#include <pmmintrin.h>  // sse3
//#include <emmintrin.h>  // sse2
//#include <xmmintrin.h>  // sse
//#include <mmintrin.h>   // mxx
//#include <avx_mathfun>
#endif

#include "vectorclass/vectori256.h"

static uint64_t uint64_index_table[64] = {1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 32ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 16384ULL, 32768ULL, 65536ULL, 131072ULL, 262144ULL, 524288ULL, 1048576ULL, 2097152ULL, 4194304ULL, 8388608ULL, 16777216ULL, 33554432ULL, 67108864ULL, 134217728ULL, 268435456ULL, 536870912ULL, 1073741824ULL, 2147483648ULL, 4294967296ULL, 8589934592ULL, 17179869184ULL, 34359738368ULL, 68719476736ULL, 137438953472ULL, 274877906944ULL, 549755813888ULL, 1099511627776ULL, 2199023255552ULL, 4398046511104ULL, 8796093022208ULL, 17592186044416ULL, 35184372088832ULL, 70368744177664ULL, 140737488355328ULL, 281474976710656ULL, 562949953421312ULL, 1125899906842624ULL, 2251799813685248ULL, 4503599627370496ULL, 9007199254740992ULL, 18014398509481984ULL, 36028797018963968ULL, 72057594037927936ULL, 144115188075855872ULL, 288230376151711744ULL, 576460752303423488ULL, 1152921504606846976ULL, 2305843009213693952ULL, 4611686018427387904ULL, 9223372036854775808ULL};

/*
 * todo(zeno) need simd
 * // 1. AVX2没有_mm256_div_epi16指令，所以将uint16转为uint32_t进行处理，一次处理256/32=8个数据
 * // 2. 没找到AVX2取余指令，通过与或运算得到取余结果

// __m128i pos16 = _mm_lddqu_si128((const __m128i *)(list + i));
// __m256i pos32 = _mm256_cvtepi16_epi32(pos16);

// Vector of 16 16-bit unsigned integers
// Vec16us pos;
// pos.load(list + i);
// Vec16us offset = pos >> 6;
// pos % 64;
// pos & 1;
 */
static inline void _avx2_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
//    uint64_t offset, load, newload, pos, index;

     // Vector of 16 16-bit unsigned integers
     Vec16us pos;
     pos.load(list + i);
     Vec16us offset = pos >> 6;
     pos % 64;
     pos & 1;
     pos.store()


    // todo first 一次只处理4个list元素
//    for (uint64_t i = 0; i < length; i += 16) { // 256/16 -> 16
//         __m256i pos_vec = _mm256_lddqu_si256((const __m256i *)(list + i));    // 16 16-bit unsigned integers
//         __m256i offset_vec16 = _mm256_srli_epi16(pos_vec, 6);
//         __m256i vec63 = _mm256_set1_epi16(63);
//         __m256i index_vec16 = _mm256_and_si256(pos_vec, vec63);  // mod 64
//
//         __m256i offset_vec32 = offset_vec16;   // todo cvt ... offset_vec16拆成4个offset_vec，每个offset_set有4个offset
//         __m256i load_vec = _mm256_i32gather_epi32(words, offset_vec32, 8); // 4 word
//         __m256i index_vec64 = index_vec16; // todo index_vec16拆成4个index_vec64, 每个index_vec64有4个index
//         __m256i vec64_1 = _mm256_set1_epi64x(1ull);
//         // __m256i tmp = _mm256_sll_epi64(vec64_1, index_vec64);
//    }

    for (uint64_t i = 0; i < length; i += 16) { // 256/16 -> 16
        __m128i pos_vec_i16_1 = _mm_lddqu_si128((const __m128i *)(list + i));
        __m128i pos_vec_i16_2 = _mm_lddqu_si128((const __m128i *)(list + i + 8));

        __m128i offset_vec_i16_1 = _mm_srli_epi16(pos_vec_i16_1, 6);
        __m128i offset_vec_i16_2 = _mm_srli_epi16(pos_vec_i16_2, 6);

        __m128i vec63_i16 = _mm_set1_epi16(63);
        __m128i index_vec_i16_1 = _mm_and_si128(pos_vec_i16_1, vec63_i16);
        __m128i index_vec_i16_2 = _mm_and_si128(pos_vec_i16_2, vec63_i16);

        __m128i vec0_i16 = _mm_set1_epi16(0);
        __m128i offset_vec_i32_1_lo = _mm_unpacklo_epi16(vec0_i16, offset_vec_i16_1);   // 4 * 32
        __m128i offset_vec_i32_1_hi = _mm_unpackhi_epi16(vec0_i16, offset_vec_i16_1);
        __m128i offset_vec_i32_2_lo = _mm_unpacklo_epi16(vec0_i16, offset_vec_i16_2);
        __m128i offset_vec_i32_2_hi = _mm_unpackhi_epi16(vec0_i16, offset_vec_i16_1);

         __m256i load_vec_i64_1 = _mm256_i32gather_epi64(words, offset_vec_i32_1_lo, 8); // 4 word
         __m256i load_vec_i64_2 = _mm256_i32gather_epi64(words, offset_vec_i32_1_hi, 8); // 4 word
         __m256i load_vec_i64_3 = _mm256_i32gather_epi64(words, offset_vec_i32_2_lo, 8); // 4 word
         __m256i load_vec_i64_4 = _mm256_i32gather_epi64(words, offset_vec_i32_2_hi, 8); // 4 word

         __m128i index_vec_i32_1_lo = _mm_unpacklo_epi16(vec0_i16, index_vec_i16_1);
         __m128i index_vec_i32_1_hi = _mm_unpackhi_epi16(vec0_i16, index_vec_i16_1);
         __m128i index_vec_i32_2_lo = _mm_unpacklo_epi16(vec0_i16, index_vec_i16_2);
         __m128i index_vec_i32_2_hi = _mm_unpackhi_epi16(vec0_i16, index_vec_i16_2);

         __m256i index_uint64_vec_i64_1 = _mm256_i32gather_epi64(uint64_index_table, index_vec_i32_1_lo, 8);
         __m256i index_uint64_vec_i64_2 = _mm256_i32gather_epi64(uint64_index_table, index_vec_i32_1_hi, 8);
         __m256i index_uint64_vec_i64_3 = _mm256_i32gather_epi64(uint64_index_table, index_vec_i32_2_lo, 8);
         __m256i index_uint64_vec_i64_4 = _mm256_i32gather_epi64(uint64_index_table, index_vec_i32_2_hi, 8);

         __m256i newload_vec_i64_1 = _mm256_or_epi64(load_vec_i64_1, index_uint64_vec_i64_1);
         __m256i newload_vec_i64_2 = _mm256_or_epi64(load_vec_i64_2, index_uint64_vec_i64_2);
         __m256i newload_vec_i64_3 = _mm256_or_epi64(load_vec_i64_3, index_uint64_vec_i64_3);
         __m256i newload_vec_i64_4 = _mm256_or_epi64(load_vec_i64_4, index_uint64_vec_i64_4);



         // uint64_t vec_decode_table = {1, };
//         __m256 index_vec_i64_1 = _mm256_cvtepi32_epi64(index_vec_i32_1_lo);
//         __m256 index_vec_i64_2 = _mm256_cvtepi32_epi64(index_vec_i32_1_hi);
//         __m256 index_vec_i64_3 = _mm256_cvtepi32_epi64(index_vec_i32_2_lo);
//         __m256 index_vec_i64_4 = _mm256_cvtepi32_epi64(index_vec_i32_2_hi);


        // __m256i index_vec64 = index_vec16;
        // __m256i vec64_1 = _mm256_set1_epi64x(1ull);
        // __m256i tmp = _mm256_sll_epi64(vec64_1, index_vec64);
    }

    // todo 尾部处理
}

static inline void _scalar_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t offset, load, newload, pos, index;
    const uint16_t *end = list + length;
    while (list != end) {
        pos = *list;
        offset = pos >> 6;
        index = pos % 64;
        load = words[offset];
        newload = load | (UINT64_C(1) << index);
        words[offset] = newload;
        list++;
    }
}

int main() {
    int words_size = roaring::internal::BITSET_CONTAINER_SIZE_IN_WORDS;
    uint64_t words[words_size];
    for (int i = 0; i < words_size; ++i) {
        words[i] = UINT64_C(1);
    }

    int list_size = 1;
    uint16_t list[list_size];
    list[0] = 2u;

    _scalar_bitset_set_list(words, list, list_size);

    int n = 64;
    for (int i = 0; i < n; ++i) {
        uint64_t l = (uint64_t)(1) << i;
        std::cout << l << "ULL, ";
    }

    uint64_t arr[64] = {1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 32ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 16384ULL, 32768ULL, 65536ULL, 131072ULL, 262144ULL, 524288ULL, 1048576ULL, 2097152ULL, 4194304ULL, 8388608ULL, 16777216ULL, 33554432ULL, 67108864ULL, 134217728ULL, 268435456ULL, 536870912ULL, 1073741824ULL, 2147483648ULL, 4294967296ULL, 8589934592ULL, 17179869184ULL, 34359738368ULL, 68719476736ULL, 137438953472ULL, 274877906944ULL, 549755813888ULL, 1099511627776ULL, 2199023255552ULL, 4398046511104ULL, 8796093022208ULL, 17592186044416ULL, 35184372088832ULL, 70368744177664ULL, 140737488355328ULL, 281474976710656ULL, 562949953421312ULL, 1125899906842624ULL, 2251799813685248ULL, 4503599627370496ULL, 9007199254740992ULL, 18014398509481984ULL, 36028797018963968ULL, 72057594037927936ULL, 144115188075855872ULL, 288230376151711744ULL, 576460752303423488ULL, 1152921504606846976ULL, 2305843009213693952ULL, 4611686018427387904ULL, 9223372036854775808ULL};
    // {1uul}
    std::cout << std::endl << arr[63];


    return 0;
}