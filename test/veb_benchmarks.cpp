#include "veb.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>

namespace
{
    template<std::size_t W>
    void benchmark_veb()
    {
        BENCHMARK("Operations with W = " + std::to_string(W))
        {
            constexpr int N = 10'000;

            Veb<W, int> veb;
            for (int i = 0; i < N; ++i)
                veb.insert(i, i);

            std::size_t c = 0;
            for (int i = 0; i < N; ++i)
                c += veb.count(i);

            for (int i = 0; i < N; i += 3) {
                veb.erase(i);
            }

            for (int i = 0; i < N; ++i)
                c += veb.count(i);

            return c;
        };
    }
} // namespace

TEST_CASE("Veb")
{
    benchmark_veb<16>();
    benchmark_veb<32>();
    benchmark_veb<64>();
}
