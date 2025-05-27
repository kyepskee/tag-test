// oi.h - biblioteka do pisania sprawdzania wejsc (inwer), wyjsc (chk) oraz generowania testow (ingen).
// Author: Krzysztof Małysa.
// In case of problems or improvements, please open an issue / merge request:
// https://sinol3.dasie.mimuw.edu.pl/sinol3/template-package

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio> // to prevent messing <cstdio> after forbidding scanf(), printf(), fopen() by macro
#include <cstdlib> // to prevent messing <cstdlib> after forbidding exit() and _Exit() by macro
#include <cstring>
#include <fcntl.h>
#include <exception>
#include <fstream> // to prevent messing <fstream> after forbidding ifstream and fstream by macro
#include <iostream> // to prevent messing <iostream> after forbidding cin is forbidden by macro
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h> // to prevent messing <unistd.h> after forbidding _exit() by macro
#include <utility>
#include <vector>

using std::string;
using std::vector;

namespace oi {

class CheckerVerdict {
    std::optional<int> partial_score = std::nullopt;
    string partial_score_msg;

public:
    // 100% points
    [[noreturn]] void exit_ok();

    // score * 1% points
    template <class... Msg>
    [[noreturn]] void exit_ok_with_score(int score, Msg&&... msg);

    template<class... Msg>
    void set_partial_score(int score, Msg&&... msg);

    // If partial score is set, then partial_score * 1% points, 0% points otherwise
    template <class... Msg>
    [[noreturn]] void exit_wrong(Msg&&... msg);

} inline checker_verdict;

struct InwerVerdict {
    struct Stream {
        int exit_code;

        struct StreamImpl {
            int exit_code;
            bool printed = false;

            ~StreamImpl();

            template<class T>
            StreamImpl& operator<<(T&& arg);
        };

        StreamImpl operator()();
    };

    // Use like cout: oi::inwer_verdict.exit_ok() << "some comment";
    // Exits the program with 0, after printing the comment.
    Stream exit_ok{0};

    // Use like cout: oi::inwer_verdict.exit_wrong() << "some comment";
    // Exits the program with 1, after printing the comment.
    Stream exit_wrong{1};
} inline inwer_verdict;

template <class... Msg>
[[noreturn]] void bug(Msg&&... msg);

#define oi_assert(condition, ...)                                          \
    ((condition) ? (void)0 : [func = __PRETTY_FUNCTION__](auto&&... msg) { \
        ::oi::detail::exit_with_error_msg(                                 \
            3,                                                             \
            __FILE__ ":",                                                  \
            __LINE__,                                                      \
            ": ",                                                          \
            func,                                                          \
            ": Assertion `" #condition "` failed",                         \
            (sizeof...(msg) == 0 ? "." : ": "),                            \
            std::forward<decltype(msg)>(msg)...                            \
        );                                                                 \
    }(__VA_ARGS__))


enum class Lang {
    EN = 0,
    PL = 1,
};

struct EofType {
} eof;

struct NlType {
} nl;

struct IgnoreWsType {
} ignore_ws; // ignore every whitespace including newline

struct Line {
    string& var;
    size_t max_size;

    Line(string& var_, size_t max_size_) : var{var_}, max_size{max_size_} {}
};

struct Str {
    string& var;
    size_t max_size;

    Str(string& var_, size_t max_size_) : var{var_}, max_size{max_size_} { assert(max_size_ > 0); }
};

struct Char {
    char& var;
    const char* variants;

    Char(char& var_, const char* variants_) : var{var_}, variants{variants_} {}
};

template <class T>
struct Num {
    T& var;
    T min, max; // inclusive

    Num(T& var_, T min_, T max_) : var{var_}, min{min_}, max{max_} { assert(min <= max); }
};

template <class A, class B, class C>
Num(A, B, C) -> Num<A>;

class Scanner {
public:
    enum class Mode {
        UserOutput, // eof ignores newline and whitespace, nl and '\n' ignores whitespace, scanning
                    // whitespace and newlines before EOF is no-op, whitespace is equivalent,
                    // scanning non-whitespace ignores whitespace, destructor scans eof
        Lax, // eof ignores newline and whitespace, nl and '\n' ignores whitespace, scanning
             // whitespace and newlines before EOF is no-op, whitespace is equivalent, scanning
             // non-whitespace ignores whitespace, destructor does nothing
        TestInput, // eof DOES NOT ignore newline or whitespace, nl and '\n' DOES NOT ignore
                   // whitespace, whitespace IS NOT equivalent, destructor scans eof
    };

    Scanner(FILE* file_, Mode mode_, Lang lang_);
    Scanner(const char* file_path, Mode mode_, Lang lang_);

    ~Scanner();

    template <class... Msg>
    [[noreturn]] void error(Msg&&... msg);

    Scanner& operator>>(const char& c);
    Scanner& operator>>(EofType /*unused*/);
    Scanner& operator>>(NlType /*unused*/);
    Scanner& operator>>(IgnoreWsType /*unused*/);

    // Reads line without the trailing newline character.
    // Use like this: scanner >> Line{s, 1000}
    Scanner& operator>>(Line line);

    // Use like this: scanner >> Str{s, 1000}
    Scanner& operator>>(Str str);

    // Use like this: scanner >> Char{c, "TN"}
    Scanner& operator>>(Char chr);

    // Use like this: scanner >> Num{x, -1000, 1000}
    // Works with double too: scanner >> Num{x, -3.14, 3.14}
    template <class T>
    Scanner& operator>>(Num<T> num);

    Scanner(const Scanner&) = delete;
    Scanner(Scanner&&) = delete;
    Scanner& operator=(const Scanner&) = delete;
    Scanner& operator=(Scanner&&) = delete;

    template <class...>
    static constexpr inline bool always_false = false;

    template <class T> requires std::is_same_v<T, char&> ||
                (std::is_convertible_v<T&&, char> &&
                 !std::is_same_v<std::remove_const_t<std::remove_reference_t<T>>, char>)
    Scanner& operator>>(T&& /*unused*/) {
        static_assert(
            always_false<T>,
            R"(Use Num{}, Char{} or Str{} e.g. scanner >> Num{x, 0, 42} >> Char{c, "TN"} >> Str{s, 100};)"
        );
        return *this;
    }

    void do_destructor_checks();

protected:
    FILE* file;
    FILE* owned_file = nullptr;
    Mode mode;
    Lang lang;

    struct Pos {
        size_t line;
        size_t pos;
    };

    Pos next_char_pos = {.line = 1, .pos = 1};
    Pos last_char_pos = {.line = 1, .pos = 1};
    Pos prev_last_char_pos = {.line = 1, .pos = 1};
    bool eofed = false;

    std::optional<int> next_char;

    enum class DelayedUnreadChars : uint8_t { WHITESPACE, NEWLINE };
    std::vector<DelayedUnreadChars> delayed_unread_chars;

    bool getchar(int& ch) noexcept; // returns true if not eofed
    void ungetchar(int ch) noexcept;
    static string char_description(int ch);

    void read_delayed_unread_chars();

    template <class T>
    void scan_integer(T& val);

    template <class T>
    void scan_floating_point(T& val);
};

class Random {
public:
    explicit Random(uint_fast64_t seed = 5489);

    template <class T> requires std::is_arithmetic_v<T>
    T operator()(T min, T max);

    template <class RandomAccessIterator>
    void shuffle(RandomAccessIterator begin, RandomAccessIterator end);

    template <class T>
    void shuffle(T& container) {
        shuffle(container.begin(), container.end());
    }

    Random(const Random&) = delete;
    Random(Random&&) = default;
    Random& operator=(const Random&) = delete;
    Random& operator=(Random&&) = default;

private:
    std::mt19937_64 generator;
};

struct TestInput {
    string str;

    explicit TestInput(string str_) : str{std::move(str_)} {}
};

struct TestOutput {
    string str;

    explicit TestOutput(string str_) : str{std::move(str_)} {}
};

struct UserOutput {
    string str;

    explicit UserOutput(string str_) : str{std::move(str_)} {}
};

struct CheckerOutput {
    string str;

    explicit CheckerOutput(string str_) : str{std::move(str_)} {}
};

} // namespace oi

#define CONCAT_RAW(a, b) a##b
#define CONCAT(a, b) CONCAT_RAW(a, b)
#define CHECKER_TEST(...)                                                             \
    namespace {                                                                       \
    __attribute__((constructor)) void CONCAT(checker_test_constructor_, __LINE__)() { \
        ::oi::detail::get_checker_test_fns().emplace_back([] {                        \
            using oi::CheckerOutput;                                                  \
            using oi::TestInput;                                                      \
            using oi::TestOutput;                                                     \
            using oi::UserOutput;                                                     \
            ::oi::detail::checker_test(                                               \
                string{__FILE__ ":"} + std::to_string(__LINE__), __VA_ARGS__          \
            );                                                                        \
        });                                                                           \
    }                                                                                 \
    }

//////////////////////////////// Implementation ////////////////////////////////

