#pragma once
#include "veb.hpp"
#include <algorithm>
#include <bitset>
#include <boost/heap/binomial_heap.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/policies.hpp>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <utility>
#include <vector>

template<typename Q>
concept numeric_queue =
    requires(Q q, const Q cq, const std::bitset<Q::key_width>& key,
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
    requires std::movable<T>
class VebQueue {
    Veb<W, std::vector<T>> veb;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] bool empty() const
    {
        return veb.empty();
    }

    void push(const std::bitset<W>& k, T val)
    {
        veb[k].emplace_back(std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        assert(!veb.empty());
        auto [k, values] = *veb.minimum();
        T value = std::move(values.back());
        values.pop_back();
        if (values.empty())
            veb.erase(k);
        return {k, std::move(value)};
    }
};

template<std::size_t W, typename T>
    requires(std::movable<T> && W <= 64)
class PriorityQueueQueue {
    using entry = std::pair<least_uint<W>, T>;
    std::priority_queue<entry, std::vector<entry>, std::greater<>> pq;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] bool empty() const
    {
        return pq.empty();
    }

    void push(const std::bitset<W>& k, T val)
    {
        pq.emplace(k.to_ullong(), std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        auto result = std::move(const_cast<entry&>(pq.top()));
        pq.pop();
        return result;
    }
};

template<std::size_t W, typename T>
    requires(std::movable<T> && W <= 64)
class MultisetQueue {
    using entry = std::pair<least_uint<W>, std::shared_ptr<T>>;
    struct compare {
        bool operator()(const entry& a, const entry& b) const
        {
            return a.first < b.first;
        }
    };
    std::multiset<entry, compare> values;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] bool empty() const
    {
        return values.empty();
    }

    void push(const std::bitset<W>& k, T val)
    {
        values.emplace(k.to_ullong(), std::make_shared<T>(std::move(val)));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        auto it = values.begin();
        auto key = it->first;
        T value = std::move(*it->second);
        values.erase(it);
        return {key, std::move(value)};
    }
};

template<std::size_t W, typename T>
    requires(std::movable<T> && W <= 64)
class BinomialHeapQueue {
    using entry = std::pair<least_uint<W>, T>;
    boost::heap::binomial_heap<entry, boost::heap::compare<std::greater<>>>
        heap;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] bool empty() const
    {
        return heap.empty();
    }

    void push(const std::bitset<W>& k, T val)
    {
        heap.emplace(k.to_ullong(), std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        auto result = std::move(const_cast<entry&>(heap.top()));
        heap.pop();
        return result;
    }
};

template<std::size_t W, typename T>
    requires(std::movable<T> && W <= 64)
class FibonacciHeapQueue {
    using entry = std::pair<least_uint<W>, T>;
    boost::heap::fibonacci_heap<entry, boost::heap::compare<std::greater<>>>
        heap;

public:
    using value_type = T;
    static constexpr std::size_t key_width = W;

    [[nodiscard]] bool empty() const
    {
        return heap.empty();
    }

    void push(const std::bitset<W>& k, T val)
    {
        heap.emplace(k.to_ullong(), std::move(val));
    }

    std::pair<std::bitset<W>, T> pop()
    {
        auto result = std::move(const_cast<entry&>(heap.top()));
        heap.pop();
        return result;
    }
};
