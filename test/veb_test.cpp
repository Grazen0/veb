#include <bitset>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <veb.hpp>
#include <vector>

namespace
{
    using u64 = unsigned long long;
    using Opt = std::optional<u64>;

    template<std::size_t W>
    using Bits = std::bitset<W>;

    template<std::size_t W>
    constexpr u64 universe_size()
    {
        return 1ULL << W;
    }

    // Convert an optional<bitset<W>> into an optional<u64> so results can be
    // compared against plain integers.
    template<std::size_t W>
    Opt val(const std::optional<std::bitset<W>>& o)
    {
        if (!o)
            return std::nullopt;
        return o->to_ullong();
    }

    // Reference implementations on top of std::set
    Opt ref_succ(const std::set<u64>& ref, u64 x)
    {
        auto it = ref.upper_bound(x);
        if (it == ref.end())
            return std::nullopt;
        return *it;
    }

    Opt ref_pred(const std::set<u64>& ref, u64 x)
    {
        auto it = ref.lower_bound(x);
        if (it == ref.begin())
            return std::nullopt;
        return *std::prev(it);
    }

    Opt ref_min(const std::set<u64>& ref)
    {
        if (ref.empty())
            return std::nullopt;
        return *ref.begin();
    }

    Opt ref_max(const std::set<u64>& ref)
    {
        if (ref.empty())
            return std::nullopt;
        return *ref.rbegin();
    }

    // Check every public query for a single key against the reference.
    template<std::size_t W>
    void check_key(const Veb<W>& veb, const std::set<u64>& ref, u64 x)
    {
        CAPTURE(W, x);
        REQUIRE(veb.member(Bits<W>(x)) == (ref.count(x) == 1));
        REQUIRE(val(veb.successor(Bits<W>(x))) == ref_succ(ref, x));
        REQUIRE(val(veb.predecessor(Bits<W>(x))) == ref_pred(ref, x));
    }

    // Check min/max and every key in the universe.
    template<std::size_t W>
    void check_full(const Veb<W>& veb, const std::set<u64>& ref)
    {
        REQUIRE(val(veb.minimum()) == ref_min(ref));
        REQUIRE(val(veb.maximum()) == ref_max(ref));
        for (u64 x = 0; x < universe_size<W>(); ++x)
            check_key(veb, ref, x);
    }

    template<std::size_t W>
    void check_min_max(const Veb<W>& veb, const std::set<u64>& ref)
    {
        REQUIRE(val(veb.minimum()) == ref_min(ref));
        REQUIRE(val(veb.maximum()) == ref_max(ref));
    }

    template<std::size_t W>
    std::unique_ptr<Veb<W>> make_veb(const std::vector<u64>& keys)
    {
        auto veb = std::make_unique<Veb<W>>();
        for (auto k : keys)
            veb->insert(Bits<W>(k));
        return veb;
    }
} // namespace

// ---------------------------------------------------------------------------
// Basics
// ---------------------------------------------------------------------------

TEST_CASE("misc veb operations", "[veb]")
{
    auto veb = std::make_unique<Veb<8>>();
    veb->insert(1);
    veb->insert(2);
    veb->insert(3);

    REQUIRE(veb->member(1));
    REQUIRE(veb->member(2));
    REQUIRE(veb->member(3));
    REQUIRE(!veb->member(4));
    REQUIRE(!veb->member(42));
}

TEMPLATE_TEST_CASE_SIG("empty veb", "[veb][empty]", ((std::size_t W), W), 2, 4,
                       8, 16)
{
    auto veb = std::make_unique<Veb<W>>();
    std::set<u64> ref;

    REQUIRE(!veb->minimum().has_value());
    REQUIRE(!veb->maximum().has_value());

    // a handful of probes including the boundaries
    const u64 last = universe_size<W>() - 1;
    for (u64 x : {u64{0}, u64{1}, last / 2, last - 1, last}) {
        REQUIRE(!veb->member(Bits<W>(x)));
        REQUIRE(!veb->successor(Bits<W>(x)).has_value());
        REQUIRE(!veb->predecessor(Bits<W>(x)).has_value());
        check_key(*veb, ref, x);
    }
}

