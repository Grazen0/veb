#include <algorithm>
#include <bitset>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <veb.hpp>
#include <vector>

namespace
{
    using u64 = unsigned long long;
    using Opt = std::optional<u64>;
    using OptInt = std::optional<int>;
    using EntryU = std::optional<std::pair<u64, int>>;
    using Model = std::map<u64, int>;

    template<std::size_t W>
    using Bits = std::bitset<W>;

    template<std::size_t W>
    constexpr u64 universe_size()
    {
        return 1ULL << W;
    }

    template<std::size_t W, typename V>
    std::optional<V> lookup(const Veb<W, V>& veb, u64 key)
    {
        if (!veb.count(Bits<W>(key)))
            return std::nullopt;

        return veb.at(Bits<W>(key));
    }

    template<std::size_t W>
    Opt val(const std::optional<std::bitset<W>>& o)
    {
        if (!o)
            return std::nullopt;
        return o->to_ullong();
    }

    template<std::size_t W, typename V>
    EntryU entry(const std::optional<std::pair<std::bitset<W>, V&>>& e)
    {
        if (!e)
            return std::nullopt;
        if (!e->second)
            return std::nullopt;
        return std::pair<u64, int>{e->first.to_ullong(), e->second};
    }

    Opt ref_succ(const Model& m, u64 x)
    {
        auto it = m.upper_bound(x);
        if (it == m.end())
            return std::nullopt;
        return it->first;
    }

    Opt ref_pred(const Model& m, u64 x)
    {
        auto it = m.lower_bound(x);
        if (it == m.begin())
            return std::nullopt;
        return std::prev(it)->first;
    }

    Opt ref_min(const Model& m)
    {
        if (m.empty())
            return std::nullopt;
        return m.begin()->first;
    }

    Opt ref_max(const Model& m)
    {
        if (m.empty())
            return std::nullopt;
        return m.rbegin()->first;
    }

    EntryU ref_min_entry(const Model& m)
    {
        if (m.empty())
            return std::nullopt;
        return std::pair<u64, int>{m.begin()->first, m.begin()->second};
    }

    EntryU ref_max_entry(const Model& m)
    {
        if (m.empty())
            return std::nullopt;
        return std::pair<u64, int>{m.rbegin()->first, m.rbegin()->second};
    }

    template<typename T>
    std::string fmt(const std::optional<T>& o)
    {
        if (!o)
            return "none";
        std::ostringstream os;
        os << *o;
        return os.str();
    }

    std::string fmt(const EntryU& e)
    {
        if (!e)
            return "none";
        std::ostringstream os;
        os << "(" << e->first << ", " << e->second << ")";
        return os.str();
    }

    template<std::size_t W>
    void diff_key(std::ostringstream& os, const Veb<W, int>& veb,
                  const Model& m, u64 x)
    {
        const bool in = m.count(x) == 1;

        if (veb.count(Bits<W>(x)) != in)
            os << "count(" << x << ") expected " << in << "; ";

        auto s = val(veb.successor(Bits<W>(x)));
        if (s != ref_succ(m, x))
            os << "successor(" << x << ") got " << fmt(s) << " expected "
               << fmt(ref_succ(m, x)) << "; ";

        auto p = val(veb.predecessor(Bits<W>(x)));
        if (p != ref_pred(m, x))
            os << "predecessor(" << x << ") got " << fmt(p) << " expected "
               << fmt(ref_pred(m, x)) << "; ";
    }

    template<std::size_t W>
    void diff_min_max(std::ostringstream& os, const Veb<W, int>& veb,
                      const Model& m)
    {
        auto mn = val(veb.min_key());
        if (mn != ref_min(m))
            os << "min_key got " << fmt(mn) << " expected " << fmt(ref_min(m))
               << "; ";

        auto mx = val(veb.max_key());
        if (mx != ref_max(m))
            os << "max_key got " << fmt(mx) << " expected " << fmt(ref_max(m))
               << "; ";

        auto mne = entry<W>(veb.cminimum());
        if (mne != ref_min_entry(m))
            os << "minimum got " << fmt(mne) << " expected "
               << fmt(ref_min_entry(m)) << "; ";

        auto mxe = entry<W>(veb.cmaximum());
        if (mxe != ref_max_entry(m))
            os << "maximum got " << fmt(mxe) << " expected "
               << fmt(ref_max_entry(m)) << "; ";
    }

    template<std::size_t W>
    struct H {
        std::unique_ptr<Veb<W, int>> veb = std::make_unique<Veb<W, int>>();
        Model model;
        int counter = 0;

        void put(u64 k, int v)
        {
            veb->insert(Bits<W>(k), v);
            model[k] = v;
        }

        int put(u64 k)
        {
            int v = ++counter;
            put(k, v);
            return v;
        }

        void put_all(const std::vector<u64>& keys)
        {
            for (u64 k : keys)
                put(k);
        }

        bool erase(u64 k)
        {
            const bool expected = model.erase(k) == 1;
            const bool got = veb->erase(Bits<W>(k));
            CAPTURE(W, k);
            REQUIRE(got == expected);
            return got;
        }

        bool has(u64 k) const
        {
            return veb->count(Bits<W>(k));
        }

        OptInt get(u64 k) const
        {
            const Veb<W, int>& cv = *veb;
            if (!cv.count(Bits<W>(k)))
                return std::nullopt;
            return cv.at(Bits<W>(k));
        }

        Opt succ(u64 k) const
        {
            return val(veb->successor(Bits<W>(k)));
        }
        Opt pred(u64 k) const
        {
            return val(veb->predecessor(Bits<W>(k)));
        }
        Opt min() const
        {
            return val(veb->min_key());
        }
        Opt max() const
        {
            return val(veb->max_key());
        }
        EntryU min_entry() const
        {
            return entry<W>(veb->minimum());
        }
        EntryU max_entry() const
        {
            return entry<W>(veb->maximum());
        }

        void check_key(u64 x) const
        {
            std::ostringstream os;
            diff_key(os, *veb, model, x);
            const std::string d = os.str();
            CAPTURE(W, x);
            INFO(d);
            REQUIRE(d.empty());
        }

