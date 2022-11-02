//
// Created by Zeno Yang on 2022/10/28.
//

#include <iostream>
#include "roaring.hh"
#include "roaring64map.hh"

void test_add_card() {
    roaring::Roaring r1;
    r1.add(1);
    r1.add(10);
    r1.add(100);
    r1.add(1000);
    r1.add(10000);
    for (uint32_t i = 65536; i < 65536*2; i+=2) {
        r1.add(i);
    }
    r1.addRange(65536L*3, 65536L*4);
    // roaring_bitmap.runOptimize();
    std::cout << "cardinality = " << r1.cardinality() << std::endl;
}

void print_roaring64_info(roaring::Roaring64Map& r) {
    std::cout << "roaring card: " << r.cardinality() << " cow: " << r.getCopyOnWrite() << " min: " << r.minimum() << " max: " << r.maximum() << " all: " << r.toString() << std::endl;
}

void test_array_bitset_container_union() {
   std::cout << "### test_array_bitset_container_union ###" << std::endl;

   // gen array container roaring
   roaring::Roaring64Map r_array;
   for (uint64_t i = 0; i < 10; ++i) {
       r_array.add(i);
   }
   print_roaring64_info(r_array);

   roaring::Roaring64Map r_bitset;
   r_bitset.addRange(0, 4097);
   r_bitset.removeRunCompression();
   print_roaring64_info(r_bitset);

   /// union with card
   // auto r_union = r_array | r_bitset;
   /// union without card

   // r_array.fastunion(1, r_bitset);
   print_roaring64_info(r_union);

}

int main_01() {
    test_array_bitset_container_union();

    return 0;
}