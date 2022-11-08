//
// Created by Zeno Yang on 2022/11/2.
//

#include <iostream>
#include <immintrin.h>  // avx2
#include <chrono>
using namespace std::chrono;

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

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

static inline uint64_t _asm_bitset_set_list_withcard(uint64_t *words, uint64_t card,
                                                     const uint16_t *list, uint64_t length) {
    uint64_t offset, load, pos;
    uint64_t shift = 6;
    const uint16_t *end = list + length;
    if (!length) return card;
    // TODO: could unroll for performance, see bitset_set_list
    // bts is not available as an intrinsic in GCC
    __asm volatile(
        "1:\n"
        "movzwq (%[list]), %[pos]\n"
        "shrx %[shift], %[pos], %[offset]\n"
        "mov (%[words],%[offset],8), %[load]\n"
        "bts %[pos], %[load]\n"
        "mov %[load], (%[words],%[offset],8)\n"
        "sbb $-1, %[card]\n"
        "add $2, %[list]\n"
        "cmp %[list], %[end]\n"
        "jnz 1b"
        : [card] "+&r"(card), [list] "+&r"(list), [load] "=&r"(load),
          [pos] "=&r"(pos), [offset] "=&r"(offset)
        : [end] "r"(end), [words] "r"(words), [shift] "r"(shift));
    return card;
}

static inline uint64_t _scalar_bitset_set_list_withcard(uint64_t *words, uint64_t card,
                                                      const uint16_t *list, uint64_t length) {
    uint64_t offset, load, newload, pos, index;
    const uint16_t *end = list + length;
    while (list != end) {
        pos = *list;
        offset = pos >> 6;
        index = pos % 64;
        load = words[offset];
        newload = load | (UINT64_C(1) << index);
        card += (load ^ newload) >> index;
        words[offset] = newload;
        list++;
    }
    return card;
}

static inline void _avx2_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t i = 0;
    const uint64_t inner_loop = sizeof(__m256i) / sizeof(uint16_t); // 256/16 -> 16
    uint16_t offsets[inner_loop];
    uint16_t indexs[inner_loop];
    __m256i vec63_i16 = _mm256_set1_epi16(63);
    uint64_t* word;
    for (; i + inner_loop <= length; i += inner_loop) {
        __m256i vec_pos_i16 = _mm256_lddqu_si256((const __m256i *)(list + i));
        __m256i vec_offset_i16 = _mm256_srli_epi16(vec_pos_i16, 6);
        __m256i vec_index_i16 = _mm256_and_si256(vec_pos_i16, vec63_i16);
        _mm256_store_si256((__m256i *)offsets, vec_offset_i16);
        _mm256_store_si256((__m256i *)indexs, vec_index_i16);
        for (uint64_t j = 0; j < inner_loop; ++j) {
            word = &words[offsets[j]];
            *word = *word | (UINT64_C(1) << indexs[j]);
        }
    }

    if (i < length) {
        _scalar_bitset_set_list(words, list + i, length - i);
    }
}

