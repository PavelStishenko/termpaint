// SPDX-License-Identifier: BSL-1.0
#include "termpaint.h"

#include <codecvt>
#include <locale>
#include <numeric>

#ifndef BUNDLED_CATCH2
#include "catch2/catch.hpp"
#else
#include "../third-party/catch.hpp"
#endif


// NOTE: This file assumes that the compiler uses utf-8 for string constants.

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(name) 0
#endif

#if __has_cpp_attribute(maybe_unused) && __cplusplus >= 201703L
#define MAYBE_UNUSED [[maybe_unused]]
#elif __has_cpp_attribute(gnu::unused)
#define MAYBE_UNUSED [[gnu::unused]]
#else
#define MAYBE_UNUSED
#endif

// sanitizer builds report spurious failures with codecvt and libstdc++, work around using iconv
#ifndef __GLIBCXX__
std::u16string toUtf16(std::string data) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(data);
}

std::u32string toUtf32(std::string data) {
    return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>{}.from_bytes(data);
}

std::u32string toUtf32(std::u16string data) {
    return std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t>{}.from_bytes(
    reinterpret_cast<const char*>(data.c_str()), reinterpret_cast<const char*>(data.c_str() + data.size()));
}
#else
#include <iconv.h>

class iconv_cache { // keep one context to avoid repeatedly loading/unloading in glibc
public:
    iconv_cache(const char *tocode, const char *fromcode) {
        cache = iconv_open(tocode, fromcode);
    }

    ~iconv_cache() {
        iconv_close(cache);
    }
private:
    iconv_t cache;
};

std::u16string toUtf16(std::string data) {
    static auto cached = iconv_cache("UTF-16LE", "UTF-8");
    static auto cachedBE = iconv_cache("UTF-16BE", "UTF-8");
    iconv_t conv;
    const char16_t endiannessTest = 1;
    if (*((const char*)&endiannessTest) == 1) {
        conv = iconv_open("UTF-16LE", "UTF-8");
    } else {
        conv = iconv_open("UTF-16BE", "UTF-8");
    }
    REQUIRE( conv != reinterpret_cast<iconv_t>(-1));
    const char *pin = data.data();
    std::vector<char16_t> outbuffer;
    outbuffer.resize(2 + data.size()); // zero termination plus BOM
    char *pout = reinterpret_cast<char*>(outbuffer.data());
    size_t inlen = data.size() + 1;
    size_t outlen = outbuffer.size() * sizeof (char16_t);
    size_t retval = iconv(conv, const_cast<char**>(&pin), &inlen, &pout, &outlen);
    REQUIRE(retval != static_cast<size_t>(-1));
    iconv_close(conv);
    if (outlen > 0 && outbuffer[0] == 0xFEFF /* BOM */) {
        return std::u16string { outbuffer.data() + 1 };
    } else {
        return std::u16string { outbuffer.data()};
    }
}

std::u32string toUtf32(std::string data) {
    static auto cached = iconv_cache("UTF-32LE", "UTF-8"); // keep one context to avoid repeatedly loading/unloading in glibc
    static auto cachedBe = iconv_cache("UTF-32BE", "UTF-8"); // keep one context to avoid repeatedly loading/unloading in glibc
    iconv_t conv;
    const char32_t endiannessTest = 1;
    if (*((const char*)&endiannessTest) == 1) {
        conv = iconv_open("UTF-32LE", "UTF-8");
    } else {
        conv = iconv_open("UTF-32BE", "UTF-8");
    }
    REQUIRE( conv != reinterpret_cast<iconv_t>(-1));
    const char *pin = data.data();
    std::vector<char32_t> outbuffer;
    outbuffer.resize(2 + data.size()); // zero termination plus BOM
    char *pout = reinterpret_cast<char*>(outbuffer.data());
    size_t inlen = data.size() + 1;
    size_t outlen = outbuffer.size() * sizeof (char32_t);
    size_t retval = iconv(conv, const_cast<char**>(&pin), &inlen, &pout, &outlen);
    REQUIRE(retval != static_cast<size_t>(-1));
    iconv_close(conv);
    if (outlen > 0 && outbuffer[0] == 0xFEFF /* BOM */) {
        return std::u32string { outbuffer.data() + 1 };
    } else {
        return std::u32string { outbuffer.data() };
    }
}