        void check_min_max() const
        {
            std::ostringstream os;
            diff_min_max(os, *veb, model);
            const std::string d = os.str();
            CAPTURE(W);
            INFO(d);
            REQUIRE(d.empty());
        }

        void check_all() const
        {
            const u64 last = universe_size<W>() - 1;
            std::set<u64> probes = {0, last / 2, last};
            for (const auto& kv : model) {
                probes.insert(kv.first);
                if (kv.first > 0)
                    probes.insert(kv.first - 1);
                if (kv.first < last)
                    probes.insert(kv.first + 1);
            }

            std::ostringstream os;
            diff_min_max(os, *veb, model);
            for (u64 x : probes)
                diff_key(os, *veb, model, x);
            const std::string d = os.str();
            CAPTURE(W);
            INFO(d);
            REQUIRE(d.empty());
        }
    };
} // namespace

TEST_CASE("misc veb operations", "[veb]")
{
    auto veb = std::make_unique<Veb<8, int>>();
    veb->insert(1, 10);
    veb->insert(2, 20);
    veb->insert(3, 30);

    REQUIRE(veb->count(1));
    REQUIRE(veb->count(2));
    REQUIRE(veb->count(3));
    REQUIRE(!veb->count(4));
    REQUIRE(!veb->count(42));

    REQUIRE(lookup(*veb, 1) == OptInt{10});
    REQUIRE(lookup(*veb, 2) == OptInt{20});
    REQUIRE(lookup(*veb, 3) == OptInt{30});
    REQUIRE(lookup(*veb, 4) == std::nullopt);
}

TEMPLATE_TEST_CASE_SIG("empty veb", "[veb][empty]", ((std::size_t W), W), 1, 2,
                       4, 8)
{
    H<W> h;
    const u64 last = universe_size<W>() - 1;

    REQUIRE(!h.veb->min_key().has_value());
    REQUIRE(!h.veb->max_key().has_value());
    REQUIRE(!h.veb->minimum().has_value());
    REQUIRE(!h.veb->maximum().has_value());

    for (u64 x : {u64{0}, last / 2, last}) {
        CAPTURE(W, x);
        REQUIRE(!h.has(x));
        REQUIRE(!h.succ(x).has_value());
        REQUIRE(!h.pred(x).has_value());
        REQUIRE_THROWS(h.veb->at(Bits<W>(x)));
        REQUIRE(!h.erase(x));
    }
    h.check_all();
}

TEMPLATE_TEST_CASE_SIG("single element", "[veb][single]", ((std::size_t W), W),
                       1, 2, 4, 8)
{
    const u64 last = universe_size<W>() - 1;
    const std::vector<u64> keys = {0, last / 2, last};

    for (u64 k : keys) {
        CAPTURE(W, k);
        H<W> h;
        int v = h.put(k);

        REQUIRE(h.has(k));
        REQUIRE(h.get(k) == OptInt{v});
        REQUIRE(h.min() == Opt{k});
        REQUIRE(h.max() == Opt{k});
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{k, v}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{k, v}
        });
        REQUIRE(!h.succ(k).has_value());
        REQUIRE(!h.pred(k).has_value());

        if (k > 0) {
            REQUIRE(h.succ(k - 1) == Opt{k});
            REQUIRE(!h.has(k - 1));
            REQUIRE(!h.get(k - 1).has_value());
        }
        if (k < last) {
            REQUIRE(h.pred(k + 1) == Opt{k});
            REQUIRE(!h.has(k + 1));
            REQUIRE(!h.get(k + 1).has_value());
        }
        h.check_all();

        SECTION("update the value")
        {
            h.put(k, 4242);
            REQUIRE(h.get(k) == OptInt{4242});
            REQUIRE(h.min() == Opt{k});
            REQUIRE(h.max() == Opt{k});
            REQUIRE(h.min_entry() == EntryU{
                                         std::pair<u64, int>{k, 4242}
            });
            REQUIRE(h.max_entry() == EntryU{
                                         std::pair<u64, int>{k, 4242}
            });
            h.check_all();
        }

        SECTION("erase it")
        {
            REQUIRE(h.erase(k));
            REQUIRE(!h.has(k));
            REQUIRE_THROWS(h.veb->at(Bits<W>(k)));
            REQUIRE(!h.min().has_value());
            REQUIRE(!h.max().has_value());
            REQUIRE(!h.veb->minimum().has_value());
            REQUIRE(!h.veb->maximum().has_value());
            h.check_all();

            REQUIRE(!h.erase(k));
            h.check_all();
        }
    }
}