namespace oi {

inline std::set<Scanner*>& get_all_scanners() noexcept {
    static std::set<Scanner*> scanners;
    [[maybe_unused]] static bool x = [] {
        void (*func)() = [] {
            // To succeed, the destructor checks have to pass
            for (auto* scanner : get_all_scanners()) {
                scanner->do_destructor_checks();
            }
        };
        if (atexit(func)) {
            std::terminate();
        }
        return true;
    }();
    return scanners;
}

[[noreturn]] inline void CheckerVerdict::exit_ok() {
    // To get the whole score, the destructor checks have to pass
    for (auto* scanner : get_all_scanners()) {
        scanner->do_destructor_checks();
    }
    std::cout << "OK\n\n100\n" << std::flush;
    _exit(0);
}

template <class... Msg>
[[noreturn]] void CheckerVerdict::exit_ok_with_score(int score, Msg&&... msg) {
    assert(0 <= score && score <= 100);
    if (score == 100) {
        // To get the whole score, the destructor checks have to pass
        for (auto* scanner : get_all_scanners()) {
            scanner->do_destructor_checks();
        }
    }
    std::cout << "OK\n";
    (std::cout << ... << std::forward<Msg>(msg)) << '\n';
    std::cout << score << '\n' << std::flush;
    _exit(0);
}

template<class... Msg>
inline void CheckerVerdict::set_partial_score(int score, Msg&&... msg) {
    assert(0 <= score && score < 100);
    partial_score = score;

    std::stringstream ss;
    (ss << ... << std::forward<Msg>(msg));
    partial_score_msg = std::move(ss).str();
}

template <class... Msg>
[[noreturn]] void CheckerVerdict::exit_wrong(Msg&&... msg) {
    if (partial_score) {
        std::cout << "OK\n";
        std::cout << partial_score_msg;
        if (!partial_score_msg.empty() && sizeof...(msg) != 0) {
            std::cout << "; ";
        }
        (std::cout << ... << std::forward<Msg>(msg)) << '\n';
        std::cout << *partial_score << '\n' << std::flush;
    } else {
        std::cout << "WRONG\n";
        (std::cout << ... << std::forward<Msg>(msg)) << '\n';
        std::cout << "0\n" << std::flush;
    }
    _exit(0);
}

InwerVerdict::Stream::StreamImpl InwerVerdict::Stream::operator()() {
    if (exit_code == 0) {
        // To pass the input verification, the destructor checks have to pass
        for (auto* scanner : get_all_scanners()) {
            scanner->do_destructor_checks();
        }
    }
    return StreamImpl{exit_code};
}

template<class T>
InwerVerdict::Stream::StreamImpl& InwerVerdict::Stream::StreamImpl::operator<<(T&& arg) {
    printed = true;
    std::cout << std::forward<T>(arg);
    return *this;
}

InwerVerdict::Stream::StreamImpl::~StreamImpl() {
    if (printed) {
        std::cout << '\n';
    }
    std::cout << std::flush;
    _exit(exit_code);
}

namespace detail {

std::ostream*& get_error_ostream() noexcept {
    static auto* kind = &std::cerr;
    return kind;
}

void change_error_ostream_to_cout() noexcept {
    get_error_ostream() = &std::cout;
}

template <class... Msg>
[[noreturn]] void exit_with_error_msg(int exit_code, Msg&&... msg) {
    (*get_error_ostream() << ... << std::forward<Msg>(msg)) << '\n' << std::flush;
    _exit(exit_code);
}

} // namespace detail

template <class... Msg>
[[noreturn]] void bug(Msg&&... msg) {
    detail::exit_with_error_msg(2, "BUG: ", std::forward<Msg>(msg)...);
}

inline Scanner::Scanner(FILE* file_, Mode mode_, Lang lang_)
: file{file_}
, mode{mode_}
, lang{lang_} {
    get_all_scanners().emplace(this);
}

inline Scanner::Scanner(const char* file_path, Mode mode_, Lang lang_)
: file{[file_path] {
    FILE* f = fopen(file_path, "r");
    if (!f) {
        bug("fopen() failed - ", strerror(errno));
    }
    return f;
}()}
, owned_file{file}
, mode{mode_}
, lang{lang_} {
    get_all_scanners().emplace(this);
}

inline Scanner::~Scanner() {
    do_destructor_checks();

    get_all_scanners().erase(this);
    if (owned_file) {
        (void)fclose(owned_file);
    }
}

template <class... Msg>
[[noreturn]] void do_error(Scanner::Mode mode, Msg&&... msg) {
    switch (mode) {
    case Scanner::Mode::UserOutput: checker_verdict.exit_wrong(std::forward<Msg>(msg)...);
    case Scanner::Mode::Lax: detail::exit_with_error_msg(4, "Lax scanner: ", std::forward<Msg>(msg)...);
    case Scanner::Mode::TestInput:
        (inwer_verdict.exit_wrong() << ... << std::forward<Msg>(msg));
    }
    __builtin_unreachable();
}

template <class... Msg>
[[noreturn]] void Scanner::error(Msg&&... msg) {
    switch (lang) {
    case Lang::EN:
        do_error(
            mode,
            "Line ",
            last_char_pos.line,
            ", position ",
            last_char_pos.pos,
            ": ",
            std::forward<Msg>(msg)...
        );
    case Lang::PL:
        do_error(
            mode,
            "Wiersz ",
            last_char_pos.line,
            ", pozycja ",
            last_char_pos.pos,
            ": ",
            std::forward<Msg>(msg)...
        );
    }
    __builtin_unreachable();
}

constexpr const char* read_eof_expected_a_string[] = {
    "Read EOF, expected a string",
    "Wczytano EOF, oczekiwano napisu",
};
constexpr const char* too_long_string[] = {
    "Too long string",
    "Zbyt dlugi napis",
};
constexpr const char* read_eof_expected_a_number[] = {
    "Read EOF, expected a number",
    "Wczytano EOF, oczekiwano liczby",
};
constexpr const char* read_minus_expected_a_positive_number[] = {
    "Read '-', expected a non-negative number",
    "Wczytano '-', oczekiwano nieujemnej liczby",
};
constexpr const char* integer_value_out_of_range[] = {
    "Integer value out of range",
    "Liczba calkowita spoza zakresu",
};
constexpr const char* real_number_value_out_of_range[] = {
    "Real number value out of range",
    "Liczba rzeczywista spoza zakresu",
};

inline Scanner& Scanner::operator>>(const char& c) {
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        if (isspace(c)) {
            if (c == '\n') {
                delayed_unread_chars.emplace_back(DelayedUnreadChars::NEWLINE);
            } else {
                delayed_unread_chars.emplace_back(DelayedUnreadChars::WHITESPACE);
            }
            return *this;
        }
    } break;
    case Mode::TestInput: break;
    }

    read_delayed_unread_chars();
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        *this >> ignore_ws;
    } break;
    case Mode::TestInput: break;
    }

    int ch = 0;
    if (!getchar(ch)) {
        switch (lang) {
        case Lang::EN: error("Read EOF, expected ", char_description(c));
        case Lang::PL: error("Wczytano EOF, oczekiwano ", char_description(c));
        }
    }
    if (ch != c) {
        switch (lang) {
        case Lang::EN: error("Read ", char_description(ch), ", expected ", char_description(c));
        case Lang::PL:
            error("Wczytano ", char_description(ch), ", oczekiwano ", char_description(c));
        }
    }
    return *this;
}

inline Scanner& Scanner::operator>>(EofType /*unused*/) {
    int ch = 0;
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        // Ignore delayed chars
        delayed_unread_chars.clear();
        // Ignore whitespace including newline
        for (;;) {
            if (!getchar(ch)) {
                return *this;
            }
            if (!isspace(ch)) {
                ungetchar(ch);
                break;
            }
        }
    } break;
    case Mode::TestInput: break;
    }

    if (getchar(ch)) {
        switch (lang) {
        case Lang::EN: error("Read ", char_description(ch), ", expected EOF");
        case Lang::PL: error("Wczytano ", char_description(ch), ", oczekiwano EOF");
        }
    }
    return *this;
}

inline Scanner& Scanner::operator>>(NlType /*unused*/) {
    return *this >> '\n';
}

inline Scanner& Scanner::operator>>(IgnoreWsType /*unused*/) {
    read_delayed_unread_chars();
    int ch = 0;
    for (;;) {
        if (!getchar(ch)) {
            ungetchar(EOF);
            break;
        }
        if (ch == '\n' || !isspace(ch)) {
            ungetchar(ch);
            break;
        }
    }
    return *this;
}

inline Scanner& Scanner::operator>>(Line line) {
    read_delayed_unread_chars();
    line.var.clear();
    int ch = 0;
    for (;;) {
        if (!getchar(ch)) {
            ungetchar(EOF);
            break;
        }
        if (ch == '\n') {
            ungetchar(ch);
            break;
        }
        line.var += static_cast<char>(ch);
    }
    return *this;
}

inline Scanner& Scanner::operator>>(Str str) {
    read_delayed_unread_chars();
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        *this >> ignore_ws;
    } break;
    case Mode::TestInput: break;
    }

    str.var.clear();
    int ch = 0;
    if (!getchar(ch)) {
        error(read_eof_expected_a_string[static_cast<int>(lang)]);
    }
    if (isspace(ch)) {
        switch (lang) {
        case Lang::EN: error("Read ", char_description(ch), ", expected a string");
        case Lang::PL: error("Wczytano ", char_description(ch), ", oczekiwano napisu");
        }
    }

    for (;;) {
        str.var += static_cast<char>(ch);
        if (str.var.size() > str.max_size) {
            error(too_long_string[static_cast<int>(lang)]);
        }

        if (!getchar(ch)) {
            ungetchar(EOF);
            break;
        }
        if (isspace(ch)) {
            ungetchar(ch);
            break;
        }
    }
    return *this;
}

inline Scanner& Scanner::operator>>(Char chr) {
    read_delayed_unread_chars();
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        *this >> ignore_ws;
    } break;
    case Mode::TestInput: break;
    }

    int ch = 0;
    if (!getchar(ch)) {
        switch (lang) {
        case Lang::EN: error("Read EOF, expected one of characters: ", chr.variants);
        case Lang::PL: error("Wczytano EOF, oczekiwano jednego ze znakow: ", chr.variants);
        }
    }
    if (strchr(chr.variants, ch) == nullptr) {
        switch (lang) {
        case Lang::EN:
            error("Read ", char_description(ch), ", expected one of characters: ", chr.variants);
        case Lang::PL:
            error(
                "Wczytano ", char_description(ch), ", oczekiwano jednego ze znakow: ", chr.variants
            );
        }
    }
    chr.var = static_cast<char>(ch);
    return *this;
}

template <class T>
inline Scanner& Scanner::operator>>(Num<T> num) {
    read_delayed_unread_chars();
    switch (mode) {
    case Mode::UserOutput:
    case Mode::Lax: {
        *this >> ignore_ws;
    } break;
    case Mode::TestInput: break;
    }

    if constexpr (std::is_integral_v<T>) {
        scan_integer(num.var);
        if (num.var < num.min || num.var > num.max) {
            error(integer_value_out_of_range[static_cast<int>(lang)]);
        }
    } else {
        scan_floating_point(num.var);
        if (num.var < num.min || num.var > num.max) {
            error(real_number_value_out_of_range[static_cast<int>(lang)]);
        }
    }
    return *this;
}

inline bool Scanner::getchar(int& ch) noexcept {
    if (eofed) {
        return false;
    }

    if (next_char) {
        ch = *next_char;
        next_char = std::nullopt;
    } else {
        ch = getc_unlocked(file);
    }
    eofed = (ch == EOF);
    prev_last_char_pos = last_char_pos;
    last_char_pos = next_char_pos;
    if (ch == '\n') {
        ++next_char_pos.line;
        next_char_pos.pos = 1;
    } else {
        ++next_char_pos.pos;
    }
    return !eofed;
}

inline void Scanner::ungetchar(int ch) noexcept {
    assert(!next_char && "cannot ungetchar() more than one without getchar()");
    next_char = ch;
    next_char_pos = last_char_pos;
    last_char_pos = prev_last_char_pos;
    eofed = false;
}

inline string Scanner::char_description(int ch) {
    if (std::isgraph(ch)) {
        return {'\'', static_cast<char>(ch), '\''};
    }

    if (ch == ' ') {
        return "' '";
    }
    if (ch == '\n') {
        return "'\\n'";
    }
    if (ch == '\r') {
        return "'\\r'";
    }
    if (ch == '\t') {
        return "'\\t'";
    }
    if (ch == '\0') {
        return "'\\0'";
    }

    constexpr char digits[] = "0123456789abcdef";
    return {'\'', '\\', 'x', digits[ch >> 4], digits[ch & 15], '\''};
}

