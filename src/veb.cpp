#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <ios>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

namespace
{
    constexpr bool is_pow2(auto n)
    {
        return n != 0 && (n & (n - 1)) == 0;
    }

    // PERF: probably needs optimization
    template<std::size_t W>
        requires(W % 2 == 0)
    constexpr std::pair<std::bitset<W / 2>, std::bitset<W / 2>>
    split(const std::bitset<W>& n)
    {
        std::bitset<W / 2> hi;
        std::bitset<W / 2> lo;

        for (std::size_t i = 0; i < W / 2; ++i)
            lo[i] = n[i];

        for (std::size_t i = 0; i < W / 2; ++i)
            hi[i] = n[(W / 2) + i];

        return {hi, lo};
    }

    template<std::size_t W>
    constexpr std::bitset<2 * W> join(const std::bitset<W>& hi,
                                      const std::bitset<W>& lo)
    {
        std::bitset<2 * W> out;

        for (std::size_t i = 0; i < W; ++i)
            out[i] = lo[i];

        for (std::size_t i = 0; i < W; ++i)
            out[W + i] = hi[i];

        return out;
    }
} // namespace

template<std::size_t W>
    requires(is_pow2(W))
class Veb {
private:
    Veb<W / 2> m_summary{};
    std::array<Veb<W / 2>, 1 << (W / 2)> m_clusters{};
    std::optional<std::bitset<W>> m_min;
    std::optional<std::bitset<W>> m_max;

    constexpr void empty_tree_insert(const std::bitset<W>& x)
    {
        assert(!m_min);
        assert(!m_max);
        m_min = x;
        m_max = x;
    }

    friend class Veb<2 * W>;

public:
    Veb() = default;

    [[nodiscard]] constexpr std::optional<std::bitset<W>> minimum() const
    {
        return m_min;
    }

    [[nodiscard]] constexpr std::optional<std::bitset<W>> maximum() const
    {
        return m_max;
    }

    [[nodiscard]] bool member(const std::bitset<W>& x) const
    {
        if (x == m_min || x == m_max)
            return true;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        return hi_cluster.member(lo);
    }

    [[nodiscard]] std::optional<std::bitset<W>>
    successor(const std::bitset<W>& x) const
    {
        if (m_min && x.to_ullong() < m_min->to_ullong())
            return m_min;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        auto max_low = hi_cluster.maximum();

        if (max_low && lo.to_ullong() < max_low->to_ullong()) {
            auto offset = hi_cluster.successor(lo);
            return join(hi, *offset);
        }

        auto succ_cluster = m_summary.successor(hi);
        if (!succ_cluster)
            return std::nullopt;

        auto offset = m_clusters.at(succ_cluster->to_ullong()).minimum();
        return join(*succ_cluster, *offset);
    }

    [[nodiscard]] std::optional<std::bitset<W>>
    predecessor(const std::bitset<W>& x) const
    {
        if (m_max && x.to_ullong() > m_max->to_ullong())
            return m_max;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        auto min_low = hi_cluster.minimum();

        if (min_low && lo.to_ullong() > min_low->to_ullong()) {
            auto offset = hi_cluster.predecessor(lo);
            return join(hi, *offset);
        }

        auto pred_cluster = m_summary.predecessor(hi);
        if (!pred_cluster) {
            if (m_min && x.to_ullong() > m_min->to_ullong())
                return m_min;

            return std::nullopt;
        }

        auto offset = m_clusters.at(pred_cluster->to_ullong()).maximum();
        return join(*pred_cluster, *offset);
    }

    void insert(std::bitset<W> x)
    {
        if (!m_min) {
            empty_tree_insert(x);
            return;
        }

        if (x.to_ullong() < m_min->to_ullong())
            std::swap(x, *m_min);

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());

        if (!hi_cluster.minimum()) {
            m_summary.insert(hi);
            hi_cluster.empty_tree_insert(lo);
        } else {
            hi_cluster.insert(lo);
        }

        if (x.to_ullong() > m_max->to_ullong())
            m_max = x;
    }

    void remove(std::bitset<W> x)
    {
        if (m_min == m_max) {
            m_min = std::nullopt;
            m_max = std::nullopt;
            return;
        }

        if (x == m_min) {
            auto first_cluster_idx = m_summary.minimum();
            auto& first_cluster = m_clusters.at(first_cluster_idx->to_ullong());
            x = join(*first_cluster_idx, *first_cluster.minimum());
            m_min = x;
        }

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        hi_cluster.remove(lo);

        if (!hi_cluster.minimum()) {
            m_summary.remove(hi);
            if (x == m_max) {
                auto summary_max = m_summary.maximum();
                if (!summary_max) {
                    m_max = m_min;
                } else {
                    auto& smax_cluster =
                        m_clusters.at(summary_max->to_ullong());
                    m_max = join(*summary_max, *smax_cluster.maximum());
                }
            }
        } else if (x == m_max) {
            m_max = join(hi, *hi_cluster.maximum());
        }
    }
};

template<>
class Veb<1> {
private:
    std::bitset<2> m_bits;
    std::optional<std::bitset<1>> m_min;
    std::optional<std::bitset<1>> m_max;

    constexpr void empty_tree_insert(const std::bitset<1>& x)
    {
        assert(!m_min);
        assert(!m_max);
        m_min = x;
        m_max = x;
    }

    friend class Veb<2>;

public:
    Veb() = default;

    [[nodiscard]] constexpr std::optional<std::bitset<1>> minimum() const
    {
        return m_min;
    }

    [[nodiscard]] constexpr std::optional<std::bitset<1>> maximum() const
    {
        return m_max;
    }

    [[nodiscard]] constexpr bool member(const std::bitset<1>& x) const
    {
        return x == m_min || x == m_max;
    }

    [[nodiscard]] std::optional<std::bitset<1>>
    successor(const std::bitset<1>& x) const
    {
        if (x == 0 && m_max == 1)
            return 1;

        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::bitset<1>>
    predecessor(const std::bitset<1>& x) const
    {
        if (x == 1 && m_min == 0)
            return 0;

        return std::nullopt;
    }

    void insert(std::bitset<1> x)
    {
        if (!m_min) {
            empty_tree_insert(x);
            return;
        }

        if (x.to_ullong() < m_min->to_ullong())
            std::swap(x, *m_min);

        if (x.to_ullong() > m_max->to_ullong())
            m_max = x;
    }

    void remove(std::bitset<1> x)
    {
        if (m_min == m_max) {
            m_min = std::nullopt;
            m_max = std::nullopt;
            return;
        }

        m_min = (x == 0);
        m_max = m_min;
    }
};

int main()
{
    auto veb = std::make_unique<Veb<8>>();

    veb->insert(43);
    veb->insert(1);
    veb->insert(6);

    std::cout << std::boolalpha;
    std::cout << veb->member(1) << "\n";
    std::cout << veb->member(6) << "\n";
    std::cout << veb->member(42) << "\n";
    std::cout << veb->member(3) << "\n";
    std::cout << veb->member(0) << "\n";
    std::cout << veb->predecessor(7)->to_ullong() << "\n";
    std::cout << "\n";

    veb->remove(1);
    veb->remove(42);

    std::cout << veb->member(1) << "\n";
    std::cout << veb->member(6) << "\n";
    std::cout << veb->member(42) << "\n";
    std::cout << veb->member(3) << "\n";
    std::cout << veb->member(0) << "\n";

    return 0;
}
