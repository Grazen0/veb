#include "algorithm.hpp"
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    dijkstra::graph read_input(const std::filesystem::path& path)
    {
        std::ifstream in{path};
        std::size_t n = 0;
        std::size_t m = 0;
        in >> n >> m;

        dijkstra::graph adj(n);

        for (std::size_t i = 0; i < m; ++i) {
            std::size_t a = 0;
            std::size_t b = 0;
            std::uint64_t w = 0;
            in >> a >> b >> w;
            --a;
            --b;
            adj[a].emplace_back(b, w);
            adj[b].emplace_back(a, w);
        }

        return adj;
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

    template<template<std::size_t, typename> typename Queue>
    std::vector<std::size_t> run(const dijkstra::graph& adj)
    {
        return dijkstra::dijkstra<Queue>(adj, 0, adj.size() - 1);
    }
} // namespace

TEST_CASE("Dijkstra benchmarks", "[benchmark]")
{
    for (const auto& path : inputs()) {
        DYNAMIC_SECTION(path.filename().string())
        {
            auto graph = read_input(path);

            BENCHMARK("Veb")
            {
                return run<VebQueue>(graph);
            };
            BENCHMARK("std::priority_queue")
            {
                return run<PriorityQueueQueue>(graph);
            };
            BENCHMARK("std::multiset")
            {
                return run<MultisetQueue>(graph);
            };
            BENCHMARK("boost::heap::binomial_heap")
            {
                return run<BinomialHeapQueue>(graph);
            };
            BENCHMARK("boost::heap::fibonacci_heap")
            {
                return run<FibonacciHeapQueue>(graph);
            };
        }
    }
}
