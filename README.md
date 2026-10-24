# Disposable Email Blocker

Single-file C++17 console application that simulates finite automata to validate the assignment's email syntax and block disposable providers. Only the C++ standard library is used; no regular expressions are used.

## Build and run

From this directory, using a C++17 compiler:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -static main.cpp -o disposable-email-blocker.exe
./disposable-email-blocker.exe
./disposable-email-blocker.exe alice@gmail.com bob@mozmail.com
./disposable-email-blocker.exe --self-test
```

The supplied Windows executable was built with MinGW using static linking. If another compiler does not support `-static`, omit that option.

Interactive mode checks one complete line at a time. Type `:quit` to exit, or use end-of-file. Command-line mode checks every supplied argument. Quote arguments containing spaces to test invalid inputs. Both modes print ACCEPT or REJECT. Exit status is 0 for a completed checking session; `--self-test` exits with 1 if any test fails.

## Public function

```cpp
bool accepts(const std::string& s);
```

Returns true only when the complete input satisfies the specified email syntax and its domain is allowed. Invalid format and blocked providers both return false. Input is not trimmed or converted to lowercase.

## How the finite automata work

The format DFA reads the entire email from left to right. Its states track the local part's last character, entry into the domain, whether a domain dot has appeared, and whether the domain currently ends with an alphanumeric character.

| State | Meaning |
| --- | --- |
| Start | No input read |
| LocalCharacter | Local part ends in a lowercase letter or digit |
| LocalDot | Local part ends in a dot |
| DomainStart | The @ was read; first domain character is required |
| DomainWithoutDot | Domain has no dot and ends in an alphanumeric character |
| DomainHyphenWithoutDot | Domain has no dot and ends in a hyphen |
| DomainWithDot | Domain has a dot and ends in an alphanumeric character |
| DomainSeparatorWithDot | Domain has a dot and ends in a dot or hyphen |
| Dead | Invalid input; all subsequent transitions stay here |

Start is the initial state. DomainWithDot is the only accepting format state. The transition function in main.cpp specifies every transition; all unrecognized characters enter Dead.

After the format passes, a second finite automaton reads the domain from right to left. Its states form a trie of the four reversed provider names. Missing edges enter an implicit safe sink. A terminal provider state blocks the address only at the beginning of the domain or immediately after a dot boundary. Otherwise matching continues until it fails or reaches the beginning.

Blocked providers:

- mozmail.com
- mailinator.com
- guerrillamail.com
- 10minutemail.com

Thus inbox.mozmail.com is blocked, while notmozmail.com and mozmail.com.evil.org are allowed. Provider matching considers only the domain.

Both machines have finitely many states and use no stack. The second machine scans in reverse to recognize suffixes directly. For a fixed blocklist, checking takes O(n) time and O(1) auxiliary space in the input length; the provider transition table is initialized once and has fixed size. This is a two-pass finite-automata implementation, not a single forward scan.

## Exact assignment rules

The local part contains lowercase letters, digits, and dots and starts and ends in a letter or digit. The domain contains lowercase letters, digits, dots, and hyphens, has at least one dot, and starts and ends in a letter or digit.

The assignment does not forbid consecutive internal dots or hyphens, single-character final labels, or long inputs. Therefore a..b@x..com, a@x.-y, and 0@1.2 are accepted. No DNS lookups, additional provider lists, or extra real-world email rules are applied.

## Verification

All 65 built-in tests pass, including the eight assignment examples, every provider and its subdomains, misleading prefixes and suffixes, malformed inputs, embedded null characters, and long inputs. An additional independent development check compared 24,531 inputs against a separate specification-based reference, including exhaustive strings over a small alphabet and generated provider cases; all passed. The external development checker is not part of the single-file application.

The written report and presentation are deferred.

## GitHub

Remote: https://github.com/abdullahratag/disposable-email-blocker

To upload the local commit from an authenticated Git installation with network access:

```powershell
git push -u origin main
```

If the remote already contains commits, inspect and reconcile them before pushing; do not force-push.
