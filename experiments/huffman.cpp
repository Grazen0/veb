#include "veb.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

struct Node {
    std::uint64_t val;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    explicit Node(std::uint64_t val)
        : val{val}
    {
    }

    Node(std::uint64_t val, std::unique_ptr<Node> left,
         std::unique_ptr<Node> right)
        : val{val},
          left{std::move(left)},
          right{std::move(right)}
    {
    }

    bool operator<(const Node& o) const
    {
        return val > o.val;
    }
};

namespace
{
    void dfs(const Node& node, std::size_t depth, std::uint64_t& ans)
    {
        if (!node.left) {
            ans += node.val * depth;
            return;
        }

        dfs(*node.left, depth + 1, ans);
        dfs(*node.right, depth + 1, ans);
    }
} // namespace

int main()
{
    std::size_t t = 0;
    std::cin >> t;

    while (t-- != 0) {
        std::size_t n = 0;
        std::cin >> n;

        Veb<64, std::vector<std::unique_ptr<Node>>> veb;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint64_t f = 0;
            std::cin >> f;
            veb[f].emplace_back(std::make_unique<Node>(f));
        }

        for (std::size_t i = 0; i < n - 1; ++i) {
            auto [kl, vl] = *veb.minimum();
            assert(!vl->empty());
            auto left = std::move(vl->back());
            vl->pop_back();
            if (vl->empty())
                veb.erase(kl);

            auto [kr, vr] = *veb.minimum();
            assert(!vr->empty());
            auto right = std::move(vr->back());
            vr->pop_back();
            if (vr->empty())
                veb.erase(kr);

            auto comb = std::make_unique<Node>(
                left->val + right->val, std::move(left), std::move(right));
            veb[comb->val].emplace_back(std::move(comb));
        }

        assert(!veb.empty());
        auto [k, vs] = *veb.minimum();
        auto& root = vs->front();

        std::uint64_t ans = 0;
        dfs(*root, 1, ans);
        std::cout << ans << "\n";
    }

    return 0;
}
