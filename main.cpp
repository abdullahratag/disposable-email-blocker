#include <array>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

bool isLetterOrDigit(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
}

// Every invalid transition enters Dead, which never accepts.
enum class FormatState {
    Start, LocalCharacter, LocalDot, DomainStart,
    DomainWithoutDot, DomainHyphenWithoutDot,
    DomainWithDot, DomainSeparatorWithDot, Dead
};

FormatState transition(FormatState state, char ch) {
    const bool alphanumeric = isLetterOrDigit(ch);
    switch (state) {
    case FormatState::Start:
        return alphanumeric ? FormatState::LocalCharacter : FormatState::Dead;
    case FormatState::LocalCharacter:
        if (alphanumeric) return state;
        if (ch == '.') return FormatState::LocalDot;
        if (ch == '@') return FormatState::DomainStart;
        return FormatState::Dead;
    case FormatState::LocalDot:
        if (alphanumeric) return FormatState::LocalCharacter;
        return ch == '.' ? state : FormatState::Dead;
    case FormatState::DomainStart:
        return alphanumeric ? FormatState::DomainWithoutDot : FormatState::Dead;
    case FormatState::DomainWithoutDot:
    case FormatState::DomainHyphenWithoutDot:
        if (alphanumeric) return FormatState::DomainWithoutDot;
        if (ch == '-') return FormatState::DomainHyphenWithoutDot;
        if (ch == '.') return FormatState::DomainSeparatorWithDot;
        return FormatState::Dead;
    case FormatState::DomainWithDot:
    case FormatState::DomainSeparatorWithDot:
        if (alphanumeric) return FormatState::DomainWithDot;
        if (ch == '.' || ch == '-') return FormatState::DomainSeparatorWithDot;
        return FormatState::Dead;
    case FormatState::Dead:
        return FormatState::Dead;
    }
    return FormatState::Dead;
}

// A trie of reversed provider names is a finite transition table.
// Missing transitions lead to an implicit safe sink: no provider can match.
class ProviderDfa {
    struct State {
        std::map<char, std::size_t> next;
        bool terminal = false;
    };
    std::vector<State> states{State{}};

public:
    ProviderDfa() {
        const std::array<std::string, 4> providers = {
            "mozmail.com", "mailinator.com", "guerrillamail.com", "10minutemail.com"
        };
        for (const auto& provider : providers) {
            std::size_t state = 0;
            for (auto it = provider.rbegin(); it != provider.rend(); ++it) {
                const auto found = states[state].next.find(*it);
                if (found != states[state].next.end()) {
                    state = found->second;
                } else {
                    const std::size_t next = states.size();
                    states[state].next.emplace(*it, next);
                    states.push_back(State{});
                    state = next;
                }
            }
            states[state].terminal = true;
        }
    }

    bool blocks(const std::string& email, std::size_t domainBegin) const {
        std::size_t state = 0;
        for (std::size_t position = email.size(); position > domainBegin;) {
            --position;
            const auto found = states[state].next.find(email[position]);
            if (found == states[state].next.end()) return false;
            state = found->second;
            // Only an exact domain or a dot boundary identifies a provider.
            if (states[state].terminal &&
                (position == domainBegin || email[position - 1] == '.')) {
                return true;
            }
        }
        return false;
    }
};

} // namespace

// Public entry point: accepts exactly the valid, non-blocklisted emails.
bool accepts(const std::string& s) {
    FormatState state = FormatState::Start;
    std::size_t domainBegin = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        state = transition(state, s[i]);
        if (state == FormatState::Dead) return false;
        if (state == FormatState::DomainStart) domainBegin = i + 1;
    }
    if (state != FormatState::DomainWithDot) return false;

    static const ProviderDfa providerDfa;
    return !providerDfa.blocks(s, domainBegin);
}

namespace {

int runSelfTests() {
    const std::vector<std::pair<std::string, bool>> tests = {
        {"alice@gmail.com", true},
        {"bob@mozmail.com", false},
        {"carol@inbox.mozmail.com", false},
        {"dave@a.b.mailinator.com", false},
        {"eve@mozmail.com.evil.org", true},
        {"frank@notmozmail.com", true},
        {"grace@10minutemail.com", false},
        {"heidi@tempmail.com", true},
        {"x@mailinator.com", false},
        {"x@guerrillamail.com", false},
        {"x@a.mozmail.com", false},
        {"x@a.mailinator.com", false},
        {"x@a.guerrillamail.com", false},
        {"x@a.10minutemail.com", false},
        {"x@notmailinator.com", true},
        {"x@notguerrillamail.com", true},
        {"x@not10minutemail.com", true},
        {"x@mozmail.comx", true},
        {"x@mozmail.com.evil.org", true},
        {"x@mailinator.com.evil.org", true},
        {"x@guerrillamail.com.evil.org", true},
        {"x@10minutemail.com.evil.org", true},
        {"x@a-mozmail.com", true},
        {"x@a.notmozmail.com", true},
        {"x@a..mozmail.com", false},
        {"x@a-.mozmail.com", false},
        {"mozmail.com@gmail.com", true},
        {"a@b.c", true},
        {"0@1.2", true},
        {"a..b@x..com", true},
        {"a@x.-y", true},
        {"a@x-.y", true},
        {"a@x--y.com", true},
        {"a.b2@x-y.example.com", true},
        {"", false},
        {"a", false},
        {"@", false},
        {"@b.com", false},
        {"a@", false},
        {"a@b", false},
        {"a@.", false},
        {"a@-", false},
        {".a@b.com", false},
        {"a.@b.com", false},
        {"a..@b.com", false},
        {"a_b@b.com", false},
        {"a+b@b.com", false},
        {"a-b@b.com", false},
        {"A@b.com", false},
        {"a@B.com", false},
        {"a@.b.com", false},
        {"a@-b.com", false},
        {"a@b.com.", false},
        {"a@b.com-", false},
        {"a@b_com", false},
        {"a@@b.com", false},
        {"a@b@c.com", false},
        {" a@b.com", false},
        {"a@b.com ", false},
        {"a@b.com\n", false},
        {"a\tb@c.com", false},
        {std::string("a@b.com\0", 8), false},
        {std::string(10000, 'a') + "@example.com", true},
        {"a@" + std::string(10000, 'a') + ".mozmail.com", false},
        {"a@" + std::string(10000, 'a') + ".example.com", true}
    };
    std::size_t failures = 0;
    for (std::size_t i = 0; i < tests.size(); ++i) {
        if (accepts(tests[i].first) != tests[i].second) {
            std::cerr << "FAIL: test " << i + 1 << '\n';
            ++failures;
        }
    }
    std::cout << tests.size() - failures << '/' << tests.size() << " tests passed.\n";
    return failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--self-test") {
        return runSelfTests();
    }
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::cout << argv[i] << " : " << (accepts(argv[i]) ? "ACCEPT" : "REJECT") << '\n';
        }
        return 0;
    }

    std::cout << "Disposable Email Blocker\n"
              << "Type an email to check, or :quit to exit.\n";
    std::string input;
    while (std::cout << "Email> " && std::getline(std::cin, input)) {
        if (input == ":quit") break;
        std::cout << (accepts(input) ? "ACCEPT" : "REJECT") << '\n';
    }
    return 0;
}