TEMPLATE_TEST_CASE_SIG("single element", "[veb][single]", ((std::size_t W), W),
                       2, 4, 8, 16)
{
    const u64 last = universe_size<W>() - 1;
    const std::vector<u64> keys = {0, 1, last / 2, last - 1, last};

    for (u64 k : keys) {
        CAPTURE(W, k);
        auto veb = make_veb<W>({k});
        std::set<u64> ref{k};

        REQUIRE(veb->member(Bits<W>(k)));
        REQUIRE(val(veb->minimum()) == Opt{k});
        REQUIRE(val(veb->maximum()) == Opt{k});

        // nothing strictly after / before the only element
        REQUIRE(!veb->successor(Bits<W>(k)).has_value());
        REQUIRE(!veb->predecessor(Bits<W>(k)).has_value());

        if (k > 0) {
            REQUIRE(val(veb->successor(Bits<W>(k - 1))) == Opt{k});
            REQUIRE(!veb->member(Bits<W>(k - 1)));
        }
        if (k < last) {
            REQUIRE(val(veb->predecessor(Bits<W>(k + 1))) == Opt{k});
            REQUIRE(!veb->member(Bits<W>(k + 1)));
        }

        if (W <= 8)
            check_full(*veb, ref);
    }
}

TEST_CASE("veb of width 1", "[veb][w1]")
{
    SECTION("empty")
    {
        Veb<1> veb;
        REQUIRE(!veb.minimum().has_value());
        REQUIRE(!veb.maximum().has_value());
        REQUIRE(!veb.member(0));
        REQUIRE(!veb.member(1));
        REQUIRE(!veb.successor(0).has_value());
        REQUIRE(!veb.successor(1).has_value());
        REQUIRE(!veb.predecessor(0).has_value());
        REQUIRE(!veb.predecessor(1).has_value());
    }

    SECTION("only 0")
    {
        Veb<1> veb;
        veb.insert(0);
        REQUIRE(veb.member(0));
        REQUIRE(!veb.member(1));
        REQUIRE(val(veb.minimum()) == Opt{0});
        REQUIRE(val(veb.maximum()) == Opt{0});
        REQUIRE(!veb.successor(0).has_value());
        REQUIRE(!veb.successor(1).has_value());
        REQUIRE(!veb.predecessor(0).has_value());
        REQUIRE(val(veb.predecessor(1)) == Opt{0});
    }

    SECTION("only 1")
    {
        Veb<1> veb;
        veb.insert(1);
        REQUIRE(!veb.member(0));
        REQUIRE(veb.member(1));
        REQUIRE(val(veb.minimum()) == Opt{1});
        REQUIRE(val(veb.maximum()) == Opt{1});
        REQUIRE(val(veb.successor(0)) == Opt{1});
        REQUIRE(!veb.successor(1).has_value());
        REQUIRE(!veb.predecessor(0).has_value());
        REQUIRE(!veb.predecessor(1).has_value());
    }

    SECTION("both, inserted 0 then 1")
    {
        Veb<1> veb;
        veb.insert(0);
        veb.insert(1);
        REQUIRE(veb.member(0));
        REQUIRE(veb.member(1));
        REQUIRE(val(veb.minimum()) == Opt{0});
        REQUIRE(val(veb.maximum()) == Opt{1});
        REQUIRE(val(veb.successor(0)) == Opt{1});
        REQUIRE(!veb.successor(1).has_value());
        REQUIRE(!veb.predecessor(0).has_value());
        REQUIRE(val(veb.predecessor(1)) == Opt{0});
    }

    SECTION("both, inserted 1 then 0")
    {
        Veb<1> veb;
        veb.insert(1);
        veb.insert(0);
        REQUIRE(val(veb.minimum()) == Opt{0});
        REQUIRE(val(veb.maximum()) == Opt{1});
        REQUIRE(val(veb.successor(0)) == Opt{1});
        REQUIRE(val(veb.predecessor(1)) == Opt{0});
    }

    SECTION("remove from a full tree")
    {
        Veb<1> veb;
        veb.insert(0);
        veb.insert(1);
        veb.remove(0);
        REQUIRE(!veb.member(0));
        REQUIRE(veb.member(1));
        REQUIRE(val(veb.minimum()) == Opt{1});
        REQUIRE(val(veb.maximum()) == Opt{1});

        Veb<1> veb2;
        veb2.insert(0);
        veb2.insert(1);
        veb2.remove(1);
        REQUIRE(veb2.member(0));
        REQUIRE(!veb2.member(1));
        REQUIRE(val(veb2.minimum()) == Opt{0});
        REQUIRE(val(veb2.maximum()) == Opt{0});
    }

    SECTION("remove last remaining element")
    {
        Veb<1> veb;
        veb.insert(1);
        veb.remove(1);
        REQUIRE(!veb.minimum().has_value());
        REQUIRE(!veb.maximum().has_value());
        REQUIRE(!veb.member(1));

        // can be reused
        veb.insert(0);
        REQUIRE(veb.member(0));
        REQUIRE(val(veb.minimum()) == Opt{0});
    }
}

