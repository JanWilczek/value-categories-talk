---
theme: default
colorSchema: light
title: (Im)Perfect Forwarding - Value Categories in C++
titleTemplate: '%s - Jan Wilczek Meetup Milan Nov 2026'
info: |
  ## (Im)Perfect Forwarding - Value Categories in C++

  Jan Wilczek's Milan C++ Meetup talk
author: Jan Wilczek
export:
  format: pdf
  timeout: 30000
  dark: false
  withClicks: false
  withToc: false
# https://sli.dev/features/drawing
drawings:
  persist: false
# slide transition: https://sli.dev/guide/animations.html#slide-transitions
transition: undefined
# enable Comark Syntax: https://comark.dev/syntax/markdown
comark: true
# duration of the presentation
duration: 50min
lineNumbers: true
fonts:
  sans: Montserrat, Open Sans
  mono: CaskaydiaCove Nerd Font
  local: CaskaydiaCove Nerd Font
---

# (Im)Perfect Forwarding

## A Practical Guide to Value Categories in C++

Jan Wilczek (thinkcell)

Milan Nov 2026

---
layout: cover
---

<img src="./assets/tc_homepage_hero.webp" class="w-full h-full"/>

<!-- Please, interrupt me. I will be showing a lot of code so you may get lost. Please, stop me and ask question, even if it's just because you fell asleep -->

---

# Outline

1. Motivation
1. Expressions
1. Expression types
1. Value categories
1. Applications
1. Problems
1. Summary

---

# What's wrong with this code?

```cpp
template <typename Func>
auto my_invoke_bad(Func&& func) {
    return func();
}
```

---
zoom: 0.8
---

# My favorite C++ class

```cpp
struct Lifetime {
    explicit Lifetime(std::string_view name) : name_{name} { std::println("{} constructor", name_); }
    ~Lifetime() { std::println("{} destructor", name_); }
    Lifetime(const Lifetime& other) : name_{other.name_ + "_copy"} { std::println("{} copy constructor", name_); }

    Lifetime(Lifetime&& other) noexcept : name_{other.name_ + "_moved"} {
        std::println("{} move constructor", name_);
    }

    Lifetime& operator=(const Lifetime& other) {
        name_ = other.name_ + "_copy";
        std::println("{} copy assignment", name_);
        return *this;
    }

    Lifetime& operator=(Lifetime&& other) noexcept {
        name_ = other.name_ + "_moved";
        std::println("{} move assignment", name_);
        return *this;
    }

private:
    std::string name_;
};
```

---

# What's wrong with this code?

https://godbolt.org/z/rexGsqvnj

```cpp
template <typename Func>
auto my_invoke_bad(Func&& func) {
    return func();
}
```

<v-click>

````md magic-move
```cpp
Lifetime const& foo() {
    static Lifetime const l{"foo"};
    return l;
}

auto const& s = my_invoke_bad(foo);
```
```cpp
Lifetime const& foo() {
    static Lifetime const l{"foo"};
    return l;
}

auto const& s = my_invoke_bad(foo);
// foo constructor
// foo_copy copy constructor
// foo_copy destructor
// foo destructor

static_assert(std::same_as<Lifetime, decltype(my_invoke_bad(foo))>); // ⚠️
```
````

</v-click>

---

# `decltype(auto)` preserves type and value category of the returned expression

https://godbolt.org/z/rexGsqvnj

````md magic-move
```cpp {2}
template <typename Func>
auto my_invoke_bad(Func&& func) {
    return func();
}

Lifetime const& foo() {
    static Lifetime const l{"foo"};
    return l;
}

auto const& s = my_invoke_bad(foo);
static_assert(std::same_as<Lifetime, decltype(my_invoke_bad(foo))>); // ⚠️
```
```cpp {2|2,11,12|14-15}
template <typename Func>
decltype(auto) my_invoke_good(Func&& func) {
    return func();
}

Lifetime const& foo() {
    static Lifetime const l{"foo"};
    return l;
}

auto const& s = my_invoke_good(foo);
static_assert(std::same_as<Lifetime const&, decltype(my_invoke_good(foo))>); // ✅

// foo constructor
// foo destructor
```
````


---
layout: quote
---

# Value categories are about expressions

<v-click>

