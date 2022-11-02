//
// Created by Zeno Yang on 2022/11/2.
//

#include <iostream>
#include "containers/bitset.h"
#ifdef __AVX2__
#include <immintrin.h>  // avx2
#include <nmmintrin.h>  // sse4.2
#include <smmintrin.h>  // sse4.1
#include <tmmintrin.h>  // sse3
#include <pmmintrin.h>  // sse3
#include <emmintrin.h>  // sse2
#include <xmmintrin.h>  // sse
#include <mmintrin.h>   // mxx

#include <avx_mathfun>
#endif

// todo(zeno) need simd
/*
 * 1. AVX2没有_mm256_div_epi16指令，所以将uint16转为uint32_t进行处理，一次处理256/32=8个数据
 * 2. 没找到AVX2取余指令，通过与或运算得到取余结果
 */
static inline void _avx2_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t offset, load, newload, pos, index;

    for (uint64_t i = 0; i < length; i += 8) { // 256/32 -> 16
        __m256i A = _mm256_lddqu_si256((const __m256i *)(list + i));    // ymm 16 uint16_t
        // set 64 ymm
        __m256i div_mod_64_vec = _mm256_set1_epi16(64);
//        __m256i offset = _mm256_div_epi16(A, div_mod_64_vec);
        __m256i offset = _mm256_div_pd(A, div_mod_64_vec);


    }

    // 尾部处理
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

    return 0;
}