// ---------------------------------------------------------------------------
// insert / member
// ---------------------------------------------------------------------------

TEST_CASE("insert and member on Veb<8>", "[veb][member][insert]")
{
    auto veb = std::make_unique<Veb<8>>();

    SECTION("boundaries")
    {
        veb->insert(0);
        veb->insert(255);
        REQUIRE(veb->member(0));
        REQUIRE(veb->member(255));
        REQUIRE(!veb->member(1));
        REQUIRE(!veb->member(254));
        REQUIRE(!veb->member(128));
    }

    SECTION("powers of two")
    {
        std::set<u64> ref;
        for (u64 p = 1; p < 256; p <<= 1) {
            veb->insert(Bits<8>(p));
            ref.insert(p);
        }
        check_full(*veb, ref);
    }

    SECTION("elements sharing a cluster")
    {
        // high nibble identical, low nibble differs
        for (u64 lo : {0x1, 0x5, 0xA, 0xF})
            veb->insert(Bits<8>(0x30 | lo));
        for (u64 lo = 0; lo < 16; ++lo) {
            bool expected = lo == 0x1 || lo == 0x5 || lo == 0xA || lo == 0xF;
            REQUIRE(veb->member(Bits<8>(0x30 | lo)) == expected);
        }
        REQUIRE(!veb->member(Bits<8>(0x20 | 0x1)));
        REQUIRE(!veb->member(Bits<8>(0x40 | 0x1)));
    }

    SECTION("elements in different clusters, same low bits")
    {
        for (u64 hi = 0; hi < 16; hi += 3)
            veb->insert(Bits<8>((hi << 4) | 0x7));
        for (u64 hi = 0; hi < 16; ++hi)
            REQUIRE(veb->member(Bits<8>((hi << 4) | 0x7)) == (hi % 3 == 0));
    }

    SECTION("ascending insertion")
    {
        std::set<u64> ref;
        for (u64 x = 0; x < 256; x += 2) {
            veb->insert(Bits<8>(x));
            ref.insert(x);
        }
        check_full(*veb, ref);
    }

    SECTION("descending insertion")
    {
        std::set<u64> ref;
        for (u64 x = 255; x > 0; x -= 3) {
            veb->insert(Bits<8>(x));
            ref.insert(x);
        }
        check_full(*veb, ref);
    }
}

TEMPLATE_TEST_CASE_SIG("insert full universe", "[veb][insert][full]",
                       ((std::size_t W), W), 2, 4, 8)
{
    auto veb = std::make_unique<Veb<W>>();
    std::set<u64> ref;

    for (u64 x = 0; x < universe_size<W>(); ++x) {
        veb->insert(Bits<W>(x));
        ref.insert(x);
        check_min_max(*veb, ref);
        REQUIRE(veb->member(Bits<W>(x)));
    }
    check_full(*veb, ref);

    // every key's neighbours are exactly its integer neighbours
    for (u64 x = 0; x + 1 < universe_size<W>(); ++x) {
        REQUIRE(val(veb->successor(Bits<W>(x))) == Opt{x + 1});
        REQUIRE(val(veb->predecessor(Bits<W>(x + 1))) == Opt{x});
    }
    REQUIRE(!veb->successor(Bits<W>(universe_size<W>() - 1)).has_value());
    REQUIRE(!veb->predecessor(Bits<W>(0)).has_value());
}