static inline void _avx2_bitset_set_list2(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t i = 0;
    __m128i vec63_i16 = _mm_set1_epi16(63);
    __m128i vec0_i16 = _mm_set1_epi16(0);
    const uint64_t inner_loop = sizeof(__m256i) / sizeof(uint16_t); // 256/16 -> 16
    uint64_t newloads[inner_loop];
    uint16_t offsets[inner_loop];
    uint64_t tmp[inner_loop];
    uint64_t* word;
    for (; i + inner_loop <= length; i += inner_loop) {
        __m128i pos_vec_i16_1 = _mm_lddqu_si128((const __m128i *)(list + i));
        __m128i pos_vec_i16_2 = _mm_lddqu_si128((const __m128i *)(list + i + 8));

        __m128i offset_vec_i16_1 = _mm_srli_epi16(pos_vec_i16_1, 6);
        __m128i offset_vec_i16_2 = _mm_srli_epi16(pos_vec_i16_2, 6);

        __m128i index_vec_i16_1 = _mm_and_si128(pos_vec_i16_1, vec63_i16);
        __m128i index_vec_i16_2 = _mm_and_si128(pos_vec_i16_2, vec63_i16);

        /// 没有_mm256_i16gather_*指令，因此将offset_vec_i16拆成多个offset_vec_i32
        __m128i offset_vec_i32_1 = _mm_unpacklo_epi16(offset_vec_i16_1, vec0_i16);   // 4 * 32
        __m128i offset_vec_i32_2 = _mm_unpackhi_epi16(offset_vec_i16_1, vec0_i16);
        __m128i offset_vec_i32_3 = _mm_unpacklo_epi16(offset_vec_i16_2, vec0_i16);
        __m128i offset_vec_i32_4 = _mm_unpackhi_epi16(offset_vec_i16_2, vec0_i16);

        __m256i load_vec_i64_1 = _mm256_i32gather_epi64((long long const *)words, offset_vec_i32_1, 8); // 4 word
        __m256i load_vec_i64_2 = _mm256_i32gather_epi64((long long const *)words, offset_vec_i32_2, 8);
        __m256i load_vec_i64_3 = _mm256_i32gather_epi64((long long const *)words, offset_vec_i32_3, 8);
        __m256i load_vec_i64_4 = _mm256_i32gather_epi64((long long const *)words, offset_vec_i32_4, 8);

        __m128i index_vec_i32_1 = _mm_unpacklo_epi16(index_vec_i16_1, vec0_i16);
        __m128i index_vec_i32_2 = _mm_unpackhi_epi16(index_vec_i16_1, vec0_i16);
        __m128i index_vec_i32_3 = _mm_unpacklo_epi16(index_vec_i16_2, vec0_i16);
        __m128i index_vec_i32_4 = _mm_unpackhi_epi16(index_vec_i16_2, vec0_i16);

        __m256i index_uint64_vec_i64_1 = _mm256_i32gather_epi64((long long const *)uint64_table, index_vec_i32_1, 8);
        __m256i index_uint64_vec_i64_2 = _mm256_i32gather_epi64((long long const *)uint64_table, index_vec_i32_2, 8);
        __m256i index_uint64_vec_i64_3 = _mm256_i32gather_epi64((long long const *)uint64_table, index_vec_i32_3, 8);
        __m256i index_uint64_vec_i64_4 = _mm256_i32gather_epi64((long long const *)uint64_table, index_vec_i32_4, 8);

        _mm256_store_si256((__m256i *)tmp, index_uint64_vec_i64_1);
        _mm256_store_si256((__m256i *)(tmp + 4), index_uint64_vec_i64_2);
        _mm256_store_si256((__m256i *)(tmp + 8), index_uint64_vec_i64_3);
        _mm256_store_si256((__m256i *)(tmp + 12), index_uint64_vec_i64_4);
        _mm_storeu_si128((__m128i *)offsets, offset_vec_i16_1);
        _mm_storeu_si128((__m128i *)(offsets + 8), offset_vec_i16_2);

        for (uint64_t j = 0; j < inner_loop; ++j) {
            word = &words[offsets[j]];
            *word = *word | tmp[j];
        }
    }

    if (i < length) {
        _scalar_bitset_set_list(words, list + i, length - i);
    }
}

static inline void _asm_bitset_set_list(uint64_t *words, const uint16_t *list, uint64_t length) {
    uint64_t pos;
    const uint16_t *end = list + length;

    uint64_t shift = 6;
    uint64_t offset;
    uint64_t load;
    for (; list + 3 < end; list += 4) {
        pos = list[0];
        __asm volatile(
            "shrx %[shift], %[pos], %[offset]\n"
            "mov (%[words],%[offset],8), %[load]\n"
            "bts %[pos], %[load]\n"
            "mov %[load], (%[words],%[offset],8)"
            : [load] "=&r"(load), [offset] "=&r"(offset)
            : [words] "r"(words), [shift] "r"(shift), [pos] "r"(pos));
        pos = list[1];
        __asm volatile(
            "shrx %[shift], %[pos], %[offset]\n"
            "mov (%[words],%[offset],8), %[load]\n"
            "bts %[pos], %[load]\n"
            "mov %[load], (%[words],%[offset],8)"
            : [load] "=&r"(load), [offset] "=&r"(offset)
            : [words] "r"(words), [shift] "r"(shift), [pos] "r"(pos));
        pos = list[2];
        __asm volatile(
            "shrx %[shift], %[pos], %[offset]\n"
            "mov (%[words],%[offset],8), %[load]\n"
            "bts %[pos], %[load]\n"
            "mov %[load], (%[words],%[offset],8)"
            : [load] "=&r"(load), [offset] "=&r"(offset)
            : [words] "r"(words), [shift] "r"(shift), [pos] "r"(pos));
        pos = list[3];
        __asm volatile(
            "shrx %[shift], %[pos], %[offset]\n"
            "mov (%[words],%[offset],8), %[load]\n"
            "bts %[pos], %[load]\n"
            "mov %[load], (%[words],%[offset],8)"
            : [load] "=&r"(load), [offset] "=&r"(offset)
            : [words] "r"(words), [shift] "r"(shift), [pos] "r"(pos));
    }

    while (list != end) {
        pos = list[0];
        __asm volatile(
            "shrx %[shift], %[pos], %[offset]\n"
            "mov (%[words],%[offset],8), %[load]\n"
            "bts %[pos], %[load]\n"
            "mov %[load], (%[words],%[offset],8)"
            : [load] "=&r"(load), [offset] "=&r"(offset)
            : [words] "r"(words), [shift] "r"(shift), [pos] "r"(pos));
        list++;
    }
}

