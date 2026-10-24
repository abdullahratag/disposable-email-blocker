# Disposable Email Blocker

Our project is a C++ console program that uses finite automata to check email addresses. It rejects invalid email formats and blocks these disposable email providers, including their subdomains:

- mozmail.com
- mailinator.com
- guerrillamail.com
- 10minutemail.com

The whole program is in `main.cpp` and only uses the C++ standard library.

## How to run

Compile using a C++17 compiler:

```powershell
g++ -std=c++17 -static main.cpp -o disposable-email-blocker.exe
```

Run the program:

```powershell
.\disposable-email-blocker.exe
```

Enter an email to see ACCEPT or REJECT. Type `:quit` to exit.

You can also pass an email directly:

```powershell
.\disposable-email-blocker.exe alice@gmail.com
```

## How it works

The `accepts()` function checks the email format using a DFA. Then another finite automaton reads the domain backward to check for a blocked provider.

For example, `user@inbox.mozmail.com` is rejected, but `user@notmozmail.com` and `user@mozmail.com.evil.org` are accepted.

## Tests

Run the 65 built-in test cases:

```powershell
.\disposable-email-blocker.exe --self-test
```