std::u32string toUtf32(std::u16string data) {
    static auto cached = iconv_cache("UTF-32LE", "UTF-8"); // keep one context to avoid repeatedly loading/unloading in glibc
    static auto cachedBe = iconv_cache("UTF-32BE", "UTF-8"); // keep one context to avoid repeatedly loading/unloading in glibc
    iconv_t conv;
    const char32_t endiannessTest = 1;
    if (*((const char*)&endiannessTest) == 1) {
        conv = iconv_open("UTF-32LE", "UTF-16LE");
    } else {
        conv = iconv_open("UTF-32BE", "UTF-16BE");
    }
    REQUIRE( conv != reinterpret_cast<iconv_t>(-1));
    const char *pin = reinterpret_cast<const char*>(data.data());
    std::vector<char32_t> outbuffer;
    outbuffer.resize(2 + data.size()); // zero termination plus BOM
    char *pout = reinterpret_cast<char*>(outbuffer.data());
    size_t inlen = (data.size() + 1) * 2;
    size_t outlen = outbuffer.size() * sizeof (char32_t);
    size_t retval = iconv(conv, const_cast<char**>(&pin), &inlen, &pout, &outlen);
    REQUIRE(retval != static_cast<size_t>(-1));
    iconv_close(conv);
    if (outlen > 0 && outbuffer[0] == 0xFEFF /* BOM */) {
        return std::u32string { outbuffer.data() + 1 };
    } else {
        return std::u32string { outbuffer.data() };
    }
}
#endif


template <typename T>
static int toInt(T x);

static int toInt(size_t x) {
    if (x > std::numeric_limits<int>::max()) {
        throw std::runtime_error("out of range in conversion to int");
    }
    return static_cast<int>(x);
}

template <typename T>
static unsigned toUInt(T x) {
    const unsigned result = static_cast<int>(x);
    if (result != x) {
        throw std::runtime_error("out of range in conversion to int");
    }
    return x;
}

template <typename STR>
class PartitionGenerator : public Catch::Generators::IGenerator<std::vector<STR>> {
public:
    PartitionGenerator(STR str) : str(str) {
        lengths.push_back(toInt(str.length()));
        refreshCache();
    }

    const std::vector<STR>& get() const override {
        return resultCache;
    }

    bool next() override {
        if (std::all_of(begin(lengths), end(lengths),
                        [](int x) {return x == 1;} )) {
            return false;
        }

        unsigned i;
        for (i = toUInt(lengths.size() - 1); i > 0; i--) {
            if (lengths[i] != 1) break;
        }

        if (i == 0 && lengths[i] == 1) return false;

        lengths.resize(i + 1);
        lengths[i] -= 1;
        lengths.push_back(str.length() - std::accumulate(begin(lengths), begin(lengths) + i + 1, 0));

        refreshCache();
        return true;
    }

private:
    void refreshCache() {
        resultCache.clear();
        int current = 0;
        for (int len: lengths) {
            resultCache.push_back(str.substr(current, len));
            current += len;
        }
    }

private:
    STR str;
    std::vector<int> lengths;
    std::vector<STR> resultCache;
};

template <typename STR>
Catch::Generators::GeneratorWrapper<std::vector<STR>> partitions(STR str) {
    return Catch::Generators::GeneratorWrapper<std::vector<STR>>(
        std::unique_ptr<Catch::Generators::IGenerator<std::vector<STR>>>(new PartitionGenerator<STR>(str)));
}