inline void Scanner::read_delayed_unread_chars() {
    auto do_read_char = [this](const char* char_description_str) {
        int ch = 0;
        if (!getchar(ch)) {
            switch (lang) {
            case Lang::EN: error("Read EOF, expected ", char_description_str);
            case Lang::PL: error("Wczytano EOF, oczekiwano ", char_description_str);
            }
        }
        return ch;
    };
    auto unexpected_char_error = [this](int ch, const char* expected_char_description_str) {
        switch (lang) {
        case Lang::EN:
            error("Read ", char_description(ch), ", expected ", expected_char_description_str);
        case Lang::PL:
            error(
                "Wczytano ", char_description(ch), ", oczekiwano ", expected_char_description_str
            );
        }
    };
    for (auto delayed_unread_char : delayed_unread_chars) {
        switch (delayed_unread_char) {
        case DelayedUnreadChars::WHITESPACE: {
            switch (mode) {
            case Mode::UserOutput:
            case Mode::Lax: {
                int ch = do_read_char("' '");
                if (!isspace(ch) || ch == '\n') {
                    unexpected_char_error(ch,"' '");
                }
            } break;
            case Mode::TestInput: std::terminate(); // BUG: should not happen
            }
        } break;
        case DelayedUnreadChars::NEWLINE: {
            switch (mode) {
            case Mode::UserOutput:
            case Mode::Lax: {
                // Read newline ignoring whitespace before it
                for (;;) {
                    int ch = do_read_char("'\\n'");
                    if (ch == '\n') {
                        break;
                    }
                    if (isspace(ch)) {
                        continue;
                    }
                    unexpected_char_error(ch, "'\\n'");
                }
            } break;
            case Mode::TestInput: std::terminate(); // BUG: should not happen
            }
        } break;
        }
    }
    delayed_unread_chars.clear();
}

template <class T>
void Scanner::scan_integer(T& val) {
    static_assert(std::is_integral_v<T>);
    int ch = 0;
    if (!getchar(ch)) {
        error(read_eof_expected_a_number[static_cast<int>(lang)]);
    }

    bool minus = false;
    if (ch == '-') {
        if (std::is_unsigned_v<T>) {
            error(read_minus_expected_a_positive_number[static_cast<int>(lang)]);
        }
        minus = true;
        if (!getchar(ch)) {
            error(read_eof_expected_a_number[static_cast<int>(lang)]);
        }
    }

    if (ch < '0' || '9' < ch) {
        switch (lang) {
        case Lang::EN: error("Read ", char_description(ch), ", expected a number");
        case Lang::PL: error("Wczytano ", char_description(ch), ", oczekiwano liczby");
        }
    }

    val = static_cast<T>(minus ? '0' - ch : ch - '0'); // Will not overflow
    for (;;) {
        if (!getchar(ch)) {
            ungetchar(EOF);
            break;
        }
        if (!isdigit(ch)) {
            ungetchar(ch);
            break;
        }

        if (__builtin_mul_overflow(val, 10, &val)) {
            error(integer_value_out_of_range[static_cast<int>(lang)]);
        }
        if (!minus && __builtin_add_overflow(val, ch - '0', &val)) {
            error(integer_value_out_of_range[static_cast<int>(lang)]);
        }
        if (minus && __builtin_sub_overflow(val, ch - '0', &val)) {
            error(integer_value_out_of_range[static_cast<int>(lang)]);
        }
    }
}

template <class T>
void Scanner::scan_floating_point(T& val) {
    static_assert(std::is_floating_point_v<T>);
    int ch = 0;
    if (!getchar(ch)) {
        error(read_eof_expected_a_number[static_cast<int>(lang)]);
    }

    bool minus = false;
    if (ch == '-') {
        minus = true;
        if (!getchar(ch)) {
            error(read_eof_expected_a_number[static_cast<int>(lang)]);
        }
    }

    if (ch < '0' || '9' < ch) {
        switch (lang) {
        case Lang::EN: error("Read ", char_description(ch), ", expected a number");
        case Lang::PL: error("Wczytano ", char_description(ch), ", oczekiwano liczby");
        }
    }

    val = (minus ? '0' - ch : ch - '0'); // Will not overflow
    for (;;) {
        if (!getchar(ch)) {
            ungetchar(EOF);
            return;
        }
        if (ch == '.') {
            break;
        }
        if (!isdigit(ch)) {
            ungetchar(ch);
            return;
        }
        val *= 10;
        if (!minus) {
            val += ch - '0';
        } else {
            val -= ch - '0';
        }
        if (std::isinf(val)) {
            error(real_number_value_out_of_range[static_cast<int>(lang)]);
        }
    }

    T pow10 = 1;
    T subpoint = 0;
    for (;;) {
        if (!getchar(ch)) {
            ungetchar(EOF);
            break;
        }
        if (!isdigit(ch)) {
            ungetchar(ch);
            break;
        }
        pow10 *= 0.1;
        subpoint += pow10 * (ch - '0');
    }
    if (!minus) {
        val += subpoint;
    } else {
        val -= subpoint;
    }
    if (std::isinf(val)) {
        error(real_number_value_out_of_range[static_cast<int>(lang)]);
    }
}

inline void Scanner::do_destructor_checks() {
    switch (mode) {
    case Mode::UserOutput:
    case Mode::TestInput: {
        *this >> eof;
    } break;
    case Mode::Lax: break;
    }
}

inline Random::Random(uint_fast64_t seed) : generator{seed} {}

template <class T> requires std::is_arithmetic_v<T>
T Random::operator()(T min, T max) {
    oi_assert(min <= max);
    constexpr auto generator_range_len = decltype(generator)::max() - decltype(generator)::min();
    if constexpr (std::is_floating_point_v<T>) {
        auto val = generator() - decltype(generator)::min(); // in range [0, generator_range_len]
        T scaled_val = static_cast<T>(val) / static_cast<T>(generator_range_len); // in range [0, 1]
        return scaled_val * (max - min) + min;
    } else if constexpr (std::is_unsigned_v<T>) {
        auto range_len = static_cast<uint_fast64_t>(max) - static_cast<uint_fast64_t>(min) + 1;
        if (range_len == 0) { // max range
            return static_cast<T>(generator());
        }
        auto limit = generator_range_len - generator_range_len % range_len;
        for (;;) {
            auto val =
                generator() - decltype(generator)::min(); // in range [0, generator_range_len]
            // We want val to be in range [0, generator_range_len - generator_range_len % range_len
            // - 1]
            // <=> val < generator_range_len - generator_range_len % range_len
            if (val >= limit) {
                continue;
            }
            return static_cast<T>(val % range_len) + min;
        }
    } else {
        using UT = std::make_unsigned_t<T>;
        // Shift [min, max] to [umin, umax], where umin >= 0
        UT umin = static_cast<UT>(min) - static_cast<UT>(std::numeric_limits<T>::min());
        UT umax = static_cast<UT>(max) - static_cast<UT>(std::numeric_limits<T>::min());
        UT val = this->operator()<UT>(umin, umax) + static_cast<UT>(std::numeric_limits<T>::min());
        return static_cast<T>(val);
    }
}

template <class RandomAccessIterator>
void Random::shuffle(RandomAccessIterator begin, RandomAccessIterator end) {
    for (auto i = end - begin; i > 1;) {
        --i;
        std::swap(begin[i], begin[this->operator()<decltype(i)>(0, i)]);
    }
}

} // namespace oi

namespace oi::detail {

inline std::vector<void (*)()>& get_checker_test_fns() {
    static std::vector<void (*)()> test_fns;
    return test_fns;
}

} // namespace oi::detail

int the_only_real_true_main(int, char**);

namespace oi::detail {

inline void checker_test(
    const string& test_name,
    TestInput test_input,
    TestOutput test_output,
    UserOutput user_output,
    CheckerOutput checker_output
) {
    auto terminate_with_error = [test_name](auto&&... msg) {
        ::oi::detail::exit_with_error_msg(
            5, "Checker test ", test_name, " failed: ", std::forward<decltype(msg)>(msg)...
        );
    };

    auto create_fd = [&terminate_with_error] {
        // Using tmpfile() to be POSIX compliant, so that it works on MacOS.
        auto* f = tmpfile();
        if (!f) {
            terminate_with_error("tmpfile() - ", strerror(errno));
        }
        int fd = dup(fileno(f));
        if (fclose(f)) {
            terminate_with_error("flose() - ", strerror(errno));
        }
        return fd;
    };

    auto create_fd_with_contents = [&terminate_with_error, &create_fd](const string& contents) {
        auto fd = create_fd();
        if (pwrite(fd, contents.data(), contents.size(), 0) !=
            static_cast<ssize_t>(contents.size()))
        {
            terminate_with_error("pwrite() - ", strerror(errno));
        }
        return fd;
    };

    int in_fd = create_fd_with_contents(test_input.str);
    int out_fd = create_fd_with_contents(test_output.str);
    int user_out_fd = create_fd_with_contents(user_output.str);

    int checker_out_fd = create_fd();

    int pid = fork();
    if (pid == -1) {
        terminate_with_error("fork() - ", strerror(errno));
    }
    if (pid == 0) {
        (void)freopen("/dev/null", "r", stdin);
        if (dup2(checker_out_fd, STDOUT_FILENO) != STDOUT_FILENO) {
            terminate_with_error("dup2() - ", strerror(errno));
        }

        char prog_name[] = "";
        auto test_input_path = string{"/dev/fd/"} + std::to_string(in_fd);
        auto test_output_path = string{"/dev/fd/"} + std::to_string(out_fd);
        auto user_output_path = string{"/dev/fd/"} + std::to_string(user_out_fd);

        char* argv[] = {
            prog_name,
            test_input_path.data(),
            user_output_path.data(),
            test_output_path.data(),
            nullptr,
        };
        exit(the_only_real_true_main(4, argv));
    }
    (void)close(user_out_fd);
    (void)close(out_fd);
    (void)close(in_fd);

    int status;
    if (waitpid(pid, &status, 0) != pid) {
        terminate_with_error("waitpid() - ", strerror(errno));
    }

    std::string captured_output;
    std::array<char, 4096> buff;
    for (off_t offset = 0;;) {
        auto rc = pread(checker_out_fd, buff.data(), buff.size(), offset);
        if (rc > 0) {
            offset += rc;
            captured_output.append(buff.data(), static_cast<size_t>(rc));
            continue;
        }
        if (rc == 0) {
            break;
        }
        terminate_with_error("pread() - ", strerror(errno));
    }
    (void)close(checker_out_fd);

    if (!WIFEXITED(status)) {
        terminate_with_error("checker program crashed with output:\n", captured_output);
    }

    int exit_code = WEXITSTATUS(status);
    if (exit_code != 0) {
        terminate_with_error(
            "checker program exited with ", exit_code, " with output:\n", captured_output
        );
    }
    if (captured_output != checker_output.str) {
        terminate_with_error(
            "checker program exited with 0 with output:\n",
            captured_output,
            "\nexpected it to exit with 0 and output:\n",
            checker_output.str
        );
    }
}

inline void checker_test(const string &test_name, const string& data) {
    constexpr std::string_view test_in_str = "@test_in\n";
    constexpr std::string_view test_out_str = "@test_out\n";
    constexpr std::string_view user_str = "@user\n";
    constexpr std::string_view checker_str = "@checker\n";
    auto find_beg_of = [&data](std::string_view delim, size_t start_pos) -> size_t {
        auto delim_pos = data.find(delim, start_pos);
        if (delim_pos == string::npos) {
            oi::bug("Could not find ", delim, " in the test string");
        }
        return delim_pos + delim.size();
    };

    auto test_in_beg = find_beg_of(test_in_str, 0);
    auto test_out_beg = find_beg_of(test_out_str, test_in_beg);
    auto user_beg = find_beg_of(user_str, test_out_beg);
    auto checker_beg = find_beg_of(checker_str, user_beg);
    checker_test(
        test_name,
        TestInput{string{
            data.begin() + static_cast<ssize_t>(test_in_beg),
            data.begin() + static_cast<ssize_t>(test_out_beg - test_out_str.size())
        }},
        TestOutput{string{
            data.begin() + static_cast<ssize_t>(test_out_beg),
            data.begin() + static_cast<ssize_t>(user_beg - user_str.size())
        }},
        UserOutput{string{
            data.begin() + static_cast<ssize_t>(user_beg),
            data.begin() + static_cast<ssize_t>(checker_beg - checker_str.size())
        }},
        CheckerOutput{data.substr(checker_beg)}
    );
}

inline bool we_are_running_on_sio2() {
    auto user_str = getenv("USER");
    return user_str != nullptr && std::string_view{user_str} == "oioioiworker";
}

} // namespace oi::detail

