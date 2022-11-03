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
#endif

static uint64_t uint64_table[64] = {1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 32ULL, 64ULL, 128ULL, 256ULL, 512ULL, 1024ULL, 2048ULL, 4096ULL, 8192ULL, 16384ULL, 32768ULL, 65536ULL, 131072ULL, 262144ULL, 524288ULL, 1048576ULL, 2097152ULL, 4194304ULL, 8388608ULL, 16777216ULL, 33554432ULL, 67108864ULL, 134217728ULL, 268435456ULL, 536870912ULL, 1073741824ULL, 2147483648ULL, 4294967296ULL, 8589934592ULL, 17179869184ULL, 34359738368ULL, 68719476736ULL, 137438953472ULL, 274877906944ULL, 549755813888ULL, 1099511627776ULL, 2199023255552ULL, 4398046511104ULL, 8796093022208ULL, 17592186044416ULL, 35184372088832ULL, 70368744177664ULL, 140737488355328ULL, 281474976710656ULL, 562949953421312ULL, 1125899906842624ULL, 2251799813685248ULL, 4503599627370496ULL, 9007199254740992ULL, 18014398509481984ULL, 36028797018963968ULL, 72057594037927936ULL, 144115188075855872ULL, 288230376151711744ULL, 576460752303423488ULL, 1152921504606846976ULL, 2305843009213693952ULL, 4611686018427387904ULL, 9223372036854775808ULL};

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

/// todo(zeno) need test
static inline void _avx2_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t i = 0;
    __m128i vec63_i16 = _mm_set1_epi16(63);
    __m128i vec0_i16 = _mm_set1_epi16(0);
    const uint64_t size = sizeof(__m256i) / sizeof(uint16_t); // 256/16 -> 16
    uint64_t newloads[size];
    uint16_t offsets[size];
    for (; i + size < length; i += size) {
        __m128i pos_vec_i16_1 = _mm_lddqu_si128((const __m128i *)(list + i));
        __m128i pos_vec_i16_2 = _mm_lddqu_si128((const __m128i *)(list + i + 8));

        __m128i offset_vec_i16_1 = _mm_srli_epi16(pos_vec_i16_1, 6);
        __m128i offset_vec_i16_2 = _mm_srli_epi16(pos_vec_i16_2, 6);

        __m128i index_vec_i16_1 = _mm_and_si128(pos_vec_i16_1, vec63_i16);
        __m128i index_vec_i16_2 = _mm_and_si128(pos_vec_i16_2, vec63_i16);

        /// 没有_mm256_i16gather_*指令，因此将offset_vec_i16拆成多个offset_vec_i32
        __m128i offset_vec_i32_1 = _mm_unpacklo_epi16(vec0_i16, offset_vec_i16_1);   // 4 * 32
        __m128i offset_vec_i32_2 = _mm_unpackhi_epi16(vec0_i16, offset_vec_i16_1);
        __m128i offset_vec_i32_3 = _mm_unpacklo_epi16(vec0_i16, offset_vec_i16_2);
        __m128i offset_vec_i32_4 = _mm_unpackhi_epi16(vec0_i16, offset_vec_i16_2);
        __m256i load_vec_i64_1 = _mm256_i32gather_epi64(words, offset_vec_i32_1, 8); // 4 word
        __m256i load_vec_i64_2 = _mm256_i32gather_epi64(words, offset_vec_i32_2, 8);
        __m256i load_vec_i64_3 = _mm256_i32gather_epi64(words, offset_vec_i32_3, 8);
        __m256i load_vec_i64_4 = _mm256_i32gather_epi64(words, offset_vec_i32_4, 8);

        __m128i index_vec_i32_1 = _mm_unpacklo_epi16(vec0_i16, index_vec_i16_1);
        __m128i index_vec_i32_2 = _mm_unpackhi_epi16(vec0_i16, index_vec_i16_1);
        __m128i index_vec_i32_3 = _mm_unpacklo_epi16(vec0_i16, index_vec_i16_2);
        __m128i index_vec_i32_4 = _mm_unpackhi_epi16(vec0_i16, index_vec_i16_2);
        __m256i index_uint64_vec_i64_1 = _mm256_i32gather_epi64(uint64_table, index_vec_i32_1, 8);
        __m256i index_uint64_vec_i64_2 = _mm256_i32gather_epi64(uint64_table, index_vec_i32_2, 8);
        __m256i index_uint64_vec_i64_3 = _mm256_i32gather_epi64(uint64_table, index_vec_i32_3, 8);
        __m256i index_uint64_vec_i64_4 = _mm256_i32gather_epi64(uint64_table, index_vec_i32_4, 8);

        __m256i newload_vec_i64_1 = _mm256_or_epi64(load_vec_i64_1, index_uint64_vec_i64_1);
        __m256i newload_vec_i64_2 = _mm256_or_epi64(load_vec_i64_2, index_uint64_vec_i64_2);
        __m256i newload_vec_i64_3 = _mm256_or_epi64(load_vec_i64_3, index_uint64_vec_i64_3);
        __m256i newload_vec_i64_4 = _mm256_or_epi64(load_vec_i64_4, index_uint64_vec_i64_4);

        /// avx2没有scatter指令，因此将newload和offset拷贝到内存上再循环处理
        _mm256_storeu_si256((__m256i *)newloads, newload_vec_i64_1);
        _mm256_storeu_si256((__m256i *)(newloads + 4), newload_vec_i64_2);
        _mm256_storeu_si256((__m256i *)(newloads + 8), newload_vec_i64_3);
        _mm256_storeu_si256((__m256i *)(newloads + 12), newload_vec_i64_4);
        _mm_storeu_si128((__m128i *)offsets, offset_vec_i16_1);
        _mm_storeu_si128((__m128i *)(offsets + 8), offset_vec_i16_1);
        for (uint64_t j = 0; j < size; ++j) {
            words[offsets[j]] = newloads[j];
        }
    }

    if (i < length) {
        _scalar_bitset_set_list(words, list + i, length - i);
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