TEST_CASE("veb of width 1", "[veb][w1]")
{
    H<1> h;

    SECTION("only 0")
    {
        h.put(0, 7);
        REQUIRE(h.has(0));
        REQUIRE(!h.has(1));
        REQUIRE(h.get(0) == OptInt{7});
        REQUIRE(!h.get(1).has_value());
        REQUIRE(h.min() == Opt{0});
        REQUIRE(h.max() == Opt{0});
        REQUIRE(!h.succ(0).has_value());
        REQUIRE(!h.succ(1).has_value());
        REQUIRE(!h.pred(0).has_value());
        REQUIRE(h.pred(1) == Opt{0});
        h.check_all();
    }

    SECTION("only 1")
    {
        h.put(1, 8);
        REQUIRE(!h.has(0));
        REQUIRE(h.has(1));
        REQUIRE(h.get(1) == OptInt{8});
        REQUIRE(!h.get(0).has_value());
        REQUIRE(h.min() == Opt{1});
        REQUIRE(h.max() == Opt{1});
        REQUIRE(h.succ(0) == Opt{1});
        REQUIRE(!h.succ(1).has_value());
        REQUIRE(!h.pred(0).has_value());
        REQUIRE(!h.pred(1).has_value());
        h.check_all();
    }

    SECTION("both, inserted 0 then 1")
    {
        h.put(0, 1);
        h.put(1, 2);
        REQUIRE(h.get(0) == OptInt{1});
        REQUIRE(h.get(1) == OptInt{2});
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{0, 1}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{1, 2}
        });
        REQUIRE(h.succ(0) == Opt{1});
        REQUIRE(h.pred(1) == Opt{0});
        h.check_all();
    }

    SECTION("both, inserted 1 then 0")
    {
        h.put(1, 2);
        h.put(0, 1);
        REQUIRE(h.get(0) == OptInt{1});
        REQUIRE(h.get(1) == OptInt{2});
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{0, 1}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{1, 2}
        });
        h.check_all();
    }

    SECTION("update either key of a full tree")
    {
        h.put(0, 1);
        h.put(1, 2);
        h.put(0, 100);
        REQUIRE(h.get(0) == OptInt{100});
        REQUIRE(h.get(1) == OptInt{2});
        h.put(1, 200);
        REQUIRE(h.get(0) == OptInt{100});
        REQUIRE(h.get(1) == OptInt{200});
        h.check_all();
    }

    SECTION("erase from a full tree")
    {
        h.put(0, 1);
        h.put(1, 2);
        REQUIRE(h.erase(0));
        REQUIRE(!h.has(0));
        REQUIRE(h.has(1));
        REQUIRE(h.get(1) == OptInt{2});
        REQUIRE(!h.erase(0));
        h.check_all();
    }

    SECTION("erase the other one")
    {
        h.put(0, 1);
        h.put(1, 2);
        REQUIRE(h.erase(1));
        REQUIRE(h.has(0));
        REQUIRE(!h.has(1));
        REQUIRE(h.get(0) == OptInt{1});
        REQUIRE(!h.erase(1));
        h.check_all();
    }

    SECTION("erase everything then reuse")
    {
        h.put(0, 1);
        h.put(1, 2);
        REQUIRE(h.erase(1));
        REQUIRE(h.erase(0));
        REQUIRE(!h.erase(0));
        REQUIRE(!h.erase(1));
        REQUIRE(!h.min().has_value());
        REQUIRE(!h.max().has_value());
        h.put(1, 9);
        REQUIRE(h.get(1) == OptInt{9});
        REQUIRE(h.min() == Opt{1});
        h.check_all();
    }
}

TEST_CASE("insert and count on Veb<8>", "[veb][count][insert]")
{
    H<8> h;

    SECTION("boundaries")
    {
        h.put(0, 100);
        h.put(255, 200);
        REQUIRE(h.has(0));
        REQUIRE(h.has(255));
        REQUIRE(!h.has(1));
        REQUIRE(!h.has(254));
        REQUIRE(!h.has(128));
        REQUIRE(h.get(0) == OptInt{100});
        REQUIRE(h.get(255) == OptInt{200});
        h.check_all();
    }

    SECTION("powers of two")
    {
        for (u64 p = 1; p < 256; p <<= 1)
            h.put(p);
        h.check_all();
        for (u64 p = 1; p < 256; p <<= 1)
            REQUIRE(h.has(p));
        REQUIRE(!h.has(3));
        REQUIRE(!h.has(0));
    }

    SECTION("elements sharing a cluster")
    {
        for (u64 lo : {0x1, 0x5, 0xA, 0xF})
            h.put(0x30 | lo);
        for (u64 lo = 0; lo < 16; ++lo) {
            bool expected = lo == 0x1 || lo == 0x5 || lo == 0xA || lo == 0xF;
            REQUIRE(h.has(0x30 | lo) == expected);
        }
        REQUIRE(!h.has(0x20 | 0x1));
        REQUIRE(!h.has(0x40 | 0x1));
        h.check_all();
    }

    SECTION("elements in different clusters, same low bits")
    {
        for (u64 hi = 0; hi < 16; hi += 3)
            h.put((hi << 4) | 0x7);
        for (u64 hi = 0; hi < 16; ++hi)
            REQUIRE(h.has((hi << 4) | 0x7) == (hi % 3 == 0));
        h.check_all();
    }

    SECTION("ascending insertion")
    {
        for (u64 x : {u64{1}, u64{2}, u64{17}, u64{33}, u64{100}, u64{200}})
            h.put(x);
        h.check_all();
    }

    SECTION("descending insertion")
    {
        for (u64 x : {u64{200}, u64{100}, u64{33}, u64{17}, u64{2}, u64{1}})
            h.put(x);
        h.check_all();
    }

    SECTION("each key keeps its own value")
    {
        for (u64 x = 0; x < 256; x += 17)
            h.put(x, static_cast<int>(x * 11 + 3));
        for (u64 x = 0; x < 256; ++x) {
            if (x % 17 == 0)
                REQUIRE(h.get(x) == OptInt{static_cast<int>(x * 11 + 3)});
            else
                REQUIRE(!h.get(x).has_value());
        }
    }

    SECTION("negative, zero and large values are stored faithfully")
    {
        h.put(10, -1);
        h.put(20, 0);
        h.put(30, 2147483647);
        h.put(40, -2147483647 - 1);
        REQUIRE(h.get(10) == OptInt{-1});
        REQUIRE(h.get(20) == OptInt{0});
        REQUIRE(h.get(30) == OptInt{2147483647});
        REQUIRE(h.get(40) == OptInt{-2147483647 - 1});
        h.check_all();
    }
}