#define main(...)                                                                              \
    the_only_real_true_main(__VA_ARGS__);                                                      \
    /* Before checker_test(), the_only_real_true_main() is declared as: */                     \
    /* int the_only_real_true_main(int, char**); */                                            \
    /* it overloads the_only_real_true_main() in case __VA_ARGS__ is empty, */                 \
    /* hence the workaround */                                                                 \
    int only_for_type_deduction_main(__VA_ARGS__);                                             \
                                                                                               \
    int main(int argc, char** argv) {                                                          \
        auto filename = std::string_view{__FILE__};                                            \
        /* Remove the extension */                                                             \
        while (!filename.empty() && filename.back() != '.') {                                  \
            filename.remove_suffix(1);                                                         \
        }                                                                                      \
        if (filename.ends_with("chk.")) {                                                      \
            ::oi::detail::change_error_ostream_to_cout();                                      \
        }                                                                                      \
                                                                                               \
        if (!::oi::detail::we_are_running_on_sio2() &&                                         \
            !oi::detail::get_checker_test_fns().empty())                                       \
        {                                                                                      \
            std::cerr << "Running " << oi::detail::get_checker_test_fns().size()               \
                      << " checker tests...\n";                                                \
            for (auto checker_test_fn : ::oi::detail::get_checker_test_fns()) {                \
                checker_test_fn();                                                             \
            }                                                                                  \
            std::cerr << "All tests passed.\n";                                                \
        }                                                                                      \
                                                                                               \
        return [&](auto main_func) {                                                           \
            /* We need main_func in the branches to be template-dependent, hence the lambda */ \
            if constexpr (std::is_convertible_v<decltype(main_func), int (*)()>) {             \
                return main_func();                                                            \
            } else {                                                                           \
                return main_func(argc, argv);                                                  \
            }                                                                                  \
        }(static_cast<decltype(&only_for_type_deduction_main)>(&the_only_real_true_main));     \
    }                                                                                          \
    int the_only_real_true_main(__VA_ARGS__)

///////////////// FORBIDDING MACROS /////////////////
#define cin                                                         \
    cin;                                                            \
    static_assert(false, "Don't use cin, use oi::Scanner instead"); \
    ::std::cin
#define cout cout; static_assert(false, "Don't use cout, use oi::checker_verdict or oi::inwer_verdict instead or oi::bug()"); ::std::cout
#define fstream                                                         \
    fstream;                                                            \
    static_assert(false, "Don't use fstream, use oi::Scanner instead"); \
    ::std::fstream
#define ifstream                                                         \
    ifstream;                                                            \
    static_assert(false, "Don't use ifstream, use oi::Scanner instead"); \
    ::std::ifstream
#define scanf(...) static_assert(false, "Don't use scanf(), use oi::Scanner instead")
#define fopen(...) static_assert(false, "Don't use fopen(), use oi::Scanner instead")
#define printf(...) static_assert(false, "Don't use printf(), use oi::checker_verdict or oi::inwer_verdict or oi::bug() instead")
#define puts(...) \
    static_assert(false, "Don't use puts(), use oi::checker_verdict or oi::inwer_verdict instead")
#define exit(...) static_assert(false, "Don't use exit(), use oi::checker_verdict or oi::inwer_verdict instead")
#define _exit(...) \
    static_assert(false, "Don't use _exit(), use oi::checker_verdict or oi::inwer_verdict instead")
#define _Exit(...) \
    static_assert(false, "Don't use _Exit(), use oi::checker_verdict or oi::inwer_verdict instead")
#define rand(...) static_assert(false, "Don't use rand(), use oi::Random")::std::rand
#define mt19337 \
    mt19337;    \
    static_assert(false, "Don't use mt19337, use oi::Random")::std::mt19337
#define mt19337_64 \
    mt19337_64;    \
    static_assert(false, "Don't use mt19337_64, use oi::Random")::std::mt19337_64
#undef assert
#define assert(...) static_assert(false, "Don't use assert, use oi_assert()")


////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// oi.h tests ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef OI_H_TESTS

#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <list>
#include <map>
#include <random>
#include <set>
#include <string_view>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <variant>
#include <vector>

std::vector<void (*)()>& get_test_fns() {
    static std::vector<void (*)()> test_fns;
    return test_fns;
}

struct Exits { int code; string msg; };

struct Crashes {};

