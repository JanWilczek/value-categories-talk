---
---
# (Im)Perfect Forwarding: A Practical Guide to Value Categories in C++


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

```cpp
class Lifetime {
public:
	explicit Lifetime(std::string_view name) : name_{name} {
		std::println("{} constructor", name_);
	}
	
	~Lifetime() {
		std::println("{} destructor", name_);
	}

	Lifetime(const Lifetime& other) : name_{other.name_ + "_copy"} {
		std::println("{} copy constructor", name_);
	}

	Lifetime(Lifetime&& other) : name_{other.name_ + "_moved"} {
		std::println("{} move constructor", name_);
	}

	Lifetime& operator=(const Lifetime& other) {
		name_ = other.name_ + "_copy";
		std::println("{} copy assignment", name_);
		return *this;
	}

Lifetime& operator=(Lifetime&& other) {
	name_ = other.name_ + "_moved";
	std::println("{} move assignment", name_);
	return *this;
}

private:
	std::string name_;
};
```

---

````md magic-move
```cpp {1-4|6-9|11}
template <typename Func>
auto my_invoke_bad(Func&& func) {
	return func();
}

Lifetime const& foo() {
	static Lifetime const l{"foo"};
	return l;
}

auto const& s = my_invoke_bad(foo);
```
```cpp {11|all|2}
template <typename Func>
auto my_invoke_bad(Func&& func) {
	return func();
}

Lifetime const& foo() {
	static Lifetime const l{"foo"};
	return l;
}

static_assert(std::same_as<Lifetime, decltype(my_invoke_bad(foo))>); // ⚠️
```
```cpp {2|11}
template <typename Func>
decltype(auto) my_invoke_bad(Func&& func) {
	return func();
}

Lifetime const& foo() {
	static Lifetime const l{"foo"};
	return l;
}

static_assert(std::same_as<Lifetime const&, decltype(my_invoke_good(foo))>); // ✅
```
```cpp {11}
template <typename Func>
decltype(auto) my_invoke_bad(Func&& func) {
	return func();
}

Lifetime const& foo() {
	static Lifetime const l{"foo"};
	return l;
}

static_assert(std::same_as<Lifetime const&, decltype(my_invoke_good(foo))>); // ✅
```
````

---

# Value categories are about expressions

<v-click>

> "An expression is a sequence of operators and operands that specifies a computation[.](https://eel.is/c++draft/expr#pre-1.sentence-2) An expression can result in a value and can cause side effects[.](https://eel.is/c++draft/expr#pre-1.sentence-3)"

</v-click>

---

# Examples of C++ expressions

<!-- Sandeep Mantrala slide -->

---

> "each C++ expression has a type and a value category"

---

# Expression type

```cpp
auto foo = <expression>;
```

---

# Expression type examples

```cpp
auto i = 1;

auto i = 1 + 2;

int a = 1;
auto i = a;

int a = 1;
int& a_ref = a;
auto i = a_ref;
```

---

# Retrieving expression type

```cpp
auto i = 1;
std::decay_t<decltype(1)> i = 1;

auto i = 1 + 2;
std::decay_t<decltype(1+2)> i = 1 + 2;
```

---

# Expression type is never a reference, `const`, or `volatile`

> "If an expression initially has the type “reference to T”, the type is adjusted to T prior to any further analysis; the value category of the expression is not altered."

> "If a prvalue initially has the type “cv T”, where T is a cv-unqualified non-class, non-array type, the type of the expression is adjusted to T prior to any further analysis."

---

# Retrieving expression type

```cpp
static_assert(std::same_as<int, std::decay_t<decltype(1)>>);

static_assert(std::same_as<int, std::decay_t<decltype(1+2)>>);

int a = 1;
int& a_ref = a;
static_assert(std::same_as<int, std::decay_t<decltype(a_ref)>>);
```

<!-- auto never gives a reference -->

---

# Value categories

<v-clicks>

Not for you...

...but for the compiler

```cpp
enum class E { one = 1, two = 2, three = 3 };

E::one = 2; // ❌
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

#define PRINT_VALUE_CATEGORY(E) std::println("{}: {}", #E, value_category<decltype((E))> // unevaluated
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