TEST_CASE("get returns a pointer to the stored value", "[veb][get]")
{
    auto veb = std::make_unique<Veb<8, int>>();

    SECTION("absent keys give nullptr")
    {
        REQUIRE_THROWS(veb->at(Bits<8>(5)));
        veb->insert(5, 50);
        veb->insert(9, 90);
        REQUIRE_THROWS(veb->at(Bits<8>(4)));
        REQUIRE_THROWS(veb->at(Bits<8>(6)));
        REQUIRE_THROWS(veb->at(Bits<8>(0)));
        REQUIRE_THROWS(veb->at(Bits<8>(255)));
    }

    SECTION("present keys give a valid pointer to their value")
    {
        veb->insert(5, 50);
        veb->insert(9, 90);
        veb->insert(200, 2000);

        int p5 = veb->at(Bits<8>(5));
        int p9 = veb->at(Bits<8>(9));
        int p200 = veb->at(Bits<8>(200));
        REQUIRE(p5 == 50);
        REQUIRE(p9 == 90);
        REQUIRE(p200 == 2000);

        REQUIRE(p5 != p9);
        REQUIRE(p9 != p200);
        REQUIRE(p5 != p200);
    }

    SECTION("const overload")
    {
        veb->insert(5, 50);
        veb->insert(9, 90);
        const Veb<8, int>& cv = *veb;

        const int p = cv.at(Bits<8>(9));
        REQUIRE(p == 90);
        REQUIRE_THROWS(cv.at(Bits<8>(10)));

        auto mn = cv.cminimum();
        auto mx = cv.cmaximum();
        REQUIRE(mn.has_value());
        REQUIRE(mx.has_value());
        REQUIRE(mn->first == Bits<8>(5));
        REQUIRE(mx->first == Bits<8>(9));
        REQUIRE(mn->second == 50);
        REQUIRE(mx->second == 90);

        static_assert(std::is_same_v<decltype(mn->second), const int&>);
        static_assert(std::is_same_v<decltype(mx->second), const int&>);
    }

    SECTION("writing through the mutable pointer changes the stored value")
    {
        veb->insert(5, 50); // minimum
        veb->insert(9, 90); // middle
        veb->insert(200, 2000); // maximum

        veb->at(Bits<8>(9)) = 91;
        REQUIRE(lookup(*veb, 9) == OptInt{91});

        veb->at(Bits<8>(5)) = 51;
        REQUIRE(lookup(*veb, 5) == OptInt{51});
        REQUIRE(veb->minimum()->second == 51);

        veb->at(Bits<8>(200)) = 2001;
        REQUIRE(lookup(*veb, 200) == OptInt{2001});
        REQUIRE(veb->maximum()->second == 2001);

        auto mutable_min = veb->minimum();
        auto mutable_max = veb->maximum();
        static_assert(std::is_same_v<decltype(mutable_min->second), int&>);
        static_assert(std::is_same_v<decltype(mutable_max->second), int&>);
        REQUIRE(mutable_min->second == veb->at(Bits<8>(5)));
        REQUIRE(mutable_max->second == veb->at(Bits<8>(200)));

        REQUIRE(val(veb->min_key()) == Opt{5});
        REQUIRE(val(veb->max_key()) == Opt{200});
        REQUIRE(val(veb->successor(Bits<8>(5))) == Opt{9});
        REQUIRE(val(veb->successor(Bits<8>(9))) == Opt{200});
    }

    SECTION("insert on an existing key is visible through get")
    {
        veb->insert(5, 50);
        veb->insert(9, 90);
        veb->insert(9, 91);
        REQUIRE(veb->at(Bits<8>(9)) == 91);
        veb->insert(5, 52);
        REQUIRE(veb->at(Bits<8>(5)) == 52);
    }

    SECTION("get is nullptr again after erase")
    {
        veb->insert(5, 50);
        veb->insert(9, 90);
        veb->insert(200, 2000);

        REQUIRE(veb->erase(Bits<8>(9)));
        REQUIRE_THROWS(veb->at(Bits<8>(9)));
        REQUIRE(veb->erase(Bits<8>(5)));
        REQUIRE_THROWS(veb->at(Bits<8>(5)));
        REQUIRE(veb->at(Bits<8>(200)) == 2000);
        REQUIRE(veb->erase(Bits<8>(200)));
        REQUIRE_THROWS(veb->at(Bits<8>(200)));
    }

    SECTION("get is consistent with count")
    {
        for (u64 k : {u64{0}, u64{1}, u64{15}, u64{16}, u64{128}, u64{255}})
            veb->insert(Bits<8>(k), static_cast<int>(k));
        for (u64 k = 0; k < 256; k += 5) {
            bool present =
                k == 0 || k == 1 || k == 15 || k == 16 || k == 128 || k == 255;
            if (veb->count(Bits<8>(k)))
                REQUIRE_NOTHROW(veb->at(Bits<8>(k)));
            else
                REQUIRE_THROWS(veb->at(Bits<8>(k)));
            REQUIRE(veb->count(Bits<8>(k)) == present);
        }
    }
}

