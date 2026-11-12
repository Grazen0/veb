#include "algorithm.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    using Cases = std::vector<std::vector<std::uint64_t>>;

    huffman::frequency_map read_input(const std::filesystem::path& path)
    {
        std::ifstream in(path);
        std::size_t n = 0;
        in >> n;

        huffman::frequency_map freqs(n);
        for (auto& f : freqs)
            in >> f;

        return freqs;
    }

    std::vector<std::filesystem::path> inputs()
    {
        std::vector<std::filesystem::path> paths;
        std::filesystem::path directory{EXPERIMENT_INPUT_DIR};

        for (const auto& entry :
             std::filesystem::directory_iterator{directory}) {
            if (entry.path().extension() == ".in")
                paths.push_back(entry.path());
        }

        std::ranges::sort(paths);
        return paths;
    }
} // namespace

TEST_CASE("Huffman benchmarks", "[benchmark]")
{
    for (const auto& path : inputs()) {
        DYNAMIC_SECTION(path.filename().string())
        {
            auto freqs = read_input(path);

            BENCHMARK("Veb")
            {
                return huffman::huffman<VebQueue>(freqs);
            };
            BENCHMARK("std::priority_queue")
            {
                return huffman::huffman<PriorityQueueQueue>(freqs);
            };
            BENCHMARK("std::multiset")
            {
                return huffman::huffman<MultisetQueue>(freqs);
            };
            BENCHMARK("boost::heap::binomial_heap")
            {
                return huffman::huffman<BinomialHeapQueue>(freqs);
            };
            BENCHMARK("boost::heap::fibonacci_heap")
            {
                return huffman::huffman<FibonacciHeapQueue>(freqs);
            };
        }
    }
}
