#include "algorithm.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    prim::graph read_input(const std::filesystem::path& path)
    {
        std::ifstream in{path};
        std::size_t n = 0;
        std::size_t m = 0;
        in >> n >> m;

        prim::graph graph(n);
        for (std::size_t i = 0; i < m; ++i) {
            std::size_t a = 0;
            std::size_t b = 0;
            std::uint32_t w = 0;
            in >> a >> b >> w;
            --a;
            --b;
            graph[a].emplace_back(b, w);
            graph[b].emplace_back(a, w);
        }
        return graph;
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

TEST_CASE("Prim benchmarks", "[benchmark]")
{
    for (const auto& path : inputs()) {
        DYNAMIC_SECTION(path.filename().string())
        {
            auto graph = read_input(path);

            BENCHMARK("Veb")
            {
                return prim::prim<VebQueue>(graph);
            };
            BENCHMARK("std::priority_queue")
            {
                return prim::prim<PriorityQueueQueue>(graph);
            };
            BENCHMARK("std::multiset")
            {
                return prim::prim<MultisetQueue>(graph);
            };
            BENCHMARK("boost::heap::binomial_heap")
            {
                return prim::prim<BinomialHeapQueue>(graph);
            };
            BENCHMARK("boost::heap::fibonacci_heap")
            {
                return prim::prim<FibonacciHeapQueue>(graph);
            };
        }
    }
}