TEST_CASE("inserting an existing key updates its value", "[veb][upsert]")
{
    H<8> h;

    SECTION("single key, repeated updates")
    {
        h.put(10, 1);
        REQUIRE(h.get(10) == OptInt{1});
        h.put(10, 2);
        REQUIRE(h.get(10) == OptInt{2});
        h.put(10, 3);
        REQUIRE(h.get(10) == OptInt{3});

        REQUIRE(h.min() == Opt{10});
        REQUIRE(h.max() == Opt{10});
        REQUIRE(!h.succ(10).has_value());
        REQUIRE(!h.pred(10).has_value());
        REQUIRE(h.succ(9) == Opt{10});
        REQUIRE(h.pred(11) == Opt{10});
        h.check_all();
    }

    SECTION("updating to the same value is harmless")
    {
        h.put(10, 5);
        h.put(10, 5);
        h.put(200, 6);
        h.put(200, 6);
        REQUIRE(h.get(10) == OptInt{5});
        REQUIRE(h.get(200) == OptInt{6});
        h.check_all();
    }

    SECTION("update the minimum")
    {
        h.put_all({10, 20, 30});
        h.put(10, 999);
        REQUIRE(h.get(10) == OptInt{999});
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{10, 999}
        });
        REQUIRE(h.max() == Opt{30});
        h.check_all();
    }

    SECTION("update the maximum")
    {
        h.put_all({10, 20, 30});
        h.put(30, 999);
        REQUIRE(h.get(30) == OptInt{999});
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{30, 999}
        });
        REQUIRE(h.min() == Opt{10});
        h.check_all();
    }

    SECTION("update a middle element")
    {
        h.put_all({10, 20, 30});
        int v10 = *h.get(10);
        int v30 = *h.get(30);
        h.put(20, 999);
        REQUIRE(h.get(20) == OptInt{999});
        REQUIRE(h.get(10) == OptInt{v10});
        REQUIRE(h.get(30) == OptInt{v30});
        REQUIRE(h.succ(10) == Opt{20});
        REQUIRE(h.succ(20) == Opt{30});
        REQUIRE(h.pred(30) == Opt{20});
        h.check_all();
    }

    SECTION("updating never creates extra keys")
    {
        h.put_all({0x05, 0x15, 0x25, 0xF5});
        for (int round = 0; round < 3; ++round)
            for (u64 k : {u64{0x05}, u64{0x15}, u64{0x25}, u64{0xF5}})
                h.put(k);
        h.check_all();

        std::vector<u64> walked;
        for (auto cur = h.min(); cur; cur = h.succ(*cur))
            walked.push_back(*cur);
        REQUIRE(walked == std::vector<u64>{0x05, 0x15, 0x25, 0xF5});
    }

    SECTION("update then insert neighbours")
    {
        h.put(100, 1);
        h.put(100, 2);
        h.put(99, 3);
        h.put(101, 4);
        h.put(100, 5);
        REQUIRE(h.get(99) == OptInt{3});
        REQUIRE(h.get(100) == OptInt{5});
        REQUIRE(h.get(101) == OptInt{4});
        REQUIRE(h.min() == Opt{99});
        REQUIRE(h.max() == Opt{101});
        h.check_all();
    }

    SECTION("update then erase")
    {
        h.put_all({1, 2, 3});
        h.put(2, 77);
        REQUIRE(h.erase(2));
        REQUIRE(!h.has(2));
        REQUIRE(!h.get(2).has_value());
        REQUIRE(!h.erase(2));
        h.check_all();
    }

    SECTION("update the lone element, then erase it")
    {
        h.put(42, 1);
        h.put(42, 2);
        REQUIRE(h.erase(42));
        REQUIRE(!h.min().has_value());
        REQUIRE(!h.max().has_value());
        REQUIRE(!h.has(42));
        REQUIRE(!h.erase(42));
        h.check_all();
    }

    SECTION("erase then reinsert gives the new value")
    {
        h.put(50, 1);
        h.put(60, 2);
        REQUIRE(h.erase(50));
        REQUIRE(!h.get(50).has_value());
        h.put(50, 3);
        REQUIRE(h.get(50) == OptInt{3});
        h.check_all();

        REQUIRE(h.erase(60));
        h.put(60, 4);
        REQUIRE(h.get(60) == OptInt{4});
        h.check_all();
    }
}

TEST_CASE("minimum and maximum track inserts", "[veb][min][max]")
{
    H<8> h;

    h.put(100, 1);
    REQUIRE(h.min_entry() == EntryU{
                                 std::pair<u64, int>{100, 1}
    });
    REQUIRE(h.max_entry() == EntryU{
                                 std::pair<u64, int>{100, 1}
    });

    h.put(150, 2);
    REQUIRE(h.min_entry() == EntryU{
                                 std::pair<u64, int>{100, 1}
    });
    REQUIRE(h.max_entry() == EntryU{
                                 std::pair<u64, int>{150, 2}
    });

    h.put(50, 3);
    REQUIRE(h.min_entry() == EntryU{
                                 std::pair<u64, int>{50, 3}
    });
    REQUIRE(h.max_entry() == EntryU{
                                 std::pair<u64, int>{150, 2}
    });

    h.put(101, 4); // between, must not change anything
    REQUIRE(h.min() == Opt{50});
    REQUIRE(h.max() == Opt{150});

    h.put(0, 5);
    REQUIRE(h.min_entry() == EntryU{
                                 std::pair<u64, int>{0, 5}
    });
    REQUIRE(h.max() == Opt{150});

    h.put(255, 6);
    REQUIRE(h.min() == Opt{0});
    REQUIRE(h.max_entry() == EntryU{
                                 std::pair<u64, int>{255, 6}
    });

    h.put(0, 50);
    h.put(255, 60);
    h.put(101, 40);
    REQUIRE(h.min_entry() == EntryU{
                                 std::pair<u64, int>{0, 50}
    });
    REQUIRE(h.max_entry() == EntryU{
                                 std::pair<u64, int>{255, 60}
    });
    h.check_all();
}

TEST_CASE("minimum and maximum track removals", "[veb][min][max][erase]")
{
    H<8> h;
    h.put_all({0, 10, 20, 30, 200, 255});

    for (u64 x : {u64{0}, u64{255}, u64{10}, u64{200}, u64{30}}) {
        REQUIRE(h.erase(x));
        h.check_min_max();
    }

    REQUIRE(h.min() == Opt{20});
    REQUIRE(h.max() == Opt{20});
    REQUIRE(h.min_entry() == h.max_entry());

    REQUIRE(h.erase(20));
    REQUIRE(!h.min().has_value());
    REQUIRE(!h.max().has_value());
    REQUIRE(!h.min_entry().has_value());
    REQUIRE(!h.max_entry().has_value());
}

TEST_CASE("failed removals do not move minimum or maximum",
          "[veb][min][max][erase]")
{
    H<8> h;
    h.put_all({10, 100, 200});

    REQUIRE(!h.erase(0));
    REQUIRE(!h.erase(255));
    REQUIRE(!h.erase(11));
    REQUIRE(!h.erase(150));

    REQUIRE(h.min() == Opt{10});
    REQUIRE(h.max() == Opt{200});
    h.check_all();
}

