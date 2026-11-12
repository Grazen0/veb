#include "algorithm.hpp"
#include <iostream>
#include <print>

// https://cses.fi/problemset/task/1671
int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    dijkstra::graph adj(n);

    for (std::size_t i = 0; i < m; ++i) {
        std::size_t a = 0;
        std::size_t b = 0;
        std::uint64_t w = 0;
        std::cin >> a >> b >> w;
        --a;
        --b;
        adj[a].emplace_back(b, w);
    }

    auto dist = dijkstra::dijkstra<VebQueue>(adj, 0);

    for (auto d : dist)
        std::print("{} ", d);
    std::println();

    return 0;
}
