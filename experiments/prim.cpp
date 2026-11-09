#include "numeric_queue.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>
#include <veb.hpp>
#include <vector>

namespace
{
    template<template<std::size_t W, typename T> typename Queue>
    using PrimQueue = Queue<32, std::size_t>;

    template<template<std::size_t W, typename T> typename Queue>
        requires(numeric_queue<PrimQueue<Queue>>)
    std::optional<std::uint64_t>
    prim(const std::vector<std::vector<std::pair<std::size_t, std::uint32_t>>>&
             adj)
    {
        std::size_t n = adj.size();
        std::vector<bool> vis(n);
        PrimQueue<Queue> q;

        std::uint64_t cost = 0;
        q.push(0, 0);

        while (!q.empty()) {
            auto [w_bits, i] = q.pop();
            if (vis[i])
                continue;

            vis[i] = true;
            cost += w_bits.to_ulong();

            for (auto [j, w] : adj[i])
                q.push(w, j);
        }

        for (std::size_t i = 0; i < n; ++i) {
            if (!vis[i])
                return std::nullopt;
        }

        return cost;
    }
} // namespace

int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    std::vector<std::vector<std::pair<std::size_t, std::uint32_t>>> adj(n);

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

    auto ans = prim<VebQueue>(adj);
    if (!ans)
        std::cout << "IMPOSSIBLE\n";
    else
        std::cout << *ans << "\n";

    return 0;
}
