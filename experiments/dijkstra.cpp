#include "numeric_queue.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
#include <veb.hpp>
#include <vector>

namespace
{
    constexpr std::uint64_t INF = 1e18;

    template<template<std::size_t W, typename T> typename Queue>
    using DijkstraQueue = Queue<64, std::size_t>;

    template<template<std::size_t W, typename T> typename Queue>
        requires(numeric_queue<DijkstraQueue<Queue>>)
    std::vector<std::size_t> dijkstra(
        const std::vector<std::vector<std::pair<std::size_t, std::uint64_t>>>&
            adj,
        std::size_t start, std::size_t end)
    {
        std::size_t n = adj.size();

        std::vector<std::uint64_t> dist(n, INF);
        std::vector<std::size_t> par(n, n);

        DijkstraQueue<Queue> q;

        dist[start] = 0;
        q.push(dist[start], start);

        while (!q.empty()) {
            auto [d_bits, i] = q.pop();
            std::uint64_t d = d_bits.to_ullong();

            if (d != dist[i])
                continue;

            for (auto [j, w] : adj[i]) {
                if (d + w < dist[j]) {
                    dist[j] = d + w;
                    par[j] = i;
                    q.push(dist[j], j);
                }
            }
        }

        if (dist[end] == INF)
            return {};

        std::size_t cur = n - 1;
        std::vector<std::size_t> path;

        while (cur != n) {
            path.emplace_back(cur);
            cur = par[cur];
        }

        std::ranges::reverse(path);
        return path;
    }
} // namespace

int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    std::vector<std::vector<std::pair<std::size_t, std::uint64_t>>> adj(n);

    for (std::size_t i = 0; i < m; ++i) {
        std::size_t a = 0;
        std::size_t b = 0;
        std::uint64_t w = 0;
        std::cin >> a >> b >> w;
        --a;
        --b;
        adj[a].emplace_back(b, w);
        adj[b].emplace_back(a, w);
    }

    auto path = dijkstra<VebQueue>(adj, 0, n - 1);

    if (path.empty()) {
        std::cout << -1 << "\n";
    } else {
        for (std::size_t x : path)
            std::cout << x + 1 << " ";
        std::cout << "\n";
    }

    return 0;
}
