#pragma once
#include "common/numeric_queue.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace prim
{
    using graph =
        std::vector<std::vector<std::pair<std::size_t, std::uint32_t>>>;

    template<template<std::size_t W, typename T> typename Queue>
    using prim_queue = Queue<32, std::size_t>;

    template<template<std::size_t W, typename T> typename Queue>
        requires(numeric_queue<prim_queue<Queue>>)
    std::optional<std::uint64_t> prim(const graph& adj)
    {
        std::size_t n = adj.size();
        std::vector<bool> vis(n);
        prim_queue<Queue> q;

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

        if (std::ranges::any_of(vis, std::negate<bool>{}))
            return std::nullopt;

        return cost;
    }
} // namespace prim
