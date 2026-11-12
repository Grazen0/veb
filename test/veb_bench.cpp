#include "veb.hpp"
#include <algorithm>
#include <cassert>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <format>
#include <random>
#include <string>
#include <vector>

namespace
{
    constexpr std::size_t Q = 10'000;

    template<std::size_t W>
    least_uint<W> random_key(std::mt19937_64& rng)
    {
        std::uint64_t r = rng();
        if constexpr (W < 64)
            r &= (std::uint64_t{1} << W) - 1;
        return r;
    }

    template<std::size_t W>
    std::vector<least_uint<W>> random_keys(std::size_t n, std::uint64_t seed)
    {
        std::mt19937_64 rng{seed};
        std::vector<least_uint<W>> keys(n);
        for (auto& k : keys)
            k = random_key<W>(rng);
        return keys;
    }

    template<std::size_t W>
    std::vector<least_uint<W>> distinct_keys(std::size_t n, std::uint64_t seed)
    {
        std::mt19937_64 rng{seed};
        std::vector<least_uint<W>> keys;
        while (keys.size() < n) {
            for (std::size_t i = keys.size(); i < n; ++i)
                keys.emplace_back(random_key<W>(rng));

            std::ranges::sort(keys);
            keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
        }
        std::ranges::shuffle(keys, rng);
        return keys;
    }
} // namespace

TEMPLATE_TEST_CASE_SIG("Veb scaling", "[bench]", ((std::size_t W), W), 16, 17,
                       32, 33, 64)
{
    auto rand_keys = random_keys<W>(Q, 42);

    for (std::size_t n : {100, 316, 1'000, 3'162, 10'000}) {
        auto keys = distinct_keys<W>(n, 67);
        std::string tag = std::format(" [W={}] [N={}]", W, n);

        Veb<W, int> veb;
        for (auto k : keys)
            veb.insert(k, 0);

        BENCHMARK("count (hit)" + tag)
        {
            std::size_t c = 0;
            for (std::size_t i = 0; i < Q; ++i)
                c += veb.count(keys[i % n]);
            return c;
        };

        BENCHMARK("count (random probe)" + tag)
        {
            std::size_t c = 0;
            for (auto k : rand_keys)
                c += veb.count(k);
            return c;
        };

        BENCHMARK("successor" + tag)
        {
            std::size_t c = 0;
            for (auto k : rand_keys)
                c += veb.successor(k).has_value();
            return c;
        };

        BENCHMARK("predecessor" + tag)
        {
            std::size_t c = 0;
            for (auto k : rand_keys)
                c += veb.predecessor(k).has_value();
            return c;
        };

        BENCHMARK("erase + reinsert" + tag)
        {
            std::size_t c = 0;
            for (std::size_t i = 0; i < Q; ++i) {
                auto k = keys[i % n];
                c += veb.erase(k);
                veb.insert(k, 0);
            }
            return c;
        };

        BENCHMARK_ADVANCED("build (n inserts)" +
                           tag)(Catch::Benchmark::Chronometer meter)
        {
            std::vector<Veb<W, int>> trees(meter.runs());
            meter.measure([&](int run) {
                for (auto k : keys)
                    trees[run].insert(k, 0);
                return trees[run].empty();
            });
        };
    }
}