TEST_CASE("test for PartitionGenerator", "[measurement]") {
    struct TestCase { std::string str; std::vector<std::vector<std::string>> splits; };
    const auto testCase = GENERATE(
             TestCase{"a", {{"a"}} },
             TestCase{"ab", {{"ab"}, {"a", "b"}} },
             TestCase{"abc", {{"abc"}, {"ab", "c"}, {"a", "bc"}, {"a", "b", "c"}} },
             TestCase{"abcd", {{"abcd"}, {"abc", "d"}, {"ab", "cd"}, {"ab", "c", "d"},
                               {"a", "bcd"}, {"a", "bc", "d"}, {"a", "b", "cd"}, {"a", "b", "c", "d"}} }
    );
    PartitionGenerator<std::string> gen(testCase.str);
    unsigned int idx = 0;
    do {
        REQUIRE(idx < testCase.splits.size());
        REQUIRE(gen.get() == testCase.splits[idx]);
        idx += 1;
    } while (gen.next());
    REQUIRE(idx == testCase.splits.size());
}

static std::string char_to_string(char ch) {
    auto uch = static_cast<unsigned char>(ch);
    return std::to_string(uch);
}

static std::string char_to_string(char16_t ch) {
    return std::to_string(ch);
}

static std::string char_to_string(char32_t ch) {
    return std::to_string(ch);
}

template <typename STR>
static std::string printPartition(const std::vector<STR> &value) {
    std::string ret = "[";
    for (const auto& part: value) {
        if (ret.size() > 1) {
            ret += ", ";
        }
        std::string partInNumbers;
        for (auto ch: part) {
            if (partInNumbers.size()) {
                partInNumbers += ", ";
            }
            partInNumbers += char_to_string(ch);
        }
        ret += "(" + partInNumbers + ")";
    }
    ret += "]";
    return ret;
}

struct Result {
    int codeunits;
    int codepoints;
    int columns;
    int clusters;
    bool limitReached;
};

class MeasurementWrapper {
public:
    MeasurementWrapper() {
        auto free = [](termpaint_integration*){};
        auto flush = [](termpaint_integration*){};
        auto write = [](termpaint_integration*, const char*, int){};
        termpaint_integration_init(&integration, free, write, flush);
        terminal = termpaint_terminal_new(&integration);
        measurement = termpaint_text_measurement_new(termpaint_terminal_get_surface(terminal));
    }
    ~MeasurementWrapper() {
        termpaint_text_measurement_free(measurement);
        termpaint_terminal_free(terminal);
        termpaint_integration_deinit(&integration);
    }

    termpaint_text_measurement* get() {
        return measurement;
    }

    termpaint_text_measurement* operator->() {
        return measurement;
    }

    termpaint_integration integration;
    termpaint_terminal *terminal;
    termpaint_text_measurement* measurement;
};

Result measureOneCluster(const std::vector<std::string>& partition) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_clusters(tm.get(), 1);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf8(tm.get(),
                                                                   part.data(),
                                                                   toInt(part.length()),
                                                                   last);
        if (!last) {
            REQUIRE(!result.limitReached);
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}

Result measureOneCluster(const std::vector<std::u16string>& partition) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_clusters(tm.get(), 1);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::u16string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf16(tm.get(),
                                                                    reinterpret_cast<const uint16_t*>(part.data()),
                                                                    toInt(part.length()),
                                                                    last);
        if (!last) {
            REQUIRE(!result.limitReached);
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}

Result measureOneCluster(const std::vector<std::u32string>& partition) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_clusters(tm.get(), 1);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::u32string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf32(tm.get(),
                                                                    reinterpret_cast<const uint32_t*>(part.data()),
                                                                    toInt(part.length()),
                                                                    last);
        if (!last) {
            REQUIRE(!result.limitReached);
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}