TEST_CASE("successor on a sparse Veb<8>", "[veb][successor]")
{
    H<8> h;
    h.put_all({5, 17, 18, 64, 200});

    SECTION("query is a count")
    {
        REQUIRE(h.succ(5) == Opt{17});
        REQUIRE(h.succ(17) == Opt{18});
        REQUIRE(h.succ(18) == Opt{64});
        REQUIRE(h.succ(64) == Opt{200});
        REQUIRE(!h.succ(200).has_value());
    }

    SECTION("query is not a count")
    {
        REQUIRE(h.succ(0) == Opt{5});
        REQUIRE(h.succ(4) == Opt{5});
        REQUIRE(h.succ(6) == Opt{17});
        REQUIRE(h.succ(16) == Opt{17});
        REQUIRE(h.succ(19) == Opt{64});
        REQUIRE(h.succ(63) == Opt{64});
        REQUIRE(h.succ(65) == Opt{200});
        REQUIRE(h.succ(199) == Opt{200});
        REQUIRE(!h.succ(201).has_value());
        REQUIRE(!h.succ(255).has_value());
    }

    SECTION("successor crosses cluster boundaries")
    {
        REQUIRE(h.succ(0x1F) == Opt{64});
        REQUIRE(h.succ(0x20) == Opt{64});
        REQUIRE(h.succ(0x3F) == Opt{64});
    }

    SECTION("updating values does not change successors")
    {
        for (u64 k : {u64{5}, u64{17}, u64{18}, u64{64}, u64{200}})
            h.put(k, 12345);
        REQUIRE(h.succ(5) == Opt{17});
        REQUIRE(h.succ(18) == Opt{64});
        REQUIRE(!h.succ(200).has_value());
        h.check_all();
    }
}

TEST_CASE("predecessor on a sparse Veb<8>", "[veb][predecessor]")
{
    H<8> h;
    h.put_all({5, 17, 18, 64, 200});

    SECTION("query is a count")
    {
        REQUIRE(!h.pred(5).has_value());
        REQUIRE(h.pred(17) == Opt{5});
        REQUIRE(h.pred(18) == Opt{17});
        REQUIRE(h.pred(64) == Opt{18});
        REQUIRE(h.pred(200) == Opt{64});
    }

    SECTION("query is not a count")
    {
        REQUIRE(!h.pred(0).has_value());
        REQUIRE(!h.pred(4).has_value());
        REQUIRE(h.pred(6) == Opt{5});
        REQUIRE(h.pred(16) == Opt{5});
        REQUIRE(h.pred(19) == Opt{18});
        REQUIRE(h.pred(63) == Opt{18});
        REQUIRE(h.pred(65) == Opt{64});
        REQUIRE(h.pred(199) == Opt{64});
        REQUIRE(h.pred(201) == Opt{200});
        REQUIRE(h.pred(255) == Opt{200});
    }

    SECTION("predecessor crosses cluster boundaries")
    {
        REQUIRE(h.pred(0x40) == Opt{18});
        REQUIRE(h.pred(0x30) == Opt{18});
        REQUIRE(h.pred(0x20) == Opt{18});
        REQUIRE(h.pred(0x10) == Opt{5});
    }

    SECTION("updating values does not change predecessors")
    {
        for (u64 k : {u64{5}, u64{17}, u64{18}, u64{64}, u64{200}})
            h.put(k, 54321);
        REQUIRE(!h.pred(5).has_value());
        REQUIRE(h.pred(64) == Opt{18});
        REQUIRE(h.pred(255) == Opt{200});
        h.check_all();
    }
}

TEST_CASE("successor and predecessor use the boundary elements",
          "[veb][successor][predecessor]")
{
    H<8> h;
    h.put_all({0, 255});

    REQUIRE(h.succ(0) == Opt{255});
    REQUIRE(h.succ(1) == Opt{255});
    REQUIRE(h.succ(254) == Opt{255});
    REQUIRE(!h.succ(255).has_value());

    REQUIRE(h.pred(255) == Opt{0});
    REQUIRE(h.pred(254) == Opt{0});
    REQUIRE(h.pred(1) == Opt{0});
    REQUIRE(!h.pred(0).has_value());
}

TEST_CASE("successor and predecessor within a single cluster",
          "[veb][successor][predecessor]")
{
    H<8> h;
    h.put_all({0x50, 0x53, 0x57, 0x5F});

    REQUIRE(h.succ(0x50) == Opt{0x53});
    REQUIRE(h.succ(0x51) == Opt{0x53});
    REQUIRE(h.succ(0x53) == Opt{0x57});
    REQUIRE(h.succ(0x58) == Opt{0x5F});
    REQUIRE(!h.succ(0x5F).has_value());

    REQUIRE(!h.pred(0x50).has_value());
    REQUIRE(h.pred(0x51) == Opt{0x50});
    REQUIRE(h.pred(0x57) == Opt{0x53});
    REQUIRE(h.pred(0x5E) == Opt{0x57});
    REQUIRE(h.pred(0x5F) == Opt{0x57});
}

TEST_CASE("successor walk enumerates all elements and values in order",
          "[veb][successor]")
{
    const std::vector<u64> keys = {3,   9,   10,  31,  32, 33,
                                   100, 128, 129, 250, 255};
    H<8> h;
    h.put_all(keys);

    std::vector<u64> walked;
    for (auto cur = h.min(); cur; cur = h.succ(*cur)) {
        walked.push_back(*cur);
        REQUIRE(h.get(*cur) == OptInt{h.model.at(*cur)});
    }
    REQUIRE(walked == keys);
}

TEST_CASE("predecessor walk enumerates all elements in reverse order",
          "[veb][predecessor]")
{
    const std::vector<u64> keys = {3,   9,   10,  31,  32, 33,
                                   100, 128, 129, 250, 255};
    H<8> h;
    h.put_all(keys);

    std::vector<u64> walked;
    for (auto cur = h.max(); cur; cur = h.pred(*cur))
        walked.push_back(*cur);
    std::reverse(walked.begin(), walked.end());
    REQUIRE(walked == keys);
}

TEST_CASE("erase returns true for existing keys", "[veb][erase]")
{
    H<8> h;
    h.put_all({1, 50, 100, 255});

    REQUIRE(h.erase(50));
    REQUIRE(h.erase(1));
    REQUIRE(h.erase(255));
    REQUIRE(h.erase(100));
    REQUIRE(!h.min().has_value());
    REQUIRE(!h.max().has_value());
}