#define TEST(test_name_, test_in_, ...) DO_TEST(__COUNTER__, test_name_, test_in_, __VA_ARGS__)
#define DO_TEST(counter, test_name_, test_in_, ...)                                                \
    void CONCAT(test_body, counter)();                                                             \
                                                                                                   \
    void CONCAT(test_, counter)() {                                                                \
        std::string_view test_name = test_name_;                                                   \
        std::string_view test_in = test_in_;                                                       \
        std::variant<Exits, Crashes> expected_out = (__VA_ARGS__);                                 \
        auto terminate_with_error = [test_name](auto&&... msg) {                                   \
            std::cerr << "Test " << test_name << " failed: (at line " << __LINE__ << "): error: "; \
            (std::cerr << ... << msg) << std::endl;                                                \
            std::terminate();                                                                      \
        };                                                                                         \
                                                                                                   \
        int in_fd = memfd_create("oi.hh test in", MFD_CLOEXEC);                                    \
        if (in_fd == -1) {                                                                         \
            terminate_with_error("memfd_create() - ", strerror(errno));                            \
        }                                                                                          \
        if (pwrite(in_fd, test_in.data(), test_in.size(), 0) !=                                    \
            static_cast<ssize_t>(test_in.size()))                                                  \
        {                                                                                          \
            terminate_with_error("pwrite() - ", strerror(errno));                                  \
        }                                                                                          \
                                                                                                   \
        int out_fd = memfd_create("oi.hh test out", MFD_CLOEXEC);                                  \
        if (out_fd == -1) {                                                                        \
            terminate_with_error("memfd_create() - ", strerror(errno));                            \
        }                                                                                          \
                                                                                                   \
        int pid = fork();                                                                          \
        if (pid == -1) {                                                                           \
            terminate_with_error("fork() - ", strerror(errno));                                    \
        }                                                                                          \
        if (pid == 0) {                                                                            \
            if (dup2(in_fd, STDIN_FILENO) != STDIN_FILENO) {                                       \
                terminate_with_error("dup2() - ", strerror(errno));                                \
            }                                                                                      \
            if (dup2(out_fd, STDOUT_FILENO) != STDOUT_FILENO) {                                    \
                terminate_with_error("dup2() - ", strerror(errno));                                \
            }                                                                                      \
            int old_stderr = dup(STDERR_FILENO);                                                   \
            (void)freopen("/dev/null", "w", stderr);                                               \
            CONCAT(test_body, counter)();                                                          \
            (void)dup2(old_stderr, STDERR_FILENO);                                                 \
            terminate_with_error("program code did not exit");                                     \
        }                                                                                          \
        (void)close(in_fd);                                                                        \
        int status;                                                                                \
        if (waitpid(pid, &status, 0) != pid) {                                                     \
            terminate_with_error("waitpid() - ", strerror(errno));                                 \
        }                                                                                          \
                                                                                                   \
        std::string out;                                                                           \
        std::array<char, 4096> buff;                                                               \
        for (off_t offset = 0;;) {                                                                 \
            auto rc = pread(out_fd, buff.data(), buff.size(), offset);                             \
            if (rc > 0) {                                                                          \
                offset += rc;                                                                      \
                out.append(buff.data(), static_cast<size_t>(rc));                                  \
                continue;                                                                          \
            }                                                                                      \
            if (rc == 0) {                                                                         \
                break;                                                                             \
            }                                                                                      \
            terminate_with_error("pread() - ", strerror(errno));                                   \
        }                                                                                          \
        (void)close(out_fd);                                                                       \
                                                                                                   \
        if (WIFEXITED(status)) {                                                                   \
            auto exit_code = WEXITSTATUS(status);                                                  \
            if (auto e = std::get_if<Exits>(&expected_out)) {                                      \
                if (e->code != exit_code || out != e->msg) {                                       \
                    terminate_with_error(                                                          \
                        "program exited with ",                                                    \
                        exit_code,                                                                 \
                        " and output:\n",                                                          \
                        out,                                                                       \
                        "\nExpected it to exit with ",                                             \
                        e->code,                                                                   \
                        " and output:\n",                                                          \
                        e->msg                                                                     \
                    );                                                                             \
                }                                                                                  \
            } else {                                                                               \
                if (!std::holds_alternative<Crashes>(expected_out)) {                              \
                    std::terminate();                                                              \
                }                                                                                  \
                terminate_with_error("program exited with ", exit_code, " but expected a crash");  \
            }                                                                                      \
        } else {                                                                                   \
            if (auto e = std::get_if<Exits>(&expected_out)) {                                      \
                terminate_with_error("program crashed but was expected to exit with ", e->code);   \
            } else {                                                                               \
                if (!std::holds_alternative<Crashes>(expected_out)) {                              \
                    std::terminate();                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    namespace {                                                                                    \
    __attribute__((constructor)) void CONCAT(test_constructor_, counter)() {                       \
        get_test_fns().emplace_back(&CONCAT(test_, counter));                                      \
    }                                                                                              \
    }                                                                                              \
    void CONCAT(test_body, counter)()

TEST("checker_verdict.exit_ok()", "", Exits{0, "OK\n\n100\n"}) {
    oi::checker_verdict.exit_ok();
}

TEST("checker_verdict.exit_ok_with_score(score)", "", Exits{0, "OK\n\n42\n"}) {
    oi::checker_verdict.exit_ok_with_score(42);
}

TEST("checker_verdict.exit_ok_with_score(score, msg)", "", Exits{0, "OK\na3\n42\n"}) {
    oi::checker_verdict.exit_ok_with_score(42, "a", 3);
}

TEST("checker_verdict.exit_ok_with_score(101)", "", Crashes{}) {
    oi::checker_verdict.exit_ok_with_score(101);
}

TEST("checker_verdict.exit_ok_with_score(101, msg)", "", Crashes{}) {
    oi::checker_verdict.exit_ok_with_score(101, "a", 3);
}

TEST("checker_verdict.exit_ok_with_score(-1)", "", Crashes{}) {
    oi::checker_verdict.exit_ok_with_score(-1);
}

TEST("checker_verdict.exit_ok_with_score(-1, msg)", "", Crashes{}) {
    oi::checker_verdict.exit_ok_with_score(-1, "a", 3);
}

TEST("checker_verdict.exit_wrong()", "", Exits{0, "WRONG\n\n0\n"}) {
    oi::checker_verdict.exit_wrong();
}

TEST("checker_verdict.exit_wrong(args)", "", Exits{0, "WRONG\n1abc42\n0\n"}) {
    oi::checker_verdict.exit_wrong(1, "abc", 42);
}

TEST("checker_verdict.set_partial_score() + ok()", "", Exits{0, "OK\n\n100\n"}) {
    oi::checker_verdict.set_partial_score(42, "x y z ", 8);
    oi::checker_verdict.exit_ok();
}

TEST("checker_verdict.set_partial_score(-1)", "", Crashes{}) {
    oi::checker_verdict.set_partial_score(-1, "x y z ", 8);
}

TEST("checker_verdict.set_partial_score(101)", "", Crashes{}) {
    oi::checker_verdict.set_partial_score(101, "x y z ", 8);
}

TEST("checker_verdict.set_partial_score(100)", "", Crashes{}) {
    oi::checker_verdict.set_partial_score(100, "x y z ", 8);
}

TEST("checker_verdict.set_partial_score() + exit_wrong()", "", Exits{0, "OK\nx y z 8; 1abc42\n42\n"}) {
    oi::checker_verdict.set_partial_score(42, "x y z ", 8);
    oi::checker_verdict.exit_wrong(1, "abc", 42);
}

TEST("checker_verdict.set_partial_score() x2 + exit_wrong()", "", Exits{0, "OK\na b c 88; 1abc42\n23\n"}) {
    oi::checker_verdict.set_partial_score(42, "x y z ", 8);
    oi::checker_verdict.set_partial_score(23, "a b c", ' ', 88);
    oi::checker_verdict.exit_wrong(1, "abc", 42);
}

TEST("inwer_verdict.exit_ok()", "", Exits{0, ""}) {
    oi::inwer_verdict.exit_ok();
}

TEST("inwer_verdict.exit_ok()", "", Exits{0, "a b c\n"}) {
    oi::inwer_verdict.exit_ok() << 'a' << " b c";
}

TEST("inwer_verdict.exit_wrong()", "", Exits{1, ""}) {
    oi::inwer_verdict.exit_wrong();
}

TEST("inwer_verdict.exit_wrong()", "", Exits{1, "a b c\n"}) {
    oi::inwer_verdict.exit_wrong() << 'a' << " b c";
}

TEST("bug(msg)", "", Exits{2, "BUG: 2 + 2 = 4\n"}) {
    oi::bug("2 + 2 = ", 4);
}

TEST("oi_assert(true)", "", Exits{0, ""}) {
    oi_assert(true);
    oi::inwer_verdict.exit_ok();
}

TEST("oi_assert(true, msg)", "", Exits{0, ""}) {
    oi_assert(true, "2 + 2 = ", 4);
    oi::inwer_verdict.exit_ok();
}

TEST("oi_assert(false)", "", Exits{3, "oi.h:1510: void test_body22(): Assertion `2 + 2 != 4` failed.\n"}) {
    oi_assert(2 + 2 != 4);
}

TEST("oi_assert(false, msg)", "", Exits{3, "oi.h:1514: void test_body23(): Assertion `2 + 2 != 4` failed: 2 + 2 = 4\n"}) {
    oi_assert(2 + 2 != 4, "2 + 2 = ", 4);
}

TEST("Scanner(UserOutput, EN)::error()", "", Exits{0, "WRONG\nLine 1, position 1: 42abc\n0\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN}.error(42, "abc");
}

TEST("Scanner(UserOutput, PL)::error()", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: 42abc\n0\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL}.error(42, "abc");
}

TEST("Scanner(TestInput, EN)::error()", "", Exits{1, "Line 1, position 1: 42abc\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN}.error(42, "abc");
}

TEST("Scanner(TestInput, PL)::error()", "", Exits{1, "Wiersz 1, pozycja 1: 42abc\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL}.error(42, "abc");
}

TEST("Scanner(Lax, EN)::error()", "", Exits{4, "Lax scanner: Line 1, position 1: 42abc\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN}.error(42, "abc");
}

TEST("Scanner(Lax, PL)::error()", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: 42abc\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL}.error(42, "abc");
}

TEST("Scanner(UserOutput, EN)::operator>>(const char&)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected 'x'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> 'x';
}

TEST("Scanner(UserOutput, PL)::operator>>(const char&)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano 'x'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> 'x';
}

TEST("Scanner(TestInput, EN)::operator>>(const char&)", "", Exits{1, "Line 1, position 1: Read EOF, expected 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> 'x';
}

TEST("Scanner(TestInput, PL)::operator>>(const char&)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> 'x';
}

TEST("Scanner(Lax, EN)::operator>>(const char&)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> 'x';
}

TEST("Scanner(Lax, PL)::operator>>(const char&)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> 'x';
}

TEST("Scanner(UserOutput, EN)::operator>>(const char&)", "x", Exits{0, "WRONG\nLine 1, position 2: Read EOF, expected 'x'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(UserOutput, PL)::operator>>(const char&)", "x", Exits{0, "WRONG\nWiersz 1, pozycja 2: Wczytano EOF, oczekiwano 'x'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(TestInput, EN)::operator>>(const char&)", "x", Exits{1, "Line 1, position 2: Read EOF, expected 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(TestInput, PL)::operator>>(const char&)", "x", Exits{1, "Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(Lax, EN)::operator>>(const char&)", "x", Exits{4, "Lax scanner: Line 1, position 2: Read EOF, expected 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(Lax, PL)::operator>>(const char&)", "x", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano 'x'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> 'x';
    s >> 'x';
}

TEST("Scanner(UserOutput, EN)::operator>>(const char&)", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected 'b'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> 'b';
}

TEST("Scanner(UserOutput, PL)::operator>>(const char&)", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano 'b'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> 'b';
}

TEST("Scanner(TestInput, EN)::operator>>(const char&)", "a", Exits{1, "Line 1, position 1: Read 'a', expected 'b'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> 'b';
}

TEST("Scanner(TestInput, PL)::operator>>(const char&)", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano 'b'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> 'b';
}

TEST("Scanner(Lax, EN)::operator>>(const char&)", "a", Exits{4, "Lax scanner: Line 1, position 1: Read 'a', expected 'b'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> 'b';
}

TEST("Scanner(Lax, PL)::operator>>(const char&)", "a", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano 'b'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> 'b';
}

TEST("Scanner(UserOutput, EN)::operator>>(const char&) ignores white space", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(const char&) ignores white space", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(const char&) does not ignore white space", " a", Exits{1, "Line 1, position 1: Read ' ', expected 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> 'a';
}

TEST("Scanner(TestInput, PL)::operator>>(const char&) does not ignore white space", " a", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> 'a';
}

TEST("Scanner(Lax, EN)::operator>>(const char&) ignores white space", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(const char&) ignores white space", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(eof)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::eof >> oi::eof >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(eof)", "", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::eof >> oi::eof >> oi::eof;
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(eof)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::eof >> oi::eof >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(eof)", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(eof)", " ", Exits{1, "Line 1, position 1: Read ' ', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(eof)", " ", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::eof;
}

TEST("Scanner(TestInput, EN)::operator>>(eof)", "\n", Exits{1, "Line 1, position 1: Read '\\n', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(eof)", "\n", Exits{1, "Wiersz 1, pozycja 1: Wczytano '\\n', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::eof;
}

TEST("Scanner(Lax)::operator>>(eof)", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(eof)", "  \n  a", Exits{0, "WRONG\nLine 2, position 3: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::eof;
}

TEST("Scanner(UserOutput, PL)::operator>>(eof)", "  \n  a", Exits{0, "WRONG\nWiersz 2, pozycja 3: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> oi::eof;
}

TEST("Scanner(TestInput, EN)::operator>>(eof)", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(eof)", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::eof;
}

TEST("Scanner(Lax, EN)::operator>>(eof)", "  \n  a", Exits{4, "Lax scanner: Line 2, position 3: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::eof;
}

TEST("Scanner(Lax, PL)::operator>>(eof)", "  \n  a", Exits{4, "Lax scanner: Wiersz 2, pozycja 3: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::eof;
}

TEST("Scanner(UserOutput)::operator>>(whitespace) and eof", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(whitespace) and eof", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and eof", "", Exits{1, "Line 1, position 1: Read EOF, expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> ' ';
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and eof", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> ' ';
    s >> oi::eof;
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and eof", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> '\n';
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and eof", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> '\n';
    s >> oi::eof;
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and eof", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> '\t';
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and eof", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> '\t';
    s >> oi::eof;
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and eof", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
    s >> oi::eof;
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and eof", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::nl;
    s >> oi::eof;
}

TEST("Scanner(Lax)::operator>>(whitespace) and eof", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(whitespace) and eof", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    s >> oi::eof;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(whitespace) and destructor", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(whitespace) and destructor", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and destructor", "", Exits{1, "Line 1, position 1: Read EOF, expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> ' ';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and destructor", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> ' ';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and destructor", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> '\n';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and destructor", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> '\n';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and destructor", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> '\t';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and destructor", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> '\t';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) and destructor", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) and destructor", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::nl;
}

TEST("Scanner(Lax)::operator>>(whitespace) and destructor do nothing", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, EN)::operator>>(whitespace) and destructor do nothing", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl >> 'a';
}

TEST("Scanner(Lax, PL)::operator>>(whitespace) and destructor do nothing", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl >> 'a';
}

TEST("Scanner(Lax)::operator>>(whitespace) and destructor do nothing", "  \n  ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, EN)::operator>>(whitespace) and destructor do nothing", "  \n   ", Exits{4, "Lax scanner: Line 2, position 4: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl >> 'a';
}

TEST("Scanner(Lax, PL)::operator>>(whitespace) and destructor do nothing", "  \n   ", Exits{4, "Lax scanner: Wiersz 2, pozycja 4: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> ' ' >> '\t' >> '\n' >> oi::nl >> ' ' >> '\t' >> '\n' >> oi::nl >> 'a';
}

TEST("Scanner(UserOutput)::operator>>(whitespace) whitespace kind does not matter", " \t\fx", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> '\t' >> ' ' >> ' ' >> 'x';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) whitespace kind does matter", " ", Exits{1, "Line 1, position 1: Read ' ', expected '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> '\t';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) whitespace kind does matter", " ", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano '\\t'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> '\t';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) whitespace kind does matter", "\t", Exits{1, "Line 1, position 1: Read '\\t', expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> ' ';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) whitespace kind does matter", "\t", Exits{1, "Wiersz 1, pozycja 1: Wczytano '\\t', oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> ' ';
}

TEST("Scanner(Lax)::operator>>(whitespace) whitespace kind does not matter", " \t\fx", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> '\t' >> ' ' >> ' ' >> 'x';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(whitespace) whitespace number matters", "  x\n  x", Exits{0, "WRONG\nLine 2, position 3: Read 'x', expected ' '\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> ' ' >> ' ' >> 'x' >> oi::nl;
    s >> ' ' >> ' ' >> ' ' >> 'x';
}

TEST("Scanner(UserOutput, PL)::operator>>(whitespace) whitespace number matters", "  x\n  x", Exits{0, "WRONG\nWiersz 2, pozycja 3: Wczytano 'x', oczekiwano ' '\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> ' ' >> ' ' >> 'x' >> oi::nl;
    s >> ' ' >> ' ' >> ' ' >> 'x';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) whitespace number matters - too little", " ", Exits{1, "Line 1, position 2: Read EOF, expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> ' ' >> ' ';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) whitespace number matters - too little", " ", Exits{1, "Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> ' ' >> ' ';
}

TEST("Scanner(TestInput, EN)::operator>>(whitespace) whitespace number matters - too much", "  a", Exits{1, "Line 1, position 2: Read ' ', expected 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> ' ' >> 'a';
}

TEST("Scanner(TestInput, PL)::operator>>(whitespace) whitespace number matters - too much", "  a", Exits{1, "Wiersz 1, pozycja 2: Wczytano ' ', oczekiwano 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> ' ' >> 'a';
}

TEST("Scanner(Lax, EN)::operator>>(whitespace) whitespace number matters", "  x\n  x", Exits{4, "Lax scanner: Line 2, position 3: Read 'x', expected ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> ' ' >> ' ' >> 'x' >> oi::nl;
    s >> ' ' >> ' ' >> ' ' >> 'x';
}

TEST("Scanner(Lax, PL)::operator>>(whitespace) whitespace number matters", "  x\n  x", Exits{4, "Lax scanner: Wiersz 2, pozycja 3: Wczytano 'x', oczekiwano ' '\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> ' ' >> ' ' >> 'x' >> oi::nl;
    s >> ' ' >> ' ' >> ' ' >> 'x';
}

TEST("Scanner(UserOutput, EN)::operator>>(nl)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(nl)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(nl)", "", Exits{1, "Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
}

TEST("Scanner(TestInput, PL)::operator>>(nl)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::nl;
}

TEST("Scanner(Lax, EN)::operator>>(nl)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(nl)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, EN)::operator>>(nl)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl >> 'x';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(nl)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::nl >> 'x';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(nl)", "\n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(nl)", "\n", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(nl)", "\n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(nl)", "   \n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(nl)", "   \n", Exits{1, "Line 1, position 1: Read ' ', expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
}

TEST("Scanner(TestInput, PL)::operator>>(nl)", "   \n", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::nl;
}

TEST("Scanner(Lax)::operator>>(nl)", "   \n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(nl)", "   a", Exits{0, "WRONG\nLine 1, position 4: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::nl;
}

TEST("Scanner(UserOutput, PL)::operator>>(nl)", "   a", Exits{0, "WRONG\nWiersz 1, pozycja 4: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> oi::nl;
}

TEST("Scanner(TestInput, EN)::operator>>(nl)", "a", Exits{1, "Line 1, position 1: Read 'a', expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::nl;
}

TEST("Scanner(TestInput, PL)::operator>>(nl)", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::nl;
}

TEST("Scanner(Lax, EN)::operator>>(nl)", "   a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(nl)", "   a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::nl;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, EN)::operator>>(nl)", "   a", Exits{4, "Lax scanner: Line 1, position 4: Read 'a', expected '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::nl >> 'b';
}

TEST("Scanner(Lax, PL)::operator>>(nl)", "   a", Exits{4, "Lax scanner: Wiersz 1, pozycja 4: Wczytano 'a', oczekiwano '\\n'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::nl >> 'b';
}

TEST("Scanner(UserOutput)::operator>>(ignore_ws) ignoring before EOF is no-op", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::ignore_ws;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(ignore_ws) ignoring before EOF is no-op", "", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::ignore_ws;
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(ignore_ws) ignoring before EOF is no-op", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::ignore_ws;
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{0, "WRONG\nLine 1, position 3: Read '\\n', expected 'a'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(UserOutput, PL)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{0, "WRONG\nWiersz 1, pozycja 3: Wczytano '\\n', oczekiwano 'a'\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(TestInput, EN)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{1, "Line 1, position 3: Read '\\n', expected 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(TestInput, PL)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{1, "Wiersz 1, pozycja 3: Wczytano '\\n', oczekiwano 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(Lax, EN)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{4, "Lax scanner: Line 1, position 3: Read '\\n', expected 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(Lax, PL)::operator>>(ignore_ws) does not ignore newline", "  \na", Exits{4, "Lax scanner: Wiersz 1, pozycja 3: Wczytano '\\n', oczekiwano 'a'\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    s >> oi::ignore_ws >> 'a';
}

TEST("Scanner(UserOutput)::operator>>(ignore_ws) ignoring works", "   aa", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a' >> oi::ignore_ws >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(ignore_ws) ignoring works", "   aa", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a' >> oi::ignore_ws >> 'a';
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(ignore_ws) ignoring works", "   aa", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    s >> oi::ignore_ws >> 'a' >> oi::ignore_ws >> 'a';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Line)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != "") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Line)", "", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != "") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Line)", "", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != "") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Line)", "\n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != "") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Line)", "\n", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != "") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Line)", "\n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != "") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Line)", " a b c ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != " a b c ") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Line)", " a b c ", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != " a b c ") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Line)", " a b c ", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10};
    if (x != " a b c ") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Line)", " a b c \n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != " a b c ") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Line)", " a b c \n", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != " a b c ") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Line)", " a b c \n", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Line{x, 10} >> '\n';
    if (x != " a b c ") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Str)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected a string\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(UserOutput, PL)::operator>>(Str)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano napisu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(TestInput, EN)::operator>>(Str)", "", Exits{1, "Line 1, position 1: Read EOF, expected a string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(TestInput, PL)::operator>>(Str)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano napisu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(Lax, EN)::operator>>(Str)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected a string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(Lax, PL)::operator>>(Str)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano napisu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(UserOutput, EN)::operator>>(Str)", " ", Exits{0, "WRONG\nLine 1, position 2: Read EOF, expected a string\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(UserOutput, PL)::operator>>(Str)", " ", Exits{0, "WRONG\nWiersz 1, pozycja 2: Wczytano EOF, oczekiwano napisu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(TestInput, EN)::operator>>(Str)", " ", Exits{1, "Line 1, position 1: Read ' ', expected a string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(TestInput, PL)::operator>>(Str)", " ", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano napisu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(Lax, EN)::operator>>(Str)", " ", Exits{4, "Lax scanner: Line 1, position 2: Read EOF, expected a string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(Lax, PL)::operator>>(Str)", " ", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano napisu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 10};
}

TEST("Scanner(UserOutput)::operator>>(Str)", "abc", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
    if (x != "abc") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Str)", "abc", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
    if (x != "abc") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Str)", "abc", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10};
    if (x != "abc") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Str)", "abc d", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10} >> ' ' >> 'd';
    if (x != "abc") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Str)", "abc d", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10} >> ' ' >> 'd';
    if (x != "abc") { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Str)", "abc d", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 10} >> ' ' >> 'd';
    if (x != "abc") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Str)", "abcefg efg", Exits{0, "WRONG\nLine 1, position 6: Too long string\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(UserOutput, PL)::operator>>(Str)", "abcefg efg", Exits{0, "WRONG\nWiersz 1, pozycja 6: Zbyt dlugi napis\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(TestInput, EN)::operator>>(Str)", "abcefg efg", Exits{1, "Line 1, position 6: Too long string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(TestInput, PL)::operator>>(Str)", "abcefg efg", Exits{1, "Wiersz 1, pozycja 6: Zbyt dlugi napis\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(Lax, EN)::operator>>(Str)", "abcefg efg", Exits{4, "Lax scanner: Line 1, position 6: Too long string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(Lax, PL)::operator>>(Str)", "abcefg efg", Exits{4, "Lax scanner: Wiersz 1, pozycja 6: Zbyt dlugi napis\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(UserOutput, EN)::operator>>(Str)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
    if (x != "a") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(Str)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
    if (x != "a") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(Str)", " a", Exits{1, "Line 1, position 1: Read ' ', expected a string\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(TestInput, PL)::operator>>(Str)", " a", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano napisu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
}

TEST("Scanner(Lax, EN)::operator>>(Str)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    string x;
    s >> oi::Str{x, 5};
    if (x != "a") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(Str)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    string x;
    s >> oi::Str{x, 5};
    if (x != "a") { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator(Char)", "a1c", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "a1c"};
    if (c != 'a') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != '1') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != 'c') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator(Char)", "a1c", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "a1c"};
    if (c != 'a') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != '1') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != 'c') { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator(Char)", "a1c", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "a1c"};
    if (c != 'a') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != '1') { std::terminate(); }
    s >> oi::Char{c, "a1c"};
    if (c != 'c') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Char)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected one of characters: abc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(UserOutput, PL)::operator>>(Char)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano jednego ze znakow: abc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(TestInput, EN)::operator>>(Char)", "", Exits{1, "Line 1, position 1: Read EOF, expected one of characters: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(TestInput, PL)::operator>>(Char)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano jednego ze znakow: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(Lax, EN)::operator>>(Char)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected one of characters: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(Lax, PL)::operator>>(Char)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano jednego ze znakow: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(UserOutput, EN)::operator>>(Char)", "A", Exits{0, "WRONG\nLine 1, position 1: Read 'A', expected one of characters: abc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(UserOutput, PL)::operator>>(Char)", "A", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'A', oczekiwano jednego ze znakow: abc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(TestInput, EN)::operator>>(Char)", "A", Exits{1, "Line 1, position 1: Read 'A', expected one of characters: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(TestInput, PL)::operator>>(Char)", "A", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'A', oczekiwano jednego ze znakow: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(Lax, EN)::operator>>(Char)", "A", Exits{4, "Lax scanner: Line 1, position 1: Read 'A', expected one of characters: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(Lax, PL)::operator>>(Char)", "A", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano 'A', oczekiwano jednego ze znakow: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(UserOutput, EN)::operator>>(Char)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
    if (c != 'a') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(Char)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
    if (c != 'a') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(Char)", " a", Exits{1, "Line 1, position 1: Read ' ', expected one of characters: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(TestInput, PL)::operator>>(Char)", " a", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano jednego ze znakow: abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
}

TEST("Scanner(Lax, EN)::operator>>(Char)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    char c;
    s >> oi::Char{c, "abc"};
    if (c != 'a') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(Char)", " a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    char c;
    s >> oi::Char{c, "abc"};
    if (c != 'a') { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected a number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>)", "", Exits{1, "Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>)", "-", Exits{0, "WRONG\nLine 1, position 2: Read EOF, expected a number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>)", "-", Exits{0, "WRONG\nWiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>)", "-", Exits{1, "Line 1, position 2: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>)", "-", Exits{1, "Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>)", "-", Exits{4, "Lax scanner: Line 1, position 2: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>)", "-", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput)::operator>>(Num<int>)", "0\n42\n-1337\n-2147483648\n2147483647", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -1337) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -2147483648) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 2147483647) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<int>)", "0\n42\n-1337\n-2147483648\n2147483647", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -1337) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -2147483648) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 2147483647) { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<int>)", "0\n42\n-1337\n-2147483648\n2147483647", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -1337) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != -2147483648) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 2147483647) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{0, "WRONG\nLine 1, position 12: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{0, "WRONG\nWiersz 1, pozycja 12: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{1, "Line 1, position 12: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{1, "Wiersz 1, pozycja 12: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{4, "Lax scanner: Line 1, position 12: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>) multiply by 10 negative overflow", "-21474836480", Exits{4, "Lax scanner: Wiersz 1, pozycja 12: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{0, "WRONG\nLine 1, position 11: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{0, "WRONG\nWiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{1, "Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{1, "Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{4, "Lax scanner: Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>) multiply by 10 positive overflow", "21474836470", Exits{4, "Lax scanner: Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{0, "WRONG\nLine 1, position 10: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{0, "WRONG\nWiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{1, "Line 1, position 10: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{1, "Wiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{4, "Lax scanner: Line 1, position 10: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>) addition positive overflow", "2147483648", Exits{4, "Lax scanner: Wiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{0, "WRONG\nLine 1, position 11: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{0, "WRONG\nWiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{1, "Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{1, "Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{4, "Lax scanner: Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>) addition negative overflow", "-2147483649", Exits{4, "Lax scanner: Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput)::operator>>(Num<int>)", "255 256 257", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<int>)", "255 256 257", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<int>)", "255 256 257", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>)", "-8", Exits{0, "WRONG\nLine 1, position 2: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>)", "-8", Exits{0, "WRONG\nWiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>)", "-8", Exits{1, "Line 1, position 2: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>)", "-8", Exits{1, "Wiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>)", "-8", Exits{4, "Lax scanner: Line 1, position 2: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>)", "-8", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>)", "8", Exits{0, "WRONG\nLine 1, position 1: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>)", "8", Exits{0, "WRONG\nWiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>)", "8", Exits{1, "Line 1, position 1: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>)", "8", Exits{1, "Wiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>)", "8", Exits{4, "Lax scanner: Line 1, position 1: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>)", "8", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, -7, 7};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<int>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<int>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(Num<int>) whitespace is not ignored", " 0", Exits{1, "Line 1, position 1: Read ' ', expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<int>) whitespace is not ignored", " 0", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(Lax, EN)::operator>>(Num<int>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    int32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(Num<int>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    int32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected a number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>)", "", Exits{1, "Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>)", "-", Exits{0, "WRONG\nLine 1, position 1: Read '-', expected a non-negative number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>)", "-", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano '-', oczekiwano nieujemnej liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>)", "-", Exits{1, "Line 1, position 1: Read '-', expected a non-negative number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>)", "-", Exits{1, "Wiersz 1, pozycja 1: Wczytano '-', oczekiwano nieujemnej liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>)", "-", Exits{4, "Lax scanner: Line 1, position 1: Read '-', expected a non-negative number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>)", "-", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano '-', oczekiwano nieujemnej liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput)::operator>>(Num<uint>)", "0\n42\n4294967295", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 4294967295) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<uint>)", "0\n42\n4294967295", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 4294967295) { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<uint>)", "0\n42\n4294967295", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 42) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
    if (x != 4294967295) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{0, "WRONG\nLine 1, position 11: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{0, "WRONG\nWiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{1, "Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{1, "Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{4, "Lax scanner: Line 1, position 11: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>) multiply by 10 overflow", "42949672950", Exits{4, "Lax scanner: Wiersz 1, pozycja 11: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{0, "WRONG\nLine 1, position 10: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{0, "WRONG\nWiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{1, "Line 1, position 10: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{1, "Wiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{4, "Lax scanner: Line 1, position 10: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>) addition overflow", "4294967296", Exits{4, "Lax scanner: Wiersz 1, pozycja 10: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::min(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput)::operator>>(Num<uint>)", "255 256 257", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<uint>)", "255 256 257", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<uint>)", "255 256 257", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 255) { std::terminate(); }
    s >> oi::Num{x, 255, 257} >> ' ';
    if (x != 256) { std::terminate(); }
    s >> oi::Num{x, 255, 257};
    if (x != 257) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>)", "6", Exits{0, "WRONG\nLine 1, position 1: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>)", "6", Exits{0, "WRONG\nWiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>)", "6", Exits{1, "Line 1, position 1: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>)", "6", Exits{1, "Wiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>)", "6", Exits{4, "Lax scanner: Line 1, position 1: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>)", "6", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>)", "78", Exits{0, "WRONG\nLine 1, position 2: Integer value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>)", "78", Exits{0, "WRONG\nWiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>)", "78", Exits{1, "Line 1, position 2: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>)", "78", Exits{1, "Wiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>)", "78", Exits{4, "Lax scanner: Line 1, position 2: Integer value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>)", "78", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Liczba calkowita spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 7, 77};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<uint>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<uint>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(Num<uint>) whitespace is not ignored", " 0", Exits{1, "Line 1, position 1: Read ' ', expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<uint>) whitespace is not ignored", " 0", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(Lax, EN)::operator>>(Num<uint>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(Num<uint>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    uint32_t x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (x != 0) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592653589793238462643383279502.0) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -0.0000000000000000000000000003141592653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / 3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()} >> '\n';
    if (std::abs(1 - x / -3141592.653589793238462643383279502) > 1e-12) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>)", "", Exits{0, "WRONG\nLine 1, position 1: Read EOF, expected a number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>)", "", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>)", "", Exits{1, "Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>)", "", Exits{1, "Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>)", "", Exits{4, "Lax scanner: Line 1, position 1: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>)", "", Exits{4, "Lax scanner: Wiersz 1, pozycja 1: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>)", "-", Exits{0, "WRONG\nLine 1, position 2: Read EOF, expected a number\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>)", "-", Exits{0, "WRONG\nWiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>)", "-", Exits{1, "Line 1, position 2: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>)", "-", Exits{1, "Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>)", "-", Exits{4, "Lax scanner: Line 1, position 2: Read EOF, expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>)", "-", Exits{4, "Lax scanner: Wiersz 1, pozycja 2: Wczytano EOF, oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{0, "WRONG\nLine 1, position 309: Real number value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{0, "WRONG\nWiersz 1, pozycja 309: Liczba rzeczywista spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{1, "Line 1, position 309: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{1, "Wiersz 1, pozycja 309: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{4, "Lax scanner: Line 1, position 309: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>) positive overflow", "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{4, "Lax scanner: Wiersz 1, pozycja 309: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{0, "WRONG\nLine 1, position 310: Real number value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{0, "WRONG\nWiersz 1, pozycja 310: Liczba rzeczywista spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{1, "Line 1, position 310: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{1, "Wiersz 1, pozycja 310: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{4, "Lax scanner: Line 1, position 310: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>) negative overflow", "-999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", Exits{4, "Lax scanner: Wiersz 1, pozycja 310: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), std::numeric_limits<decltype(x)>::max()};
}

TEST("Scanner(UserOutput)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 3141592653580000000000000000000000.0, 3141592653590000000000000000000000.0} >> '\n';
    s >> oi::Num{x, -3141592653590000000000000000000000.0, -3141592653580000000000000000000000.0} >> '\n';
    s >> oi::Num{x, 0.000000000000000000000000000314159265358, 0.000000000000000000000000000314159265359} >> '\n';
    s >> oi::Num{x, -0.000000000000000000000000000314159265359, -0.000000000000000000000000000314159265358} >> '\n';
    s >> oi::Num{x, 3141592.65358, 3141592.65359} >> '\n';
    s >> oi::Num{x, -3141592.65359, -3141592.65358} >> '\n';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 3141592653580000000000000000000000.0, 3141592653590000000000000000000000.0} >> '\n';
    s >> oi::Num{x, -3141592653590000000000000000000000.0, -3141592653580000000000000000000000.0} >> '\n';
    s >> oi::Num{x, 0.000000000000000000000000000314159265358, 0.000000000000000000000000000314159265359} >> '\n';
    s >> oi::Num{x, -0.000000000000000000000000000314159265359, -0.000000000000000000000000000314159265358} >> '\n';
    s >> oi::Num{x, 3141592.65358, 3141592.65359} >> '\n';
    s >> oi::Num{x, -3141592.65359, -3141592.65358} >> '\n';
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax)::operator>>(Num<double>)", R"(0
-0
3141592653589793238462643383279502
-3141592653589793238462643383279502
0.0000000000000000000000000003141592653589793238462643383279502
-0.0000000000000000000000000003141592653589793238462643383279502
3141592.653589793238462643383279502
-3141592.653589793238462643383279502
)", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 0, 0} >> '\n';
    s >> oi::Num{x, 3141592653580000000000000000000000.0, 3141592653590000000000000000000000.0} >> '\n';
    s >> oi::Num{x, -3141592653590000000000000000000000.0, -3141592653580000000000000000000000.0} >> '\n';
    s >> oi::Num{x, 0.000000000000000000000000000314159265358, 0.000000000000000000000000000314159265359} >> '\n';
    s >> oi::Num{x, -0.000000000000000000000000000314159265359, -0.000000000000000000000000000314159265358} >> '\n';
    s >> oi::Num{x, 3141592.65358, 3141592.65359} >> '\n';
    s >> oi::Num{x, -3141592.65359, -3141592.65358} >> '\n';
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{0, "WRONG\nLine 1, position 35: Real number value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{0, "WRONG\nWiersz 1, pozycja 35: Liczba rzeczywista spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{1, "Line 1, position 35: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{1, "Wiersz 1, pozycja 35: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{4, "Lax scanner: Line 1, position 35: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>)", "3141592.653589793238462643383279502", Exits{4, "Lax scanner: Wiersz 1, pozycja 35: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 3141592.65359, std::numeric_limits<decltype(x)>::max()} >> '\n';
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{0, "WRONG\nLine 1, position 36: Real number value out of range\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{0, "WRONG\nWiersz 1, pozycja 36: Liczba rzeczywista spoza zakresu\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{1, "Line 1, position 36: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{1, "Wiersz 1, pozycja 36: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{4, "Lax scanner: Line 1, position 36: Real number value out of range\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>)", "-3141592.653589793238462643383279502", Exits{4, "Lax scanner: Wiersz 1, pozycja 36: Liczba rzeczywista spoza zakresu\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, std::numeric_limits<decltype(x)>::lowest(), -3141592.65359} >> '\n';
}

TEST("Scanner(UserOutput, EN)::operator>>(Num<double>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::operator>>(Num<double>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::operator>>(Num<double>) whitespace is not ignored", " 0", Exits{1, "Line 1, position 1: Read ' ', expected a number\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(TestInput, PL)::operator>>(Num<double>) whitespace is not ignored", " 0", Exits{1, "Wiersz 1, pozycja 1: Wczytano ' ', oczekiwano liczby\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 0, 0};
}

TEST("Scanner(Lax, EN)::operator>>(Num<double>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    double x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL)::operator>>(Num<double>) whitespace is ignored", " 0", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    double x;
    s >> oi::Num{x, 0, 0};
    if (x != 0) { std::terminate(); }
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::~Scanner() scans eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
}

TEST("Scanner(UserOutput, PL)::~Scanner() scans eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
}

TEST("Scanner(TestInput, EN)::~Scanner() scans eof", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
}

TEST("Scanner(TestInput, PL)::~Scanner() scans eof", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
}

TEST("Scanner(Lax)::~Scanner() does not scan eof", "a", Exits{0, "OK\n\n100\n"}) {
    oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

#undef exit
TEST("Scanner(UserOutput, EN) exit() scans eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    exit(42);
}

TEST("Scanner(UserOutput, PL) exit() scans eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    exit(42);
}

TEST("Scanner(TestInput, EN) exit() scans eof", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    exit(42);
}

TEST("Scanner(TestInput, PL) exit() scans eof", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    exit(42);
}

TEST("Scanner(Lax) exit() does not scan eof", "a", Exits{42, ""}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    exit(42);
}

TEST("Scanner(UserOutput, EN) checker_verdict.exit_ok() scans eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL) checker_verdict.exit_ok() scans eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN) inwer_verdict.ok() scans eof", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(TestInput, PL) inwer_verdict.ok() scans eof", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax) checker_verdict.exit_ok() does not scan eof", "a", Exits{0, "OK\n\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN) checker_verdict.exit_ok_with_score(100, msg) scans eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok_with_score(100, "abc");
}

TEST("Scanner(UserOutput, PL) checker_verdict.exit_ok_with_score(100, msg) scans eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    oi::checker_verdict.exit_ok_with_score(100, "abc");
}

TEST("Scanner(Lax) checker_verdict.exit_ok_with_score(100, msg) does not scan eof", "a", Exits{0, "OK\nabc\n100\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_ok_with_score(100, "abc");
}

TEST("Scanner(UserOutput) checker_verdict.exit_ok_with_score(99, msg) does not scan eof", "a", Exits{0, "OK\nabc\n99\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok_with_score(99, "abc");
}

TEST("Scanner(Lax) checker_verdict.exit_ok_with_score(99, msg) does not scan eof", "a", Exits{0, "OK\nabc\n99\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_ok_with_score(99, "abc");
}

TEST("Scanner(UserOutput) partial score == 99 checker_verdict.wrong() does not scan eof", "a", Exits{0, "OK\nabc; xyz\n99\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.set_partial_score(99, "abc");
    oi::checker_verdict.exit_wrong("xyz");
}

TEST("Scanner(Lax) partial score == 99 checker_verdict.wrong() does not scan eof", "a", Exits{0, "OK\nabc; xyz\n99\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.set_partial_score(99, "abc");
    oi::checker_verdict.exit_wrong("xyz");
}

TEST("Scanner(UserOutput) checker_verdict.wrong() does not scan eof", "a", Exits{0, "WRONG\nabc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_wrong("abc");
}

TEST("Scanner(TestInput) inwer_verdict.exit_wrong() does not scan eof", "a", Exits{1, "abc\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    oi::inwer_verdict.exit_wrong() << "abc";
}

TEST("Scanner(Lax) checker_verdict.wrong() does not scan eof", "a", Exits{0, "WRONG\nabc\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_wrong("abc");
}

TEST("Scanner(UserOutput, EN)::constructor(FILE*) makes checker_verdict.exit_ok() scan eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::constructor(FILE*) makes checker_verdict.exit_ok() scan eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::constructor(FILE*) makes inwer_verdict.ok() scan eof", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(TestInput, PL)::constructor(FILE*) makes inwer_verdict.ok() scan eof", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN)::constructor(const char*) makes checker_verdict.exit_ok() scan eof", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    auto s = oi::Scanner{"/dev/stdin", oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL)::constructor(const char*) makes checker_verdict.exit_ok() scan eof", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    auto s = oi::Scanner{"/dev/stdin", oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN)::constructor(const char*) makes inwer_verdict.ok() scan eof", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    auto s = oi::Scanner{"/dev/stdin", oi::Scanner::Mode::TestInput, oi::Lang::EN};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(TestInput, PL)::constructor(const char*) makes inwer_verdict.ok() scan eof", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    auto s = oi::Scanner{"/dev/stdin", oi::Scanner::Mode::TestInput, oi::Lang::PL};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN) destructed before checker_verdict.exit_ok() is ok", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::UserOutput, oi::Lang::EN);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, EN) destructed before checker_verdict.exit_ok() is ok", "a", Exits{0, "WRONG\nLine 1, position 1: Read 'a', expected EOF\n0\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::UserOutput, oi::Lang::EN);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(UserOutput, PL) destructed before checker_verdict.exit_ok() is ok", "a", Exits{0, "WRONG\nWiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n0\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::UserOutput, oi::Lang::PL);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::UserOutput, oi::Lang::PL};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(TestInput, EN) destructed before inwer_verdict.ok() is ok", "a", Exits{1, "Line 1, position 1: Read 'a', expected EOF\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::TestInput, oi::Lang::EN);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::EN};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(TestInput, PL) destructed before inwer_verdict.ok() is ok", "a", Exits{1, "Wiersz 1, pozycja 1: Wczytano 'a', oczekiwano EOF\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::TestInput, oi::Lang::PL);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::TestInput, oi::Lang::PL};
    oi::inwer_verdict.exit_ok();
}

TEST("Scanner(Lax, EN) destructed before checker_verdict.exit_ok() is ok", "a", Exits{0, "OK\n\n100\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::Lax, oi::Lang::EN);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::EN};
    oi::checker_verdict.exit_ok();
}

TEST("Scanner(Lax, PL) destructed before checker_verdict.exit_ok() is ok", "a", Exits{0, "OK\n\n100\n"}) {
    void* mem = mmap(nullptr, sizeof(oi::Scanner), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::terminate(); }
    auto dev_null_scanner = new (mem) oi::Scanner("/dev/null", oi::Scanner::Mode::Lax, oi::Lang::PL);
    dev_null_scanner->~Scanner();
    if (munmap(mem, sizeof(oi::Scanner))) {
        std::terminate();
    }

    auto s = oi::Scanner{stdin, oi::Scanner::Mode::Lax, oi::Lang::PL};
    oi::checker_verdict.exit_ok();
}

template<class T, class Rand, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void distributes_evenly(T min, T max, size_t reps, Rand rnd) {
    vector<int> count(static_cast<size_t>(max - min + 1));
    for (size_t i = 0; i < reps; ++i) {
        auto val = rnd();
        if (val < min || val > max) { std::terminate(); }
        ++count[static_cast<size_t>(val - min)];
    }
    oi_assert(reps >= 100 * count.size());
    auto diff = *std::max_element(count.begin(), count.end()) - *std::min_element(count.begin(), count.end());
    std::cerr << diff << ' ' << diff / (static_cast<double>(reps) / static_cast<double>(count.size())) << '\n';
    if (diff > static_cast<double>(reps) / static_cast<double>(count.size()) * 0.4) {
        std::terminate();
    }
}

template<class T, class Rand, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
void distributes_evenly(T min, T max, size_t reps, Rand rnd) {
    std::set<T> vals;
    for (size_t i = 0; i < reps; ++i) {
        auto val = rnd();
        if (val < min || val > max) { std::terminate(); }
        vals.emplace(val);
    }

    T max_consecutive_diff = 0;
    auto last = min;
    vals.emplace(max);
    for (auto x : vals) {
        max_consecutive_diff = std::max(x - last, max_consecutive_diff);
        last = x;
    }

    std::cerr << max_consecutive_diff << ' ' << max_consecutive_diff / ((max - min) / static_cast<double>(reps)) << std::endl;
    if (max_consecutive_diff > (max - min) / static_cast<double>(reps) * 20) {
        std::terminate();
    }
}

void test_random() {
    {
        oi::Random rd;
        distributes_evenly(10, 20, 10000, [&] { return rd(10, 20); });
    }
    {
        oi::Random rd;
        distributes_evenly<int8_t>(-128, 127, 100000, [&] { return rd.operator()<int8_t>(-128, 127); });
    }
    {
        oi::Random rd;
        distributes_evenly<int8_t>(-128, 127, 100000, [&] { return rd.operator()<int8_t>(-128, 127); });
    }
    {
        oi::Random rd;
        distributes_evenly(-2.78, 3.14, 10000, [&] { return rd(-2.78, 3.14); });
    }
    {
        oi::Random rd;
        constexpr int N = 10000;
        std::map<vector<int>, int> count;
        for (int i = 0; i < N; ++i) {
            vector<int> v = {1, 2, 3, 4, 5};
            rd.shuffle(v);
            ++count[v];
        }
        if (count.size() != 120) { std::terminate(); }
        auto min_count = count.begin()->second;
        auto max_count = count.begin()->second;
        for (auto&& [v, c] : count) {
            min_count = std::min(c, min_count);
            max_count = std::max(c, max_count);
        }
        auto diff = max_count - min_count;
        std::cerr << diff << ' ' << static_cast<double>(diff) / (static_cast<double>(N) / 120) << '\n';
        if (diff > static_cast<double>(N) / 120 * 0.5) {
            std::terminate();
        }
    }
}

int main() {
    oi::detail::change_error_ostream_to_cout();
    for (auto test_fn : get_test_fns()) {
        test_fn();
    }
    test_random();
    (void)fputs("All tests passed\n", stdout);
    return 0;
}

#endif // OI_H_TESTS