TEMPLATE_TEST_CASE_SIG("insert in reverse order", "[veb][insert][full]",
                       ((std::size_t W), W), 2, 4, 8)
{
    auto veb = std::make_unique<Veb<W>>();
    std::set<u64> ref;

    for (u64 x = universe_size<W>(); x-- > 0;) {
        veb->insert(Bits<W>(x));
        ref.insert(x);
        check_min_max(*veb, ref);
    }
    check_full(*veb, ref);
}

// ---------------------------------------------------------------------------
// minimum / maximum
// ---------------------------------------------------------------------------

TEST_CASE("minimum and maximum track inserts", "[veb][min][max]")
{
    auto veb = std::make_unique<Veb<8>>();

    veb->insert(100);
    REQUIRE(val(veb->minimum()) == Opt{100});
    REQUIRE(val(veb->maximum()) == Opt{100});

    veb->insert(150);
    REQUIRE(val(veb->minimum()) == Opt{100});
    REQUIRE(val(veb->maximum()) == Opt{150});

    veb->insert(50);
    REQUIRE(val(veb->minimum()) == Opt{50});
    REQUIRE(val(veb->maximum()) == Opt{150});

    veb->insert(101); // between, must not change anything
    REQUIRE(val(veb->minimum()) == Opt{50});
    REQUIRE(val(veb->maximum()) == Opt{150});

    veb->insert(0);
    REQUIRE(val(veb->minimum()) == Opt{0});
    REQUIRE(val(veb->maximum()) == Opt{150});

    veb->insert(255);
    REQUIRE(val(veb->minimum()) == Opt{0});
    REQUIRE(val(veb->maximum()) == Opt{255});
}

TEST_CASE("minimum and maximum track removals", "[veb][min][max][remove]")
{
    auto veb = make_veb<8>({0, 10, 20, 30, 200, 255});
    std::set<u64> ref{0, 10, 20, 30, 200, 255};

    for (u64 x : {u64{0}, u64{255}, u64{10}, u64{200}, u64{30}}) {
        veb->remove(Bits<8>(x));
        ref.erase(x);
        check_min_max(*veb, ref);
    }

    REQUIRE(val(veb->minimum()) == Opt{20});
    REQUIRE(val(veb->maximum()) == Opt{20});

    veb->remove(Bits<8>(20));
    REQUIRE(!veb->minimum().has_value());
    REQUIRE(!veb->maximum().has_value());
}

// ---------------------------------------------------------------------------
// successor / predecessor
// ---------------------------------------------------------------------------

TEST_CASE("successor on a sparse Veb<8>", "[veb][successor]")
{
    auto veb = make_veb<8>({5, 17, 18, 64, 200});

    SECTION("query is a member")
    {
        REQUIRE(val(veb->successor(5)) == Opt{17});
        REQUIRE(val(veb->successor(17)) == Opt{18});
        REQUIRE(val(veb->successor(18)) == Opt{64});
        REQUIRE(val(veb->successor(64)) == Opt{200});
        REQUIRE(!veb->successor(200).has_value());
    }

    SECTION("query is not a member")
    {
        REQUIRE(val(veb->successor(0)) == Opt{5});
        REQUIRE(val(veb->successor(4)) == Opt{5});
        REQUIRE(val(veb->successor(6)) == Opt{17});
        REQUIRE(val(veb->successor(16)) == Opt{17});
        REQUIRE(val(veb->successor(19)) == Opt{64});
        REQUIRE(val(veb->successor(63)) == Opt{64});
        REQUIRE(val(veb->successor(65)) == Opt{200});
        REQUIRE(val(veb->successor(199)) == Opt{200});
        REQUIRE(!veb->successor(201).has_value());
        REQUIRE(!veb->successor(255).has_value());
    }

    SECTION("successor crosses cluster boundaries")
    {
        // 18 = 0x12, 64 = 0x40 : clusters 1 and 4, with empty clusters between
        REQUIRE(val(veb->successor(0x1F)) == Opt{64});
        REQUIRE(val(veb->successor(0x20)) == Opt{64});
        REQUIRE(val(veb->successor(0x3F)) == Opt{64});
    }
}

