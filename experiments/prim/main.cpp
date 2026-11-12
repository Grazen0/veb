#include "algorithm.hpp"
#include <iostream>
#include <print>

// https://cses.fi/problemset/task/1675
int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    prim::graph adj(n);

    for (std::size_t i = 0; i < m; ++i) {
        std::size_t a = 0;
        std::size_t b = 0;
        std::uint32_t w = 0;
        std::cin >> a >> b >> w;
        --a;
        --b;

        adj[a].emplace_back(b, w);
        adj[b].emplace_back(a, w);
    }

    auto result = prim::prim<VebQueue>(adj);
    if (result)
        std::println("{}", *result);
    else
        std::println("IMPOSSIBLE");

    return 0;
}
