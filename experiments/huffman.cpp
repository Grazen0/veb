#include "numeric_queue.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace
{
    struct Node {
        std::uint64_t val;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    void dfs(const Node& node, std::size_t depth, std::uint64_t& ans)
    {
        if (!node.left) {
            ans += node.val * depth;
            return;
        }

        dfs(*node.left, depth + 1, ans);
        dfs(*node.right, depth + 1, ans);
    }

    template<template<std::size_t, typename> typename Queue>
    using HuffmanQueue = Queue<64, std::unique_ptr<Node>>;

    template<template<std::size_t W, typename T> typename Queue>
        requires(numeric_queue<HuffmanQueue<Queue>>)
    std::uint64_t huffman(const std::vector<std::uint64_t>& freqs)
    {
        HuffmanQueue<Queue> q;

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
} // namespace

int main()
{
    std::size_t t = 0;
    std::cin >> t;

    while (t-- != 0) {
        std::size_t n = 0;
        std::cin >> n;
        std::vector<std::uint64_t> freqs(n);

        for (std::size_t i = 0; i < n; ++i)
            std::cin >> freqs[i];

        std::cout << huffman<VebQueue>(freqs) << "\n";
    }

    return 0;
}