TEST_CASE("predecessor on a sparse Veb<8>", "[veb][predecessor]")
{
    auto veb = make_veb<8>({5, 17, 18, 64, 200});

    SECTION("query is a member")
    {
        REQUIRE(!veb->predecessor(5).has_value());
        REQUIRE(val(veb->predecessor(17)) == Opt{5});
        REQUIRE(val(veb->predecessor(18)) == Opt{17});
        REQUIRE(val(veb->predecessor(64)) == Opt{18});
        REQUIRE(val(veb->predecessor(200)) == Opt{64});
    }

    SECTION("query is not a member")
    {
        REQUIRE(!veb->predecessor(0).has_value());
        REQUIRE(!veb->predecessor(4).has_value());
        REQUIRE(val(veb->predecessor(6)) == Opt{5});
        REQUIRE(val(veb->predecessor(16)) == Opt{5});
        REQUIRE(val(veb->predecessor(19)) == Opt{18});
        REQUIRE(val(veb->predecessor(63)) == Opt{18});
        REQUIRE(val(veb->predecessor(65)) == Opt{64});
        REQUIRE(val(veb->predecessor(199)) == Opt{64});
        REQUIRE(val(veb->predecessor(201)) == Opt{200});
        REQUIRE(val(veb->predecessor(255)) == Opt{200});
    }

    SECTION("predecessor crosses cluster boundaries")
    {
        REQUIRE(val(veb->predecessor(0x40)) == Opt{18});
        REQUIRE(val(veb->predecessor(0x30)) == Opt{18});
        REQUIRE(val(veb->predecessor(0x20)) == Opt{18});
        REQUIRE(val(veb->predecessor(0x10)) == Opt{5});
    }
}

TEST_CASE("successor and predecessor use the boundary elements",
          "[veb][successor][predecessor]")
{
    auto veb = make_veb<8>({0, 255});

    REQUIRE(val(veb->successor(0)) == Opt{255});
    REQUIRE(val(veb->successor(1)) == Opt{255});
    REQUIRE(val(veb->successor(254)) == Opt{255});
    REQUIRE(!veb->successor(255).has_value());

    REQUIRE(val(veb->predecessor(255)) == Opt{0});
    REQUIRE(val(veb->predecessor(254)) == Opt{0});
    REQUIRE(val(veb->predecessor(1)) == Opt{0});
    REQUIRE(!veb->predecessor(0).has_value());
}

TEST_CASE("successor and predecessor within a single cluster",
          "[veb][successor][predecessor]")
{
    auto veb = make_veb<8>({0x50, 0x53, 0x57, 0x5F});

    REQUIRE(val(veb->successor(0x50)) == Opt{0x53});
    REQUIRE(val(veb->successor(0x51)) == Opt{0x53});
    REQUIRE(val(veb->successor(0x53)) == Opt{0x57});
    REQUIRE(val(veb->successor(0x58)) == Opt{0x5F});
    REQUIRE(!veb->successor(0x5F).has_value());

    REQUIRE(!veb->predecessor(0x50).has_value());
    REQUIRE(val(veb->predecessor(0x51)) == Opt{0x50});
    REQUIRE(val(veb->predecessor(0x57)) == Opt{0x53});
    REQUIRE(val(veb->predecessor(0x5E)) == Opt{0x57});
    REQUIRE(val(veb->predecessor(0x5F)) == Opt{0x57});
}

