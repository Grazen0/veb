#ifndef EXPERIMENTS_NUMERIC_QUEUE_HPP
#define EXPERIMENTS_NUMERIC_QUEUE_HPP

#include <algorithm>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <type_traits>
#include <veb.hpp>
#include <vector>

template<typename Q>
concept numeric_queue =
    requires(Q q, const Q& cq, const std::bitset<Q::key_width>& key,
             typename Q::value_type val) {
        typename Q::value_type;
        { Q::key_width } -> std::same_as<const std::size_t&>;
        { cq.empty() } -> std::same_as<bool>;
        { q.push(key, std::move(val)) } -> std::same_as<void>;
        {
            q.pop()
        } -> std::same_as<
            std::pair<std::bitset<Q::key_width>, typename Q::value_type>>;
    };

template<std::size_t W, typename T>
    requires(std::movable<T>)
class VebQueue {
private:
    Veb<W, std::vector<T>> veb;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] constexpr bool empty() const
    {
        return veb.empty();
    }

    constexpr void push(const std::bitset<W>& k, T val)
    {
        veb[k].emplace_back(std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        assert(!veb.empty());

        auto [k, vs] = *veb.minimum();
        assert(!vs->empty());

        T val = std::move(vs->back());
        vs->pop_back();

        if (vs->empty())
            veb.erase(k);

        return std::make_pair(k, std::move(val));
    }
};

template<std::size_t W, typename T>
    requires(std::movable<T> && W <= 64)
class PriorityQueueQueue {
private:
    using KeyInt = std::conditional_t<
        W <= 8, std::uint8_t,
        std::conditional_t<
            W <= 16, std::uint16_t,
            std::conditional_t<W <= 32, std::uint32_t, std::uint64_t>>>;

    using Entry = std::pair<KeyInt, T>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> pq;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] constexpr bool empty() const
    {
        return pq.empty();
    }

    constexpr void push(const std::bitset<W>& k, T val)
    {
        pq.emplace(k.to_ullong(), std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        auto [k, val] = std::move(const_cast<Entry&>(pq.top()));
        pq.pop();

        return std::make_pair(k, std::move(val));
    }
};

#endif
