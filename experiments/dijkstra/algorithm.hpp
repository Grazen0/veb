#pragma once
#include "common/numeric_queue.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace dijkstra
{
    using graph =
        std::vector<std::vector<std::pair<std::size_t, std::uint64_t>>>;

    constexpr std::uint64_t INF = 1e18;

    template<template<std::size_t W, typename T> typename Queue>
    using dijkstra_queue = Queue<64, std::size_t>;

    template<template<std::size_t W, typename T> typename Queue>
        requires(numeric_queue<dijkstra_queue<Queue>>)
    std::vector<std::uint64_t> dijkstra(const graph& adj, std::size_t start)
    {
        std::vector<std::uint64_t> dist(adj.size(), INF);
        dijkstra_queue<Queue> q;

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
                    q.push(dist[j], j);
                }
            }
        }

        return dist;
    }
} // namespace dijkstra