TEST_CASE("erase returns false for absent keys", "[veb][erase]")
{
    H<8> h;

    SECTION("empty veb")
    {
        REQUIRE(!h.erase(0));
        REQUIRE(!h.erase(42));
        REQUIRE(!h.erase(255));
        h.check_all();
    }

    SECTION("absent keys of a sparse veb")
    {
        h.put_all({5, 17, 18, 64, 200});
        for (u64 x : {u64{0}, u64{4}, u64{6}, u64{16}, u64{19}, u64{63},
                      u64{65}, u64{199}, u64{201}, u64{255}}) {
            CAPTURE(x);
            REQUIRE(!h.erase(x));
        }
        h.check_all();
        REQUIRE(h.model.size() == 5);
    }

    SECTION("absent key in a cluster that has other elements")
    {
        h.put_all({0x31, 0x35, 0x3A});
        REQUIRE(!h.erase(0x30));
        REQUIRE(!h.erase(0x32));
        REQUIRE(!h.erase(0x3F));
        h.check_all();
    }

    SECTION("absent key in an empty cluster between populated ones")
    {
        h.put_all({0x01, 0x91});
        REQUIRE(!h.erase(0x41));
        REQUIRE(!h.erase(0x40));
        REQUIRE(!h.erase(0x8F));
        h.check_all();
    }

    SECTION("single element veb, other key")
    {
        h.put(77);
        REQUIRE(!h.erase(76));
        REQUIRE(!h.erase(78));
        REQUIRE(!h.erase(0));
        REQUIRE(!h.erase(255));
        REQUIRE(h.min() == Opt{77});
        REQUIRE(h.max() == Opt{77});
        REQUIRE(h.has(77));
        h.check_all();
    }

    SECTION("removing the same key twice")
    {
        h.put_all({10, 20, 30});
        REQUIRE(h.erase(20));
        REQUIRE(!h.erase(20));
        REQUIRE(h.erase(10));
        REQUIRE(!h.erase(10));
        REQUIRE(h.erase(30));
        REQUIRE(!h.erase(30));
        h.check_all();
    }

    SECTION("removing a key that was once present, after others changed")
    {
        h.put_all({10, 20, 30});
        REQUIRE(h.erase(10));
        REQUIRE(!h.erase(10));
        h.put(5);
        REQUIRE(!h.erase(10));
        h.put(10);
        REQUIRE(h.erase(10));
        h.check_all();
    }
}

TEST_CASE("erase the only element", "[veb][erase]")
{
    H<8> h;
    h.put(77);
    REQUIRE(h.erase(77));

    REQUIRE(!h.has(77));
    REQUIRE(!h.get(77).has_value());
    REQUIRE(!h.min().has_value());
    REQUIRE(!h.max().has_value());
    REQUIRE(!h.succ(0).has_value());
    REQUIRE(!h.pred(255).has_value());
    REQUIRE(!h.erase(77));
}

TEST_CASE("erase from a two element tree", "[veb][erase]")
{
    H<8> h;

    SECTION("erase the min")
    {
        h.put(10, 1);
        h.put(20, 2);
        REQUIRE(h.erase(10));
        REQUIRE(!h.has(10));
        REQUIRE(h.has(20));
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{20, 2}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{20, 2}
        });
        h.check_all();
    }

    SECTION("erase the max")
    {
        h.put(10, 1);
        h.put(20, 2);
        REQUIRE(h.erase(20));
        REQUIRE(h.has(10));
        REQUIRE(!h.has(20));
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{10, 1}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{10, 1}
        });
        h.check_all();
    }

    SECTION("same cluster, erase the min")
    {
        h.put(0x21, 1);
        h.put(0x2E, 2);
        REQUIRE(h.erase(0x21));
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{0x2E, 2}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{0x2E, 2}
        });
        REQUIRE(h.has(0x2E));
        REQUIRE(!h.has(0x21));
        h.check_all();
    }

    SECTION("same cluster, erase the max")
    {
        h.put(0x21, 1);
        h.put(0x2E, 2);
        REQUIRE(h.erase(0x2E));
        REQUIRE(h.min_entry() == EntryU{
                                     std::pair<u64, int>{0x21, 1}
        });
        REQUIRE(h.max_entry() == EntryU{
                                     std::pair<u64, int>{0x21, 1}
        });
        REQUIRE(h.has(0x21));
        REQUIRE(!h.has(0x2E));
        h.check_all();
    }
}

TEST_CASE("erase min, max and middle elements of Veb<8>", "[veb][erase]")
{
    const std::vector<u64> keys = {1, 2, 17, 18, 33, 100, 101, 200, 254};
    H<8> h;
    h.put_all(keys);

    SECTION("erase the minimum repeatedly")
    {
        while (!h.model.empty()) {
            u64 m = h.model.begin()->first;
            REQUIRE(h.erase(m));
            h.check_all();
        }
    }

    SECTION("erase the maximum repeatedly")
    {
        while (!h.model.empty()) {
            u64 m = h.model.rbegin()->first;
            REQUIRE(h.erase(m));
            h.check_all();
        }
    }

    SECTION("erase middle elements, keeping min and max")
    {
        for (u64 x : {u64{33}, u64{18}, u64{100}, u64{2}, u64{200}}) {
            REQUIRE(h.erase(x));
            h.check_all();
        }
        REQUIRE(h.min() == Opt{1});
        REQUIRE(h.max() == Opt{254});
    }

    SECTION("alternate min and max")
    {
        bool take_min = true;
        while (!h.model.empty()) {
            u64 x = take_min ? h.model.begin()->first : h.model.rbegin()->first;
            REQUIRE(h.erase(x));
            h.check_all();
            take_min = !take_min;
        }
    }

    SECTION("shuffled order")
    {
        std::vector<u64> order = keys;
        std::mt19937 rng(12345);
        std::shuffle(order.begin(), order.end(), rng);
        for (u64 x : order) {
            REQUIRE(h.erase(x));
            h.check_all();
        }
        REQUIRE(!h.min().has_value());
    }
}