int main_1() {
    // int words_size = roaring::internal::BITSET_CONTAINER_SIZE_IN_WORDS; // 1024
    int words_size = 1024;
    uint64_t words[words_size];
    for (uint64_t i = 0; i < words_size; ++i) {
        words[i] = (uint64_t)(i + 1);
    }

    const size_t list_size = 65536;
    uint16_t list[list_size];
    // uint16_t list[list_size] = {2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 65u};
    for (uint16_t i = 0; i <= list_size - 2; ++i) {
        list[i] = i;
    }

    std::cout << "begin bench..." << std::endl;

    double d = 1.0;
    ankerl::nanobench::Bench().run("some double ops", [&] {
        for (int i = 0; i < 1000; ++i) {
            _scalar_bitset_set_list(words, list, list_size);
        }
        ankerl::nanobench::doNotOptimizeAway(d);
    });

    return 0;
}

int main_0() {
    // int words_size = roaring::internal::BITSET_CONTAINER_SIZE_IN_WORDS; // 1024
    int words_size = 1024;
    uint64_t words[words_size];
    for (uint64_t i = 0; i < words_size; ++i) {
        words[i] = (uint64_t)(i + 1);
    }

    const size_t list_size = 65536;
    uint16_t list[list_size];
    // uint16_t list[list_size] = {2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 65u};
    for (uint16_t i = 0; i <= list_size - 2; ++i) {
        list[i] = i;
    }

    std::cout << "begin bench..." << std::endl;

    auto start1 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _scalar_bitset_set_list(words, list, list_size);
    }
    auto end1 = system_clock::now();
    auto duration1 = duration_cast<milliseconds>(end1 - start1);
    std::cout << "scalar cost: " << double(duration1.count()) * milliseconds::period::num << " ms" << std::endl;


    auto start2 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _avx2_bitset_set_list(words, list, list_size);
    }
    auto end2 = system_clock::now();
    auto duration2 = duration_cast<milliseconds>(end2 - start2);
    std::cout << "avx2 cost: " << double(duration2.count()) * milliseconds::period::num << " ms" << std::endl;

    auto start22 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _avx2_bitset_set_list(words, list, list_size);
    }
    auto end22 = system_clock::now();
    auto duration22 = duration_cast<milliseconds>(end22 - start22);
    std::cout << "avx2 2 cost: " << double(duration22.count()) * milliseconds::period::num << " ms" << std::endl;

    auto start3 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _asm_bitset_set_list(words, list, list_size);
    }
    auto end3 = system_clock::now();
    auto duration3 = duration_cast<milliseconds>(end3 - start3);
    std::cout << "asm cost: " << double(duration3.count()) * milliseconds::period::num << " ms" << std::endl;

    auto start4 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _scalar_bitset_set_list_withcard(words, 1024, list, list_size);
    }
    auto end4 = system_clock::now();
    auto duration4 = duration_cast<milliseconds>(end4 - start4);
    std::cout << "scalar with card cost: " << double(duration4.count()) * milliseconds::period::num << " ms" << std::endl;

    auto start5 = system_clock::now();
    for (int i = 0; i < 100000; ++i) {
        _asm_bitset_set_list_withcard(words, 1024, list, list_size);
    }
    auto end5 = system_clock::now();
    auto duration5 = duration_cast<milliseconds>(end5 - start5);
    std::cout << "asm with card cost: " << double(duration5.count()) * milliseconds::period::num << " ms" << std::endl;

    std::cout << "words: ";
    for (int i = 0; i < words_size; ++i) {
        std::cout << words[i] << ", ";
    }
    std::cout << std::endl;

    return 0;
}

/// g++ --std=c++11 -O3 -march=native zeno_dev_array_bitset.cpp