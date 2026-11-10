#ifndef VEB_VEB_HPP
#define VEB_VEB_HPP

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

template<std::size_t W, typename V>
    requires(1 <= W && W <= 64) // required for to_ullong() not to explode
class Veb {
private:
    static constexpr std::size_t H = (W + 1) / 2;
    static constexpr std::size_t L = W / 2;

    using Key = std::bitset<W>;
    using Entry = std::pair<Key, V>;
    using EntryRef = std::pair<Key, V&>;
    using EntryRefConst = std::pair<Key, const V&>;

    std::unique_ptr<Veb<H, std::monostate>> m_summary{};
    std::unordered_map<std::bitset<H>, Veb<L, V>> m_clusters{};
    std::optional<Entry> m_min{};
    std::optional<Key> m_max{};

    template<std::size_t W2>
        requires(2 <= W2)
    static constexpr std::pair<std::bitset<(W2 + 1) / 2>, std::bitset<W2 / 2>>
    split(const std::bitset<W2>& n)
    {
        constexpr std::size_t H = (W2 + 1) / 2;
        constexpr std::size_t L = W2 / 2;
        std::bitset<H> hi{n.to_ullong() >> L};
        std::bitset<L> lo{n.to_ullong()};
        return {hi, lo};
    }

    template<std::size_t H, std::size_t L>
        requires(L >= 1 && L <= H && H <= L + 1)
    static constexpr std::bitset<H + L> join(const std::bitset<H>& hi,
                                             const std::bitset<L>& lo)
    {
        return lo.to_ullong() | (hi.to_ullong() << L);
    }

public:
    Veb() = default;

    [[nodiscard]] constexpr bool empty() const
    {
        return !m_min.has_value();
    }

