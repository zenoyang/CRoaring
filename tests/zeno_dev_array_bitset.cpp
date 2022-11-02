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
    uint64_t offset, load, newload, pos, index;

    // todo first 一次只处理4个list元素
    for (uint64_t i = 0; i < length; i += 16) { // 256/16 -> 16
         __m256i pos_vec = _mm256_lddqu_si256((const __m256i *)(list + i));    // 16 16-bit unsigned integers
         __m256i offset_vec16 = _mm256_srli_epi16(pos_vec, 6);
         __m256i vec63 = _mm256_set1_epi16(63);
         __m256i index_vec16 = _mm256_and_si256(pos_vec, vec63);  // mod 64

         __m256i offset_vec32 = offset_vec16;   // todo cvt ... offset_vec16拆成4个offset_vec，每个offset_set有4个offset
         __m256i load_vec = _mm256_i32gather_epi32(words, offset_vec32, 8); // 4 word
         __m256i index_vec64 = index_vec16; // todo index_vec16拆成4个index_vec64, 每个index_vec64有4个index
         __m256i vec64_1 = _mm256_set1_epi64x(1ull);
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

    return 0;
}