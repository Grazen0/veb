#ifndef VEB_VEB_HPP
#define VEB_VEB_HPP

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <utility>
#include <variant>

// required for to_ullong() not to explode
static constexpr std::size_t MAX_W = 64;

// TODO: refactor with modules or something
static constexpr bool is_pow2(auto n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

template<std::size_t W>
    requires(W <= MAX_W && W % 2 == 0)
static constexpr std::pair<std::bitset<W / 2>, std::bitset<W / 2>>
split(const std::bitset<W>& n)
{
    constexpr std::size_t H = W / 2;
    std::bitset<H> lo{n.to_ullong()};
    std::bitset<H> hi{n.to_ullong() >> H};
    return {hi, lo};
}

template<std::size_t W>
    requires(2 * W <= MAX_W)
static constexpr std::bitset<2 * W> join(const std::bitset<W>& hi,
                                         const std::bitset<W>& lo)
{
    return std::bitset<2 * W>{lo.to_ullong() | (hi.to_ullong() << W)};
}

template<std::size_t W, typename V>
    requires(1 <= W && W <= MAX_W && is_pow2(W))
class Veb {
private:
    using Key = std::bitset<W>;
    using Entry = std::pair<Key, V>;
    using EntryRef = std::pair<Key, V*>;
    using EntryRefConst = std::pair<Key, const V*>;

    Veb<W / 2, std::monostate> m_summary{};
    std::array<Veb<W / 2, V>, 1 << (W / 2)> m_clusters{};
    std::optional<Entry> m_min;
    std::optional<Key> m_max;

public:
    Veb() = default;

    [[nodiscard]] constexpr std::optional<Key> min_key() const
    {
        if (!m_min)
            return std::nullopt;

        return m_min->first;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> minimum() const
    {
        if (!m_min)
            return std::nullopt;

        return std::make_pair(m_min->first, &m_min->second);
    }

    [[nodiscard]] constexpr std::optional<EntryRef> minimum()
    {
        if (!m_min)
            return std::nullopt;

        return std::make_pair(m_min->first, &m_min->second);
    }

    [[nodiscard]] constexpr std::optional<Key> max_key() const
    {
        return m_max;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> maximum() const
    {
        if (!m_max)
            return std::nullopt;
        return std::make_pair(*m_max, get(*m_max));
    }

    [[nodiscard]] constexpr std::optional<EntryRef> maximum()
    {
        if (!m_max)
            return std::nullopt;
        return std::make_pair(*m_max, get(*m_max));
    }

    [[nodiscard]] const V* get(const Key& x) const
    {
        if (!m_min)
            return nullptr;

        if (x == m_min->first)
            return &m_min->second;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        return hi_cluster.get(lo);
    }

    [[nodiscard]] V* get(const Key& x)
    {
        if (!m_min)
            return nullptr;

        if (x == m_min->first)
            return &m_min->second;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        return hi_cluster.get(lo);
    }

    [[nodiscard]] bool member(const Key& x) const
    {
        if (!m_min)
            return false;

        if (x == m_min->first || x == m_max)
            return true;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        return hi_cluster.member(lo);
    }

    [[nodiscard]] std::optional<Key> successor(const Key& x) const
    {
        if (m_min && x.to_ullong() < m_min->first.to_ullong())
            return m_min->first;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        auto max_low = hi_cluster.max_key();

        if (max_low && lo.to_ullong() < max_low->to_ullong()) {
            auto offset = hi_cluster.successor(lo);
            return join(hi, *offset);
        }

        auto succ_cluster = m_summary.successor(hi);
        if (!succ_cluster)
            return std::nullopt;

        auto offset = m_clusters.at(succ_cluster->to_ullong()).min_key();
        return join(*succ_cluster, *offset);
    }

    [[nodiscard]] std::optional<Key> predecessor(const Key& x) const
    {
        if (m_max && x.to_ullong() > m_max->to_ullong())
            return m_max;

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());
        auto min_low = hi_cluster.min_key();

        if (min_low && lo.to_ullong() > min_low->to_ullong()) {
            auto offset = hi_cluster.predecessor(lo);
            return join(hi, *offset);
        }

        auto pred_cluster = m_summary.predecessor(hi);
        if (!pred_cluster) {
            if (m_min && x.to_ullong() > m_min->first.to_ullong())
                return m_min->first;

            return std::nullopt;
        }

        auto offset = m_clusters.at(pred_cluster->to_ullong()).max_key();
        return join(*pred_cluster, *offset);
    }

    void insert(Key x, V val)
    {
        if (!m_min) {
            assert(!m_max);
            m_min = {x, std::move(val)};
            m_max = x;
            return;
        }

        if (x == m_min->first) {
            m_min->second = std::move(val);
            return;
        }

        if (x.to_ullong() < m_min->first.to_ullong()) {
            std::swap(x, m_min->first);
            std::swap(val, m_min->second);
        }

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());

        if (!hi_cluster.min_key())
            m_summary.insert(hi, {});

        hi_cluster.insert(lo, std::move(val));

        if (x.to_ullong() > m_max->to_ullong())
            m_max = x;
    }

    bool remove(Key x)
    {
        if (!m_min)
            return false;

        if (m_min->first == m_max) {
            if (x != m_min->first)
                return false;

            m_min.reset();
            m_max.reset();
            return true;
        }

        bool should_remove = false;

        if (x == m_min->first) {
            auto first_cluster_idx = m_summary.min_key();
            assert(first_cluster_idx);

            auto& first_cluster = m_clusters.at(first_cluster_idx->to_ullong());
            auto replacement = first_cluster.minimum();
            assert(replacement);

            x = join(*first_cluster_idx, replacement->first);
            m_min = {x, std::move(*replacement->second)};
            should_remove = true;
        }

        auto [hi, lo] = split(x);
        auto& hi_cluster = m_clusters.at(hi.to_ullong());

        bool removed = hi_cluster.remove(lo);

        if (should_remove)
            assert(removed);
        else if (!removed)
            return false;

        if (!hi_cluster.min_key()) {
            m_summary.remove(hi);
            if (x == m_max) {
                auto summary_max = m_summary.max_key();
                if (!summary_max) {
                    m_max = m_min->first;
                } else {
                    auto& smax_cluster =
                        m_clusters.at(summary_max->to_ullong());
                    m_max = join(*summary_max, *smax_cluster.max_key());
                }
            }
        } else if (x == m_max) {
            m_max = join(hi, *hi_cluster.max_key());
        }

        return true;
    }
};

template<typename V>
class Veb<1, V> {
private:
    using Key = std::bitset<1>;
    using Entry = std::pair<Key, V>;
    using EntryRef = std::pair<Key, V*>;
    using EntryRefConst = std::pair<Key, const V*>;

    std::array<std::optional<V>, 2> m_vals;

public:
    Veb() = default;

    [[nodiscard]] constexpr std::optional<Key> min_key() const
    {
        if (m_vals[0])
            return 0;
        if (m_vals[1])
            return 1;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> minimum() const
    {
        auto k = min_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k, &*m_vals[k->to_ullong()]);
    }

    [[nodiscard]] constexpr std::optional<EntryRef> minimum()
    {
        auto k = min_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k, &*m_vals[k->to_ullong()]);
    }

    [[nodiscard]] constexpr std::optional<Key> max_key() const
    {
        if (m_vals[1])
            return 1;
        if (m_vals[0])
            return 0;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> maximum() const
    {
        auto k = max_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k, &*m_vals[k->to_ullong()]);
    }

    [[nodiscard]] constexpr std::optional<EntryRef> maximum()
    {
        auto k = max_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k, &*m_vals[k->to_ullong()]);
    }

    [[nodiscard]] constexpr bool member(const Key& x) const
    {
        return m_vals[x.to_ullong()].has_value();
    }

    [[nodiscard]] constexpr V* get(const Key& x)
    {
        auto& val = m_vals[x.to_ullong()];
        if (!val)
            return nullptr;
        return &*val;
    }

    [[nodiscard]] constexpr const V* get(const Key& x) const
    {
        auto& val = m_vals[x.to_ullong()];
        if (!val)
            return nullptr;
        return &*val;
    }

    [[nodiscard]] std::optional<Key> successor(const Key& x) const
    {
        if (x == 0 && m_vals[1])
            return 1;

        return std::nullopt;
    }

    [[nodiscard]] std::optional<Key> predecessor(const Key& x) const
    {
        if (x == 1 && m_vals[0])
            return 0;

        return std::nullopt;
    }

    void insert(const Key& x, V val)
    {
        m_vals[x.to_ullong()] = std::move(val);
    }

    bool remove(const Key& x)
    {
        bool res = m_vals[x.to_ullong()].has_value();
        m_vals[x.to_ullong()].reset();
        return res;
    }
};

#endif