TEST_CASE( "Measurements for single clusters", "[measurement]") {
    Result result;
    struct TestCase { const std::string str; int columns; std::string desc; };
    const auto testCase = GENERATE(
        TestCase{"A",                 1, "plain latin letter"},
        TestCase{"が",                2, "plain hiragana"},
        TestCase{"\xcc\x88",          1, "isolated U+0308 combining diaeresis"},
        TestCase{"a\xcc\x88",         1, "'a' + U+0308 combining diaeresis"},
        TestCase{"a\xcc\x88\xcc\xa4", 1, "'a' + U+0308 combining diaeresis + U+0324 combining diaeresis below"},
        TestCase{"a\xf3\xa0\x84\x80\xf3\xa0\x84\x81", 1, "'a' + U+E0100 variation selector-17 + U+E0101 variation selector-18 (nonsense)"},
        TestCase{"\x7f",              1, "erase marker"}
    );
    std::u32string utf32 = toUtf32(testCase.str);
    SECTION("parse as utf8") {
        auto partition = GENERATE_COPY(partitions(testCase.str));
        INFO(testCase.desc);
        INFO("Partition: " << printPartition(partition));
        INFO("Checking for string " << testCase.str)
        result = measureOneCluster(partition);
        CHECK(result.limitReached == true);
        CHECK(result.columns == testCase.columns);
        CHECK(result.codeunits == toInt(testCase.str.length()));
        CHECK(result.codepoints == toInt(utf32.length()));
    }
    SECTION("parse as utf16") {
        INFO(testCase.desc);
        INFO("Checking for string " << testCase.str)
        std::u16string utf16 = toUtf16(testCase.str);
        auto partition = GENERATE_COPY(partitions(utf16));
        INFO("Partition: " << printPartition(partition));
        result = measureOneCluster(partition);
        CHECK(result.limitReached == true);
        CHECK(result.columns == testCase.columns);
        CHECK(result.codeunits == toInt(utf16.length()));
        CHECK(result.codepoints == toInt(utf32.length()));
    }
    SECTION("parse as utf32") {
        INFO(testCase.desc);
        INFO("Checking for string " << testCase.str)
        auto partition = GENERATE_COPY(partitions(utf32));
        INFO("Partition: " << printPartition(partition));
        result = measureOneCluster(partition);
        CHECK(result.limitReached == true);
        CHECK(result.columns == testCase.columns);
        CHECK(result.codeunits == toInt(utf32.length()));
        CHECK(result.codepoints == toInt(utf32.length()));
    }
}

Result measureTest(const std::vector<std::string>& partition, int limCodepoints,
                   int limClusters, int limWidth, int limCodeunits) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_codepoints(tm.get(), limCodepoints);
    termpaint_text_measurement_set_limit_clusters(tm.get(), limClusters);
    termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
    termpaint_text_measurement_set_limit_ref(tm.get(), limCodeunits);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf8(tm.get(),
                                                                   part.data(),
                                                                   toInt(part.length()),
                                                                   last);

        if (result.limitReached) {
            break;
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}

Result measureTest(const std::vector<std::u16string>& partition, int limCodepoints,
                   int limClusters, int limWidth, int limCodeunits) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_codepoints(tm.get(), limCodepoints);
    termpaint_text_measurement_set_limit_clusters(tm.get(), limClusters);
    termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
    termpaint_text_measurement_set_limit_ref(tm.get(), limCodeunits);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::u16string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf16(tm.get(),
                                                                   reinterpret_cast<const uint16_t*>(part.data()),
                                                                   toInt(part.length()),
                                                                   last);

        if (result.limitReached) {
            break;
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}

Result measureTest(const std::vector<std::u32string>& partition, int limCodepoints,
                   int limClusters, int limWidth, int limCodeunits) {
    MeasurementWrapper tm;
    termpaint_text_measurement_set_limit_codepoints(tm.get(), limCodepoints);
    termpaint_text_measurement_set_limit_clusters(tm.get(), limClusters);
    termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
    termpaint_text_measurement_set_limit_ref(tm.get(), limCodeunits);
    Result result;
    for (unsigned i = 0; i < partition.size(); i++) {
        const std::u32string &part = partition[i];
        const bool last = i == partition.size() - 1;
        result.limitReached = termpaint_text_measurement_feed_utf32(tm.get(),
                                                                   reinterpret_cast<const uint32_t*>(part.data()),
                                                                   toInt(part.length()),
                                                                   last);

        if (result.limitReached) {
            break;
        }
    }
    result.codepoints = termpaint_text_measurement_last_codepoints(tm.get());
    result.codeunits = termpaint_text_measurement_last_ref(tm.get());
    result.columns = termpaint_text_measurement_last_width(tm.get());
    result.clusters = termpaint_text_measurement_last_clusters(tm.get());
    return result;
}