    [[nodiscard]] constexpr std::optional<Key> min_key() const
    {
        if (!m_min)
            return std::nullopt;

        return m_min->first;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> cminimum() const
    {
        if (!m_min)
            return std::nullopt;

        return std::make_pair(m_min->first,
                              std::reference_wrapper{m_min->second});
    }

    [[nodiscard]] constexpr std::optional<EntryRef> minimum()
    {
        if (!m_min)
            return std::nullopt;

        return std::make_pair(m_min->first,
                              std::reference_wrapper{m_min->second});
    }

    [[nodiscard]] constexpr std::optional<Key> max_key() const
    {
        return m_max;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> cmaximum() const
    {
        if (!m_max)
            return std::nullopt;
        return std::make_pair(*m_max, std::reference_wrapper{at(*m_max)});
    }

    [[nodiscard]] constexpr std::optional<EntryRef> maximum()
    {
        if (!m_max)
            return std::nullopt;
        return std::make_pair(*m_max, std::reference_wrapper{at(*m_max)});
    }

    [[nodiscard]] V& operator[](const Key& k)
        requires(std::is_default_constructible_v<V>)
    {
        if (!count(k))
            insert(k, V{});

        return at(k);
    }

    [[nodiscard]] const V& at(const Key& k) const
    {
        if (!m_min)
            throw std::out_of_range("veb key out of range");

        if (k == m_min->first)
            return m_min->second;

        auto [hi, lo] = split(k);
        if (!m_clusters.count(hi))
            throw std::out_of_range("veb key out of range");

        return m_clusters.at(hi).at(lo);
    }

    [[nodiscard]] V& at(const Key& k)
    {
        if (!m_min)
            throw std::out_of_range("veb key out of range");

        if (k == m_min->first)
            return m_min->second;

        auto [hi, lo] = split(k);
        if (!m_clusters.count(hi))
            throw std::out_of_range("veb key out of range");

        return m_clusters.at(hi).at(lo);
    }

    [[nodiscard]] std::size_t count(const Key& k) const
    {
        if (!m_min)
            return 0;

        if (k == m_min->first || k == m_max)
            return 1;

        auto [hi, lo] = split(k);
        if (!m_clusters.count(hi))
            return 0;

        return m_clusters.at(hi).count(lo);
    }

    [[nodiscard]] std::optional<Key> successor(const Key& k) const
    {
        if (m_min && k.to_ullong() < m_min->first.to_ullong())
            return m_min->first;

        auto [hi, lo] = split(k);
        if (m_clusters.count(hi)) {
            auto& hi_cluster = m_clusters.at(hi);
            auto max_low = hi_cluster.max_key();

            if (max_low && lo.to_ullong() < max_low->to_ullong()) {
                auto offset = hi_cluster.successor(lo);
                return join(hi, *offset);
            }
        }

        if (!m_summary)
            return std::nullopt;

        auto succ_cluster = m_summary->successor(hi);
        if (!succ_cluster)
            return std::nullopt;

        auto offset = m_clusters.at(*succ_cluster).min_key();
        return join(*succ_cluster, *offset);
    }

    [[nodiscard]] std::optional<Key> predecessor(const Key& k) const
    {
        if (m_max && k.to_ullong() > m_max->to_ullong())
            return m_max;

        auto [hi, lo] = split(k);
        if (m_clusters.count(hi)) {
            auto& hi_cluster = m_clusters.at(hi);
            auto min_low = hi_cluster.min_key();

            if (min_low && lo.to_ullong() > min_low->to_ullong()) {
                auto offset = hi_cluster.predecessor(lo);
                return join(hi, *offset);
            }
        }

        if (!m_summary)
            return std::nullopt;

        auto pred_cluster = m_summary->predecessor(hi);
        if (!pred_cluster) {
            if (m_min && k.to_ullong() > m_min->first.to_ullong())
                return m_min->first;

            return std::nullopt;
        }

        auto offset = m_clusters.at(*pred_cluster).max_key();
        return join(*pred_cluster, *offset);
    }

    void insert(Key k, V val)
    {
        if (!m_min) {
            assert(!m_max);
            m_min = {k, std::move(val)};
            m_max = k;
            return;
        }

        if (k == m_min->first) {
            m_min->second = std::move(val);
            return;
        }

        if (k.to_ullong() < m_min->first.to_ullong()) {
            std::swap(k, m_min->first);
            std::swap(val, m_min->second);
        }

        auto [hi, lo] = split(k);
        auto& hi_cluster = m_clusters[hi];

        if (!hi_cluster.min_key()) {
            if (!m_summary)
                m_summary = std::make_unique<Veb<H, std::monostate>>();

            m_summary->insert(hi, {});
        }

        hi_cluster.insert(lo, std::move(val));

        if (k.to_ullong() > m_max->to_ullong())
            m_max = k;
    }

    bool erase(Key k)
    {
        if (!m_min)
            return false;

        if (m_min->first == m_max) {
            if (k != m_min->first)
                return false;

            m_min.reset();
            m_max.reset();
            return true;
        }

        bool should_erase = false;

        if (k == m_min->first) {
            assert(m_summary);
            auto first_cluster_idx = m_summary->min_key();
            assert(first_cluster_idx);

            auto& first_cluster = m_clusters.at(*first_cluster_idx);
            auto replacement = first_cluster.minimum();
            assert(replacement);

            k = join(*first_cluster_idx, replacement->first);
            m_min = std::make_pair(k, std::move(replacement->second));
            should_erase = true;
        }

        auto [hi, lo] = split(k);
        if (!m_clusters.count(hi))
            return false;

        auto& hi_cluster = m_clusters.at(hi);
        bool erased = hi_cluster.erase(lo);

        if (should_erase)
            assert(erased);
        else if (!erased)
            return false;

        if (!hi_cluster.min_key()) {
            assert(m_summary);
            m_summary->erase(hi);

            if (k == m_max) {
                auto summary_max = m_summary->max_key();
                if (!summary_max) {
                    m_max = m_min->first;
                } else {
                    auto& smax_cluster = m_clusters.at(*summary_max);
                    m_max = join(*summary_max, *smax_cluster.max_key());
                }
            }
        } else if (k == m_max) {
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
    using EntryRef = std::pair<Key, V&>;
    using EntryRefConst = std::pair<Key, const V&>;

    std::array<std::optional<V>, 2> m_vals{};

public:
    Veb() = default;

    [[nodiscard]] constexpr bool empty() const
    {
        return !m_vals[0] && !m_vals[1];
    }

    [[nodiscard]] constexpr std::optional<Key> min_key() const
    {
        if (m_vals[0])
            return 0;
        if (m_vals[1])
            return 1;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> cminimum() const
    {
        auto k = min_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k,
                              std::reference_wrapper{*m_vals[k->to_ullong()]});
    }

    [[nodiscard]] constexpr std::optional<EntryRef> minimum()
    {
        auto k = min_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k,
                              std::reference_wrapper{*m_vals[k->to_ullong()]});
    }

    [[nodiscard]] constexpr std::optional<Key> max_key() const
    {
        if (m_vals[1])
            return 1;
        if (m_vals[0])
            return 0;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<EntryRefConst> cmaximum() const
    {
        auto k = max_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k,
                              std::reference_wrapper{*m_vals[k->to_ullong()]});
    }

    [[nodiscard]] constexpr std::optional<EntryRef> maximum()
    {
        auto k = max_key();
        if (!k)
            return std::nullopt;
        return std::make_pair(*k,
                              std::reference_wrapper{*m_vals[k->to_ullong()]});
    }

    [[nodiscard]] constexpr std::size_t count(const Key& k) const
    {
        return m_vals[k.to_ullong()].has_value();
    }

    [[nodiscard]] constexpr V& at(const Key& k)
    {
        auto& val = m_vals[k.to_ullong()];
        if (!val)
            throw std::out_of_range("veb key out of range");
        return *val;
    }

    [[nodiscard]] constexpr const V& at(const Key& k) const
    {
        auto& val = m_vals[k.to_ullong()];
        if (!val)
            throw std::out_of_range("veb key out of range");
        return *val;
    }

    [[nodiscard]] std::optional<Key> successor(const Key& k) const
    {
        if (k == 0 && m_vals[1])
            return 1;

        return std::nullopt;
    }

    [[nodiscard]] std::optional<Key> predecessor(const Key& k) const
    {
        if (k == 1 && m_vals[0])
            return 0;

        return std::nullopt;
    }

    constexpr void insert(const Key& k, V val)
    {
        m_vals[k.to_ullong()] = std::move(val);
    }

    constexpr bool erase(const Key& k)
    {
        bool res = m_vals[k.to_ullong()].has_value();
        m_vals[k.to_ullong()].reset();
        return res;
    }
};

#endif
