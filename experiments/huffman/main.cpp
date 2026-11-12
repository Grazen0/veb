#include "algorithm.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <print>

namespace
{
    std::uint64_t dfs(const std::unique_ptr<huffman::Node>& node)
    {
        if (!node)
            return 0;

        return node->val + dfs(node->left) + dfs(node->right);
    }
} // namespace

// https://cses.fi/problemset/task/1161
int main()
{
    std::size_t x = 0;
    std::size_t n = 0;
    std::cin >> x >> n;

    std::vector<std::uint64_t> freqs(n);
    for (auto& f : freqs)
        std::cin >> f;

    auto tree = huffman::huffman<VebQueue>(freqs);
    std::uint64_t ans = dfs(tree) - x;
    std::println("{}", ans);
    return 0;
}