struct ExpectedMeasures {
    int codeunits = 0;
    int width = 0;
    int codepoints = 0;
    int clusters = 0;

    template<typename C>
    void addCluster(C cluster) {
        codeunits += cluster.str.size();
        width += cluster.columns;
        codepoints += toUtf32(cluster.str).size();
        ++clusters;
    }

    template<typename C>
    void addClusterUtf16(C cluster) {
        codeunits += toUtf16(cluster.str).size();
        width += cluster.columns;
        codepoints += toUtf32(cluster.str).size();
        ++clusters;
    }

    template<typename C>
    void addClusterUtf32(C cluster) {
        int sizeInUtf32 = toUtf32(cluster.str).size();
        codeunits += sizeInUtf32;
        codepoints += sizeInUtf32;
        width += cluster.columns;
        ++clusters;
    }

};

TEST_CASE( "Measurements for strings", "[measurement]") {
    struct C { const std::string str; int columns; };
    struct TestCase { const std::vector<C> data; std::string desc; };
    const auto testCase = GENERATE(
                TestCase{ {C{"A", 1}, C{"b", 1}, C{"c", 1}, C{"d", 1} }, "Latin Abcde"},
                TestCase{ {C{"A", 1}, C{"b\xcc\x88", 1}, C{"c", 1}, C{"d", 1} }, "Latin Abcde with U+0308 combining diaeresis after b"},
                TestCase{ {C{"A", 1}, C{"b", 1}, C{"c\xcc\x88\xcc\xa4", 1}, C{"d", 1} }, "Latin Abcde with U+0308 combining diaeresis + U+0324 combining diaeresis below after c"},
                TestCase{ {C{"\xcc\x88", 1} } , "isolated U+0308 combining diaeresis"},
                TestCase{ {C{"A", 1}, C{"が", 2}, C{"c", 1}, C{"d", 1} }, "Latin A followed by plain hiragana and latin cde"},
                TestCase{ {C{"A", 1}, C{"\xF0\x9B\x80\x80", 2}, C{"d", 1} }, "Latin A followed by U+1B000 katakana letter archaic e and latin cde"},
                TestCase{ {C{"A", 1}, C{"\xF0\x9F\x8D\x92", 2}, C{"d", 1} }, "Latin A followed by U+1F352 cherries and latin cde"},
                TestCase{ {C{"\x7f", 1}, C{"b", 1} }, "erase marker plus b"},
                TestCase{ {C{"a", 1}, C{"\x7f", 1}, C{"b", 1} }, "a plus erase marker plus b"},
                TestCase{ {C{"\x7f", 1}, C{"\xcc\x88", 1} } , "erase marker plus U+0308 combining diaeresis"}
    );
    INFO(testCase.desc);
    SECTION("utf8 - codeunits") {
        std::string all;
        for (const C& cluster: testCase.data) {
            all += cluster.str;
        }
        const int size = toInt(all.size());
        const int len = GENERATE_COPY(range(0, size));
        auto partition = GENERATE_COPY(partitions(all));
        INFO("len: " << len);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codeunits + toInt(cluster.str.size()) > len) break;
            expected.addCluster(cluster);
        }

        Result result = measureTest(partition, -1, -1, -1, len);
        CHECK(result.columns == expected.width);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf8 - codepoints") {
        std::string all;
        for (const C& cluster: testCase.data) {
            all += cluster.str;
        }
        int maxCodepoints = toUtf32(all).size();
        const int codepointsLimit = GENERATE_COPY(range(0, maxCodepoints));
        auto partition = GENERATE_COPY(partitions(all));
        INFO("codepointsLimit: " << codepointsLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codepoints + toInt(toUtf32(cluster.str).size()) > codepointsLimit) break;
            expected.addCluster(cluster);
        }

        Result result = measureTest(partition, codepointsLimit, -1, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf8 - width") {
        std::string all;
        int maxWidth = 0;
        for (const C& cluster: testCase.data) {
            all += cluster.str;
            maxWidth += cluster.columns;
        }
        const int widthLimit = GENERATE_COPY(range(0, maxWidth));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(widthLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.width + cluster.columns > widthLimit) break;
            expected.addCluster(cluster);
        }

        Result result = measureTest(partition, -1, -1, widthLimit, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf8 - clusters") {
        std::string all;
        int maxClusters = 0;
        for (const C& cluster: testCase.data) {
            all += cluster.str;
            ++maxClusters;
        }
        const int clusterLimit = GENERATE_COPY(range(0, maxClusters));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(clusterLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.clusters + 1 > clusterLimit) break;
            expected.addCluster(cluster);
        }

        Result result = measureTest(partition, -1, clusterLimit, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    // ----------------------
    SECTION("utf16 - codeunits") {
        std::u16string all;
        for (const C& cluster: testCase.data) {
            all += toUtf16(cluster.str);
        }
        const int size = toInt(all.size());
        const int len = GENERATE_COPY(range(0, size));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(len);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codeunits + toInt(toUtf16(cluster.str).size()) > len) break;
            expected.addClusterUtf16(cluster);
        }

        Result result = measureTest(partition, -1, -1, -1, len);
        CHECK(result.columns == expected.width);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf16 - codepoints") {
        std::u16string all;
        for (const C& cluster: testCase.data) {
            all += toUtf16(cluster.str);
        }
        int maxCodepoints = toUtf32(all).size();
        const int codepointsLimit = GENERATE_COPY(range(0, maxCodepoints));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(codepointsLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codepoints + toInt(toUtf32(cluster.str).size()) > codepointsLimit) break;
            expected.addClusterUtf16(cluster);
        }

        Result result = measureTest(partition, codepointsLimit, -1, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf16 - width") {
        std::u16string all;
        int maxWidth = 0;
        for (const C& cluster: testCase.data) {
            all += toUtf16(cluster.str);
            maxWidth += cluster.columns;
        }
        const int widthLimit = GENERATE_COPY(range(0, maxWidth));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(widthLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.width + cluster.columns > widthLimit) break;
            expected.addClusterUtf16(cluster);
        }

        Result result = measureTest(partition, -1, -1, widthLimit, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf16 - clusters") {
        std::u16string all;
        int maxClusters = 0;
        for (const C& cluster: testCase.data) {
            all += toUtf16(cluster.str);
            ++maxClusters;
        }
        const int clusterLimit = GENERATE_COPY(range(0, maxClusters));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(clusterLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.clusters + 1 > clusterLimit) break;
            expected.addClusterUtf16(cluster);
        }

        Result result = measureTest(partition, -1, clusterLimit, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    // ----------------------
    SECTION("utf32 - codeunits") {
        std::u32string all;
        for (const C& cluster: testCase.data) {
            all += toUtf32(cluster.str);
        }
        const int size = toInt(all.size());
        const int len = GENERATE_COPY(range(0, size));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(len);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codeunits + toInt(toUtf32(cluster.str).size()) > len) break;
            expected.addClusterUtf32(cluster);
        }

        Result result = measureTest(partition, -1, -1, -1, len);
        CHECK(result.columns == expected.width);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf32 - codepoints") {
        std::u32string all;
        for (const C& cluster: testCase.data) {
            all += toUtf32(cluster.str);
        }
        int maxCodepoints = all.size();
        const int codepointsLimit = GENERATE_COPY(range(0, maxCodepoints));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(codepointsLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.codepoints + toInt(toUtf32(cluster.str).size()) > codepointsLimit) break;
            expected.addClusterUtf32(cluster);
        }

        Result result = measureTest(partition, codepointsLimit, -1, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf32 - width") {
        std::u32string all;
        int maxWidth = 0;
        for (const C& cluster: testCase.data) {
            all += toUtf32(cluster.str);
            maxWidth += cluster.columns;
        }
        const int widthLimit = GENERATE_COPY(range(0, maxWidth));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(widthLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.width + cluster.columns > widthLimit) break;
            expected.addClusterUtf32(cluster);
        }

        Result result = measureTest(partition, -1, -1, widthLimit, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }
    SECTION("utf32 - clusters") {
        std::u32string all;
        int maxClusters = 0;
        for (const C& cluster: testCase.data) {
            all += toUtf32(cluster.str);
            ++maxClusters;
        }
        const int clusterLimit = GENERATE_COPY(range(0, maxClusters));
        auto partition = GENERATE_COPY(partitions(all));
        CAPTURE(clusterLimit);
        INFO("Partition: " << printPartition(partition));

        ExpectedMeasures expected;
        for (const C& cluster: testCase.data) {
            if (expected.clusters + 1 > clusterLimit) break;
            expected.addClusterUtf32(cluster);
        }

        Result result = measureTest(partition, -1, clusterLimit, -1, -1);
        CHECK(result.codeunits == expected.codeunits);
        CHECK(result.columns == expected.width);
        CHECK(result.codepoints == expected.codepoints);
        CHECK(result.clusters == expected.clusters);
    }

}