> An expression is a sequence of operators and operands that specifies a computation[.](https://eel.is/c++draft/expr#pre-1.sentence-2) An expression can result in a value and can cause side effects[.](https://eel.is/c++draft/expr#pre-1.sentence-3)

</v-click>

---
layout: two-cols-header
---

# Examples of C++ expressions

::left::

### ✅ Expressions

```cpp {none|1,2|1,3|1,4|1,5|7,8|7,9|7,10|12,13|12,14|12,15|none}
// Evaluating to a value
5 + 3              // evaluates to 8
std::string("hi")  // temporary string object
func()       // evaluates to the return value
[](){ return 42; }() // closure call

// Accessing existing objects
*ptr
arr[2]
std::move(x)

// Causing side effects
x = 5            // assigns 5 to x
++x              // increments x by 1
std::cout << x   // prints to standard output
```

::right::

### ❌ Not expressions

```cpp {none|1|3|5|7|9-11}
int a;               // declaration statement

namespace ns {}      // namespace definition

void fun() {}        // function definition

typedef int MyInt;   // type alias

class MyClass {
  // ...
};                   // class definition
```

<div class="abs-b text-left pl-15 pb-6 w-full">

<span class="text-xs opacity-60">Adapted from Sandeep Mantrala, *C++ Value Categories in Action*, C++ on Sea 2025</span>

</div>

<!-- Only expressions have value categories! -->

---
layout: quote
---

> each C++ expression has a type and a value category

---

# Expression type

```cpp
auto foo = <expression>; // foo has the type of the expression
```

---

# Expression type examples

```cpp {1|3|5-6|8-10}
auto i = 1;

auto j = 1 + 2;

int a = 1;
auto k = a;

int b = 1;
int& b_ref = b;
auto l = b_ref;
```

---

# Retrieving expression type

```cpp {1-2|4-5|7-8|10-12}
auto i = 1;
std::decay_t<decltype(1)> i = 1;

auto i = 1 + 2;
std::decay_t<decltype(1+2)> i = 1 + 2;

int a = 1;
std::decay_t<decltype(a)> k = a;

int b = 1;
int& b_ref = b;
std::decay_t<decltype(b_ref)> l = b_ref;
```

---

# Expression type is never a reference

> If an expression initially has the type “reference to T”, the type is adjusted to T prior to any further analysis; the value category of the expression is not altered.

---

# Retrieving expression type

```cpp
static_assert(std::same_as<int, std::decay_t<decltype(1)>>);

static_assert(std::same_as<int, std::decay_t<decltype(1+2)>>);

int b = 1;
int& b_ref = a;
static_assert(std::same_as<int, std::decay_t<decltype(b_ref)>>);
```

<!-- auto never gives a reference -->

---

# Value categories

<p></p>

<v-clicks>

Not for you...

...but for the compiler

```cpp
enum class E { one = 1, two = 2, three = 3 };

E::one = 2; // ❌
E::one = E::two; // ❌
```

</v-clicks>

---

# `decltype(<variable>)`

```cpp
int i = 0;
static_assert(std::same_as<int, decltype(i)>);

int& i_ref = i;
static_assert(std::same_as<int&, decltype(i_ref)>);

int&& i_ref_ref = std::move(i);
static_assert(std::same_as<int&&, decltype(i_ref_ref)>);
```

---

# `decltype((<expr>))`

```cpp {|1|3-4|6}
static_assert(std::same_as<int, decltype((1))>); // 1 is prvalue

int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // i is lvalue

static_assert(std::same_as<int&&, decltype((std::move(i)))>); // std::move(i) is xvalue
```

---

```cpp
template <typename T>
struct value_category_t {
    static constexpr auto name = "prvalue";
};

template <typename T>
struct value_category_t<T&> {
    static constexpr auto name = "lvalue";
};

template <typename T>
struct value_category_t<T&&> {
    static constexpr auto name = "xvalue";
};

template <typename T>
constexpr auto value_category = value_category_t<T>::name;

#define PRINT_VALUE_CATEGORY(E) std::println("{}: {}", #E, value_category<decltype((E))>) // unevaluated
```

---

```cpp
int& i_ref = i;
static_assert(std::same_as<int&, decltype((i_ref))>); // lvalue!

int&& i_ref_ref = std::move(i);
static_assert(std::same_as<int&, decltype((i_ref_ref))>); // lvalue!
```

---

# Object access in C++

```cpp
struct S {
    int one = 1;
    int two = 2;
};

int main() {
    S s{};
    
    auto i = s.one;
    auto j = s.two;
}
```

---

# Object access in C++

```asm
s$ = 8
; ...
mov DWORD PTR s$[rsp], 1
mov DWORD PTR s$[rsp+4], 2

i$ = 0
j$ = 4
; ...
; initialize i
mov eax, DWORD PTR s$[rsp]
mov DWORD PTR i$[rsp], eax

; initialize j
mov eax, DWORD PTR s$[rsp+4]
mov DWORD PTR j$[rsp], eax
```

---

# Object access in C++

```cpp
struct S {
    int one = 1;
    int two = 2;
};

int main() {
    auto k = S{}.two;
}
```


```asm
$T1 = 24
; ...
mov DWORD PTR $T1[rsp], 1
mov DWORD PTR $T1[rsp+4], 2
mov eax, DWORD PTR $T1[rsp+4]
mov DWORD PTR k$[rsp], eax
```

---

# lvalue

Something we can take the address of.

````md magic-move
```cpp
S s{};
S* s_addr = &s;
```
```cpp
S s{};
S* s_addr = &s;

S* stemp_addr = &S{}; // error: taking address of rvalue (GCC) | error C2102: '&' requires l-value (MSVC)
```
````

---

# Only an lvalue can bind to a non-const lvalue reference

````md magic-move
```cpp
int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // lvalue
```
```cpp
int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // lvalue

S& s_ref = S{}; // error: cannot bind non-const lvalue reference of type 'S&' to an rvalue of type 'S' (GCC) 
                // A non-const reference may only be bound to an lvalue (MSVC)
```
```cpp
int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // lvalue

S& s_ref = S{}; // error: cannot bind non-const lvalue reference of type 'S&' to an rvalue of type 'S' (GCC) 
                // A non-const reference may only be bound to an lvalue (MSVC)

S const& s_constref = S{}; // temporary lifetime extension
```
````

<!-- Evaluating expression results in an rvalue/lvalue -> ... is lvalue/rvalue -->

---

# lvalue examples

```cpp
int b = 1;
PRINT_VALUE_CATEGORY(b); // lvalue
int& b_ref = b;
PRINT_VALUE_CATEGORY(b_ref); // lvalue
int&& b_ref_ref = std::move(b);
PRINT_VALUE_CATEGORY(b_ref_ref); // lvalue!
```


> (...) named rvalue references are treated as lvalues and unnamed rvalue references to objects are treated as xvalues; rvalue references to functions are treated as lvalues whether named or not.

```cpp
assert(&b == &b_ref_ref);
```


---

# lvalue examples - pointers

```cpp
S s{};
S* p = &s;
PRINT_VALUE_CATEGORY(p); // lvalue
PRINT_VALUE_CATEGORY(*p); // lvalue
PRINT_VALUE_CATEGORY(p->member); // lvalue
PRINT_VALUE_CATEGORY((*p).member); // lvalue
```

---

# lvalue examples - functions

```cpp
void foo() {}

void (*p)() = &foo;
PRINT_VALUE_CATEGORY(foo); // lvalue
```


```cpp
int& a_ref() {
    static int a = 0;
    return a;
}

a_ref() = 5;
PRINT_VALUE_CATEGORY(a_ref()); // lvalue
```


---

# lvalue examples - array access

```cpp
int a[] = {0, 1, 2};
int* p = a;
PRINT_VALUE_CATEGORY(a); // lvalue
PRINT_VALUE_CATEGORY(p); // lvalue
PRINT_VALUE_CATEGORY(a[0]); // lvalue
PRINT_VALUE_CATEGORY(p[0]); // lvalue
```

---

# lvalue examples

```cpp
int i = 0;
PRINT_VALUE_CATEGORY(++i); // lvalue

PRINT_VALUE_CATEGORY(static_cast<int&>(i)) // lvalue
```

---

# lvalues - summary

* has a name? $\rightarrow$ lvalue
* can take address? $\rightarrow$ lvalue
* assignable? $\rightarrow$ lvalue

```cpp
auto const i_const = 1;
i_const = 2; // error, although i_const is an lvalue
auto a = 1;
auto const& a_constref = a;
a_constref = 2; // error, although a_constref is an lvalue
```

---

# rvalues

* xvalues
- prvalues

---

# prvalues ("pure rvalues")

```cpp
int x = 2;
1 = x; // error: lvalue required as left operand of assignment (GCC) 
       // error C2106: '=': left operand must be l-value (MSVC)
```

---

# prvalue examples

```cpp {1|2-3|4|5-6|7-9|10|11|12|13|14|15}
PRINT_VALUE_CATEGORY(1); // prvalue
int foo();
PRINT_VALUE_CATEGORY(foo()); // prvalue
PRINT_VALUE_CATEGORY([](){}); // prvalue
enum class E { one, two, three };
PRINT_VALUE_CATEGORY(E::one); // prvalue
bool b1 = true;
bool b2 = false;
PRINT_VALUE_CATEGORY(b1 && b2); // prvalue
PRINT_VALUE_CATEGORY(3 < x); // prvalue
PRINT_VALUE_CATEGORY(&x); // prvalue
PRINT_VALUE_CATEGORY(true); // prvalue
PRINT_VALUE_CATEGORY(2.f); // prvalue
PRINT_VALUE_CATEGORY(2.); // prvalue
PRINT_VALUE_CATEGORY("foo"); // lvalue!
```

<!-- prvalue is never const or volatile, and function calls returning void are also treated as prvalues. -->

---

# `this` pointer

```cpp
struct S {
    void foo() {
        PRINT_VALUE_CATEGORY(this); // prvalue
    }
};

S s{};
s.foo();
```

---

# xvalue

```cpp
int x = 1;
PRINT_VALUE_CATEGORY(std::move(x)); // xvalue
PRINT_VALUE_CATEGORY(static_cast<int&&>(x)); // xvalue
```

---

# Value categories classification

<img src="/assets/value_categories_classification.png" />

---
layout: cover
---

# Applications

---

# Move semantics (C++11)

```cpp
void foo(const Lifetime&) {
    std::println("const&");
}

void foo(Lifetime&) {
    std::println("&");
}

void foo(Lifetime&&) {
    std::println("&&");
}

int main() {
    Lifetime l{"bar"};
    foo(std::move(l));
    foo(Lifetime{"baz"});
}
```

---

# Forwarding reference (C++11)

```cpp
template <typename T>
void foo(T&& forwarding_ref) {}

int i = 1;
foo(i); // calls foo<int&>(int&)
foo(0); // calls foo<int>(int&&)
foo(std::move(i)); // calls foo<int>(int&&)
```


---

# Simpler forwarding reference syntax (C++17)

```cpp
template <typename T>
void foo(T&& forwarding_ref) {}

void foo_cpp17(auto&& forwarding_ref) {}
```

---

# Forwarding forwarding reference

````md magic-move
```cpp
template <typename T>
void foo(T&& forwarding_ref) {
    bar(std::forward<T>(forwarding_ref));
}

void foo_cpp17(auto&& forwarding_ref) {
    bar(???);
}
```
```cpp
template <typename T>
void foo(T&& forwarding_ref) {
    bar(std::forward<T>(forwarding_ref));
}

void foo_cpp17(auto&& forwarding_ref) {
    bar(tc_move_if_owned(forwarding_ref));
}
```
````

---

# `tc_move_if_owned()`

```cpp
namespace detail {
    template <typename T>
    struct move_if_owned {
        using type = T&&;
    };
}

// T -> T&&
// T& -> T&
// T&& -> T&&
#define tc_move_if_owned(v) (static_cast<typename detail::move_if_owned<decltype(v)>::type>(v))
```

---

# Reference collapsing

<img src="./assets/reference_collapsing.png" width="70%" />

---

```cpp
template <typename Func>
auto my_invoke_bad(Func&& func) {
    return func();
}

Lifetime const& foo() {
    static const Lifetime l{"foo"};
    return l;
}

int main() {
    auto const& s = my_invoke_bad(foo);
}
// foo constructor
// foo_copy copy constructor
// foo_copy destructor
// foo destructor
```

---

```cpp
template <typename Func>
decltype(auto) my_invoke_good(Func&& func) {
    return func();
}


Lifetime const& foo() {
    static const Lifetime l{"foo"};
    return l;
}

int main() {
    auto const& s = my_invoke_good(foo);
}
// foo constructor
// foo destructor
```

---

```cpp
void test_rvalue_ref_arg(int&& x_ref_ref) {
    PRINT_VALUE_CATEGORY(x_ref_ref); // lvalue!
    PRINT_VALUE_CATEGORY(std::move(x_ref_ref)); // xvalue
}

int b = 1;
test_rvalue_ref_arg(std::move(b));
test_rvalue_ref_arg(1); // prvalue-to-xvalue conversion
```

---

# Pitfalls of `std::move()`

```cpp
Lifetime l{"foo"};
Lifetime& l_ref = l;
Lifetime const& l_constref = l;
Lifetime const l_const{"foo_const"};

Lifetime l_constref_moved = std::move(l_constref); // copy
Lifetime l_const_moved = std::move(l_const); // copy
Lifetime l_ref_moved = std::move(l_ref); // move
```

---

# Safer move

```cpp
namespace detail {

template <typename Decltype>
constexpr decltype(auto) safe_move(auto&& obj) noexcept {
    static_assert(!std::is_const<std::remove_reference_t<decltype(obj)>>::value, "Cannot move out of const.");
    static_assert(!std::is_rvalue_reference<decltype(obj)>::value, "Unnecessary move; already an rvalue.");
    static_assert(!std::is_lvalue_reference<Decltype>::value, "Are you sure you want to move out of an lvalue reference?");
    
    return static_cast<std::remove_reference_t<decltype(obj)>&&>(obj);
}
}

#define safe_move(...) detail::safe_move<decltype(__VA_ARGS__)>(__VA_ARGS__)

int main() {
    int j = 1;
    safe_move(j); // OK, an lvalue
    int const k_const = 1;
    safe_move(k_const); // compile error: const
    int& j_ref = j;
    safe_move(j_ref); // compile error: lvalue reference
    safe_move(std::move(j)); // compile error: already an rvalue
    int&& j_ref_ref = std::move(j);
    safe_move(j_ref_ref); // OK, because an lvalue
}
```

---

# What if you really want to move from an lvalue reference?

* `tc_move_always()`
* `tc_move_always_even_const()`

---
layout: cover
---

# Relocating prvalues

---

```cpp
template <typename T> decltype(auto) id(T&& t) { return std::forward<T>(t); }

auto&& a = id(std::move(x)); // ✅
auto&& b = id(make_vec());   // ❌
```

---

# `tc::temporary`

```cpp
template <typename T, unsigned Lifetime>
struct temporary {
  T&& m_ref;
  ...
};
```

---

# `tc::temporary`

```cpp
template <typename T>
using prvalue_as_temporary_t = std::conditional_t<
    std::is_reference<T>::value || tc::instance_tn<T, tc::temporary>, T, tc::temporary<T, 0>>;

#define tc_prvalue_as_temporary(...) \
  static_cast<tc::prvalue_as_temporary_t<decltype((__VA_ARGS__))>>(__VA_ARGS__)
```

---

# References

## Written

1. [*Value Categories* on cppreference.com](https://en.cppreference.com/cpp/language/value_category)
1. [David Mazières, *C++ value categories and decltype demystified*](https://www.scs.stanford.edu/~dm/blog/decltype.html)
1. [Sébastien Bini, Ed Catmur, *Relocating prvalues*, C++ Standard proposal](https://wg21.link/P2785)

## Videos

1. [Ben Saks, *Back to Basics: Understanding Value Categories*, CppCon 2019](https://youtu.be/XS2JddPq7GQ?si=bQoNwXxk0c-HdMwe)
1. [Inbal Levi, *Back to Basics: Master C++ Value Categories With Standard Tools*, CppCon 2022](https://youtu.be/tH0Z2OvHAd8?si=FQA4ISNlebjP24p5)
1. [Sandeep Mantrala, *Understanding C++ Value Categories: lvalue, rvalue, xvalue, std::move, std::forward & Best Practices*, C++ On Sea 2025](https://youtu.be/d5h9xpC9m8I?si=3ac14PJmIRu64zth)

## Code

1. [thinkcell public library](https://github.com/think-cell/think-cell-library)

---

# Summary

<v-clicks>

* lvalue: something we can take the address of
* prvalue: temporary
* xvalue: unnamed rvalue reference
* `decltype(x)` returns the type `x` was declared with
* `decltype((x))` returns `T`, `T&`, or `T&&`, if expression `x` is a prvalue, lvalue, or an xvalue respectively
* It's worth understanding value categories to read th C++ standard
* Apply to thinkcell!

</v-clicks>

