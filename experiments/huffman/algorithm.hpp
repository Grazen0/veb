#pragma once
#include "common/numeric_queue.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace huffman
{
    using frequency_map = std::vector<std::uint64_t>;

    struct Node {
        std::uint64_t val;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    template<template<std::size_t W, typename T> typename Queue>
    using huffman_queue = Queue<64, std::unique_ptr<Node>>;

    inline void dfs(const Node& node, std::size_t depth, std::uint64_t& ans)
    {
        if (!node.left) {
            ans += node.val * depth;
            return;
        }

        dfs(*node.left, depth + 1, ans);
        dfs(*node.right, depth + 1, ans);
    }

    template<template<std::size_t, typename> typename Queue>
        requires(numeric_queue<huffman_queue<Queue>>)
    std::uint64_t huffman(const frequency_map& freqs)
    {
        huffman_queue<Queue> q;

        for (auto f : freqs)
            q.push(f, std::make_unique<Node>(f));

        for (std::size_t i = 0; i < freqs.size() - 1; ++i) {
            auto left = q.pop().second;
            auto right = q.pop().second;
            std::uint64_t sum = left->val + right->val;
            q.push(sum, std::make_unique<Node>(sum, std::move(left),
                                               std::move(right)));
        }

        assert(!q.empty());
        auto root = q.pop().second;

        std::uint64_t ans = 0;
        dfs(*root, 1, ans);
        return ans;
    }
} // namespace huffman
