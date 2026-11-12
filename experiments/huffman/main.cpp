#include "algorithm.hpp"
#include <iostream>
#include <print>

int main()
{
    std::size_t tests = 0;
    std::cin >> tests;

    while (tests-- != 0) {
        std::size_t n = 0;
        std::cin >> n;

        std::vector<std::uint64_t> freqs(n);
        for (auto& f : freqs)
            std::cin >> f;

        auto ans = huffman::huffman<VebQueue>(freqs);
        std::println("{}", ans);
    }

    return 0;
}