TEST_CASE("successor walk enumerates all elements in order", "[veb][successor]")
{
    std::vector<u64> keys = {3, 9, 10, 31, 32, 33, 100, 128, 129, 250, 255};
    auto veb = make_veb<8>(keys);

    std::vector<u64> walked;
    auto cur = veb->minimum();
    while (cur) {
        walked.push_back(cur->to_ullong());
        cur = veb->successor(*cur);
    }
    REQUIRE(walked == keys);
}

TEST_CASE("predecessor walk enumerates all elements in reverse order",
          "[veb][predecessor]")
{
    std::vector<u64> keys = {3, 9, 10, 31, 32, 33, 100, 128, 129, 250, 255};
    auto veb = make_veb<8>(keys);

    std::vector<u64> walked;
    auto cur = veb->maximum();
    while (cur) {
        walked.push_back(cur->to_ullong());
        cur = veb->predecessor(*cur);
    }
    std::reverse(walked.begin(), walked.end());
    REQUIRE(walked == keys);
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

TEST_CASE("remove the only element", "[veb][remove]")
{
    auto veb = make_veb<8>({77});
    veb->remove(77);

    REQUIRE(!veb->member(77));
    REQUIRE(!veb->minimum().has_value());
    REQUIRE(!veb->maximum().has_value());
    REQUIRE(!veb->successor(0).has_value());
    REQUIRE(!veb->predecessor(255).has_value());
}

TEST_CASE("remove from a two element tree", "[veb][remove]")
{
    SECTION("remove the min")
    {
        auto veb = make_veb<8>({10, 20});
        veb->remove(10);
        REQUIRE(!veb->member(10));
        REQUIRE(veb->member(20));
        REQUIRE(val(veb->minimum()) == Opt{20});
        REQUIRE(val(veb->maximum()) == Opt{20});
    }

    SECTION("remove the max")
    {
        auto veb = make_veb<8>({10, 20});
        veb->remove(20);
        REQUIRE(veb->member(10));
        REQUIRE(!veb->member(20));
        REQUIRE(val(veb->minimum()) == Opt{10});
        REQUIRE(val(veb->maximum()) == Opt{10});
    }

    SECTION("two elements in the same cluster")
    {
        auto veb = make_veb<8>({0x21, 0x2E});
        veb->remove(0x21);
        REQUIRE(val(veb->minimum()) == Opt{0x2E});
        REQUIRE(val(veb->maximum()) == Opt{0x2E});
        REQUIRE(veb->member(0x2E));
        REQUIRE(!veb->member(0x21));
    }

    SECTION("two elements in the same cluster, remove the max")
    {
        auto veb = make_veb<8>({0x21, 0x2E});
        veb->remove(0x2E);
        REQUIRE(val(veb->minimum()) == Opt{0x21});
        REQUIRE(val(veb->maximum()) == Opt{0x21});
        REQUIRE(veb->member(0x21));
        REQUIRE(!veb->member(0x2E));
    }
}

TEST_CASE("remove min, max and middle elements of Veb<8>", "[veb][remove]")
{
    const std::vector<u64> keys = {1, 2, 17, 18, 33, 100, 101, 200, 254};

    SECTION("remove the minimum repeatedly")
    {
        auto veb = make_veb<8>(keys);
        std::set<u64> ref(keys.begin(), keys.end());
        while (!ref.empty()) {
            u64 m = *ref.begin();
            veb->remove(Bits<8>(m));
            ref.erase(m);
            check_full(*veb, ref);
        }
    }

    SECTION("remove the maximum repeatedly")
    {
        auto veb = make_veb<8>(keys);
        std::set<u64> ref(keys.begin(), keys.end());
        while (!ref.empty()) {
            u64 m = *ref.rbegin();
            veb->remove(Bits<8>(m));
            ref.erase(m);
            check_full(*veb, ref);
        }
    }

    SECTION("remove middle elements, keeping min and max")
    {
        auto veb = make_veb<8>(keys);
        std::set<u64> ref(keys.begin(), keys.end());
        for (u64 x : {u64{33}, u64{18}, u64{100}, u64{2}, u64{200}}) {
            veb->remove(Bits<8>(x));
            ref.erase(x);
            check_full(*veb, ref);
        }
        REQUIRE(val(veb->minimum()) == Opt{1});
        REQUIRE(val(veb->maximum()) == Opt{254});
    }

    SECTION("alternate min and max")
    {
        auto veb = make_veb<8>(keys);
        std::set<u64> ref(keys.begin(), keys.end());
        bool take_min = true;
        while (!ref.empty()) {
            u64 x = take_min ? *ref.begin() : *ref.rbegin();
            veb->remove(Bits<8>(x));
            ref.erase(x);
            check_full(*veb, ref);
            take_min = !take_min;
        }
    }
}

TEST_CASE("remove empties a cluster", "[veb][remove]")
{
    // cluster 3 holds a single element, cluster 5 holds several
    auto veb = make_veb<8>({0x05, 0x30, 0x50, 0x51, 0x5F, 0xF0});
    std::set<u64> ref{0x05, 0x30, 0x50, 0x51, 0x5F, 0xF0};

    veb->remove(0x30);
    ref.erase(0x30);
    check_full(*veb, ref);
    REQUIRE(val(veb->successor(0x05)) == Opt{0x50});
    REQUIRE(val(veb->predecessor(0x50)) == Opt{0x05});

    // empty cluster 5 completely
    for (u64 x : {u64{0x51}, u64{0x50}, u64{0x5F}}) {
        veb->remove(Bits<8>(x));
        ref.erase(x);
        check_full(*veb, ref);
    }
    REQUIRE(val(veb->successor(0x05)) == Opt{0xF0});
    REQUIRE(val(veb->predecessor(0xF0)) == Opt{0x05});
}

TEST_CASE("remove updates max when max cluster becomes empty",
          "[veb][remove][max]")
{
    auto veb = make_veb<8>({0x01, 0x40, 0x80, 0xC0});
    veb->remove(0xC0);
    REQUIRE(val(veb->maximum()) == Opt{0x80});
    veb->remove(0x80);
    REQUIRE(val(veb->maximum()) == Opt{0x40});
    veb->remove(0x40);
    REQUIRE(val(veb->maximum()) == Opt{0x01});
    REQUIRE(val(veb->minimum()) == Opt{0x01});
}

TEST_CASE("remove updates min when min cluster becomes empty",
          "[veb][remove][min]")
{
    auto veb = make_veb<8>({0x01, 0x40, 0x80, 0xC0});
    veb->remove(0x01);
    REQUIRE(val(veb->minimum()) == Opt{0x40});
    veb->remove(0x40);
    REQUIRE(val(veb->minimum()) == Opt{0x80});
    veb->remove(0x80);
    REQUIRE(val(veb->minimum()) == Opt{0xC0});
    REQUIRE(val(veb->maximum()) == Opt{0xC0});
}

TEST_CASE("reinsert after remove", "[veb][remove][insert]")
{
    auto veb = make_veb<8>({10, 20, 30});
    std::set<u64> ref{10, 20, 30};

    veb->remove(20);
    ref.erase(20);
    check_full(*veb, ref);

    veb->insert(20);
    ref.insert(20);
    check_full(*veb, ref);

    veb->remove(10);
    ref.erase(10);
    veb->insert(10);
    ref.insert(10);
    check_full(*veb, ref);

    veb->remove(30);
    ref.erase(30);
    veb->insert(30);
    ref.insert(30);
    check_full(*veb, ref);
}

TEST_CASE("reuse after emptying", "[veb][remove][insert]")
{
    auto veb = make_veb<8>({1, 2, 3, 100, 200});

    for (u64 x : {u64{1}, u64{2}, u64{3}, u64{100}, u64{200}})
        veb->remove(Bits<8>(x));

    REQUIRE(!veb->minimum().has_value());
    REQUIRE(!veb->maximum().has_value());
    for (u64 x = 0; x < 256; ++x)
        REQUIRE(!veb->member(Bits<8>(x)));

    std::set<u64> ref;
    for (u64 x : {u64{7}, u64{250}, u64{128}}) {
        veb->insert(Bits<8>(x));
        ref.insert(x);
    }
    check_full(*veb, ref);
}