TEST_CASE( "Continue measurements for strings", "[measurement]") {
    struct S { const std::string str; int columns; };
    struct TestCase { std::vector<S> segs; std::string desc; };
    const auto testCase = GENERATE(
                TestCase{ { {"Ab", 2}, {"c", 1}, {"de", 2} }, "Latin Abcde"}
    );
    INFO(testCase.desc);
    SECTION("utf8 - width") {
        std::string all;
        for (const S& segment: testCase.segs) {
            all += segment.str;
        }

        MeasurementWrapper tm;
        int limWidth = 0;
        int expectedCodeunits = 0;
        int previousRef = 0;
        for (const S& segment: testCase.segs) {
            limWidth += segment.columns;
            expectedCodeunits += segment.str.size();
            termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
            bool limitReached = termpaint_text_measurement_feed_utf8(tm.get(),
                                                                     all.data() + previousRef,
                                                                     toInt(all.length()) - previousRef,
                                                                     true);
            int codeunits = termpaint_text_measurement_last_ref(tm.get());
            int columns = termpaint_text_measurement_last_width(tm.get());
            CHECK(limitReached);
            CHECK(codeunits == expectedCodeunits);
            CHECK(columns == limWidth);
            previousRef = codeunits;
        }
    }
    SECTION("utf16 - width") {
        std::u16string all;
        for (const S& segment: testCase.segs) {
            all += toUtf16(segment.str);
        }

        MeasurementWrapper tm;
        int limWidth = 0;
        int expectedCodeunits = 0;
        int previousRef = 0;
        for (const S& segment: testCase.segs) {
            limWidth += segment.columns;
            expectedCodeunits += toUtf16(segment.str).size();
            termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
            bool limitReached = termpaint_text_measurement_feed_utf16(tm.get(),
                                                                      reinterpret_cast<const uint16_t*>(all.data()) + previousRef,
                                                                      toInt(all.length()) - previousRef,
                                                                      true);
            int codeunits = termpaint_text_measurement_last_ref(tm.get());
            int columns = termpaint_text_measurement_last_width(tm.get());
            CHECK(limitReached);
            CHECK(codeunits == expectedCodeunits);
            CHECK(columns == limWidth);
            previousRef = codeunits;
        }
    }
    SECTION("utf32 - width") {
        std::u32string all;
        for (const S& segment: testCase.segs) {
            all += toUtf32(segment.str);
        }

        MeasurementWrapper tm;
        int limWidth = 0;
        int expectedCodeunits = 0;
        int previousRef = 0;
        for (const S& segment: testCase.segs) {
            limWidth += segment.columns;
            expectedCodeunits += toUtf32(segment.str).size();
            termpaint_text_measurement_set_limit_width(tm.get(), limWidth);
            bool limitReached = termpaint_text_measurement_feed_utf32(tm.get(),
                                                                      reinterpret_cast<const uint32_t*>(all.data()) + previousRef,
                                                                      toInt(all.length()) - previousRef,
                                                                      true);
            int codeunits = termpaint_text_measurement_last_ref(tm.get());
            int columns = termpaint_text_measurement_last_width(tm.get());
            CHECK(limitReached);
            CHECK(codeunits == expectedCodeunits);
            CHECK(columns == limWidth);
            previousRef = codeunits;
        }
    }
}
