#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <veb.hpp>
#include <vector>

int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    std::vector<std::vector<std::pair<std::size_t, std::uint32_t>>> adj(n);
    std::vector<bool> vis(n);

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

    Veb<32, std::vector<std::size_t>> veb;

    std::uint64_t ans = 0;
    veb[0].emplace_back(0);

    while (!veb.empty()) {
        auto [w_bits, vs] = *veb.minimum();
        assert(!vs->empty());

        std::size_t i = vs->back();
        vs->pop_back();
        if (vs->empty())
            veb.erase(w_bits);

        if (vis[i])
            continue;

        std::uint32_t w = w_bits.to_ullong();
        vis[i] = true;
        ans += w;

        for (auto [j, w] : adj[i])
            veb[w].emplace_back(j);
    }

    for (std::size_t i = 0; i < n; ++i) {
        if (!vis[i]) {
            std::cout << "IMPOSSIBLE\n";
            return 0;
        }
    }

    std::cout << ans << "\n";
    return 0;
}