TEST_CASE("erase keeps values of the remaining keys", "[veb][erase]")
{
    H<8> h;
    for (u64 x = 0; x < 256; x += 21)
        h.put(x, static_cast<int>(x + 1000));

    for (u64 x = 0; x < 256; x += 42) {
        REQUIRE(h.erase(x));
        for (u64 y = 0; y < 256; y += 21) {
            if (h.model.count(y))
                REQUIRE(h.get(y) == OptInt{static_cast<int>(y + 1000)});
            else
                REQUIRE(!h.get(y).has_value());
        }
    }
    h.check_all();
}

TEST_CASE("erase empties a cluster", "[veb][erase]")
{
    H<8> h;
    h.put_all({0x05, 0x30, 0x50, 0x51, 0x5F, 0xF0});

    REQUIRE(h.erase(0x30));
    h.check_all();
    REQUIRE(h.succ(0x05) == Opt{0x50});
    REQUIRE(h.pred(0x50) == Opt{0x05});

    for (u64 x : {u64{0x51}, u64{0x50}, u64{0x5F}}) {
        REQUIRE(h.erase(x));
        h.check_all();
    }
    REQUIRE(h.succ(0x05) == Opt{0xF0});
    REQUIRE(h.pred(0xF0) == Opt{0x05});
}

TEST_CASE("erase updates max when max cluster becomes empty",
          "[veb][erase][max]")
{
    H<8> h;
    h.put_all({0x01, 0x40, 0x80, 0xC0});

    REQUIRE(h.erase(0xC0));
    REQUIRE(h.max() == Opt{0x80});
    REQUIRE(h.erase(0x80));
    REQUIRE(h.max() == Opt{0x40});
    REQUIRE(h.erase(0x40));
    REQUIRE(h.max() == Opt{0x01});
    REQUIRE(h.min() == Opt{0x01});
    h.check_all();
}

TEST_CASE("erase updates min when min cluster becomes empty",
          "[veb][erase][min]")
{
    H<8> h;
    h.put_all({0x01, 0x40, 0x80, 0xC0});

    REQUIRE(h.erase(0x01));
    REQUIRE(h.min() == Opt{0x40});
    REQUIRE(h.erase(0x40));
    REQUIRE(h.min() == Opt{0x80});
    REQUIRE(h.erase(0x80));
    REQUIRE(h.min() == Opt{0xC0});
    REQUIRE(h.max() == Opt{0xC0});
    h.check_all();
}

TEST_CASE("reinsert after erase", "[veb][erase][insert]")
{
    H<8> h;
    h.put_all({10, 20, 30});

    REQUIRE(h.erase(20));
    h.check_all();
    h.put(20);
    h.check_all();

    REQUIRE(h.erase(10));
    h.put(10);
    h.check_all();

    REQUIRE(h.erase(30));
    h.put(30);
    h.check_all();

    REQUIRE(h.min() == Opt{10});
    REQUIRE(h.max() == Opt{30});
}

TEST_CASE("reuse after emptying", "[veb][erase][insert]")
{
    H<8> h;
    h.put_all({1, 2, 3, 100, 200});

    for (u64 x : {u64{1}, u64{2}, u64{3}, u64{100}, u64{200}})
        REQUIRE(h.erase(x));

    REQUIRE(!h.min().has_value());
    REQUIRE(!h.max().has_value());
    for (u64 x : {u64{1}, u64{2}, u64{3}, u64{100}, u64{200}}) {
        REQUIRE(!h.has(x));
        REQUIRE(!h.get(x).has_value());
    }

    h.put_all({7, 250, 128});
    h.check_all();
}

TEST_CASE("veb with a non-trivial value type", "[veb][string]")
{
    auto veb = std::make_unique<Veb<8, std::string>>();
    using OptStr = std::optional<std::string>;

    veb->insert(3, "three");
    veb->insert(200, "two hundred");
    veb->insert(77, "seventy-seven");

    REQUIRE(lookup(*veb, 3) == OptStr{"three"});
    REQUIRE(lookup(*veb, 200) == OptStr{"two hundred"});
    REQUIRE(lookup(*veb, 77) == OptStr{"seventy-seven"});
    REQUIRE_THROWS(veb->at(Bits<8>(4)));

    veb->insert(3, "THREE");
    REQUIRE(lookup(*veb, 3) == OptStr{"THREE"});
    REQUIRE(lookup(*veb, 77) == OptStr{"seventy-seven"});

    veb->insert(77, "");
    REQUIRE(veb->count(77));
    REQUIRE(veb->at(Bits<8>(77)) == "");

    veb->at(Bits<8>(200)).append("!");
    REQUIRE(lookup(*veb, 200) == OptStr{"two hundred!"});

    REQUIRE(veb->minimum()->first.to_ullong() == 3);
    REQUIRE(veb->minimum()->second == "THREE");
    REQUIRE(veb->maximum()->first.to_ullong() == 200);
    REQUIRE(veb->maximum()->second == "two hundred!");

    REQUIRE(val(veb->successor(Bits<8>(3))) == Opt{77});
    REQUIRE(val(veb->successor(Bits<8>(77))) == Opt{200});

    REQUIRE(veb->erase(Bits<8>(77)));
    REQUIRE(!veb->erase(Bits<8>(77)));
    REQUIRE(!veb->count(77));
    REQUIRE_THROWS(veb->at(Bits<8>(77)));
    REQUIRE(val(veb->successor(Bits<8>(3))) == Opt{200});

    REQUIRE(veb->erase(Bits<8>(3)));
    REQUIRE(veb->erase(Bits<8>(200)));
    REQUIRE(!veb->min_key().has_value());
    REQUIRE(!veb->max_key().has_value());
}

TEMPLATE_TEST_CASE_SIG("short random upsert/erase sequence vs std::map",
                       "[veb][random]", ((std::size_t W), W), 4, 8)
{
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<u64> key_dist(0, universe_size<W>() - 1);
    std::uniform_int_distribution<int> coin(0, 99);

    H<W> h;

    for (int i = 0; i < 300; ++i) {
        u64 k = key_dist(rng);
        CAPTURE(i, k);

        if (coin(rng) < 55)
            h.put(k);
        else
            h.erase(k);

        h.check_all();
    }
}
