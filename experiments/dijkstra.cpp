#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
#include <veb.hpp>
#include <vector>

static constexpr std::uint64_t INF = 1e18;

int main()
{
    std::size_t n = 0;
    std::size_t m = 0;
    std::cin >> n >> m;

    std::vector<std::vector<std::pair<std::size_t, std::uint64_t>>> adj(n);
    std::vector<std::uint64_t> dist(n, INF);
    std::vector<std::size_t> par(n, n);

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

    Veb<64, std::vector<std::size_t>> veb;

    dist[0] = 0;
    veb[dist[0]].emplace_back(0);

    while (!veb.empty()) {
        auto [d_bits, vs] = *veb.minimum();
        assert(!vs->empty());

        std::size_t i = vs->back();
        vs->pop_back();
        if (vs->empty())
            veb.erase(d_bits);

        std::uint64_t d = d_bits.to_ullong();

        if (d != dist[i])
            continue;

        for (auto [j, w] : adj[i]) {
            if (d + w < dist[j]) {
                dist[j] = d + w;
                par[j] = i;
                veb[dist[j]].emplace_back(j);
            }
        }
    }

    if (dist[n - 1] == INF) {
        std::cout << -1 << "\n";
        return 0;
    }

    std::size_t cur = n - 1;
    std::vector<std::size_t> path;

    while (cur != n) {
        path.emplace_back(cur);
        cur = par[cur];
    }

    std::ranges::reverse(path);

    for (std::size_t x : path)
        std::cout << x + 1 << " ";

    std::cout << "\n";
    return 0;
}
