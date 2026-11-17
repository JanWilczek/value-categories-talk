* This talk is on value categories. But before we even discuss this, let's ask ourselves, why should we care? To illustrate this, I'd like you to tell me: what's wrong with this code? https://godbolt.org/z/rexGsqvnj
```cpp
template <typename Func>
auto my_invoke_bad(Func&& func) {
	return func();
}
```
* This function takes a callable and invokes it. First, can anyone tell me what this syntax, `Func&&`, means? (forwarding reference). OK, and now, what's wrong with this function?
* On the outset, everything seems fine; we invoke the callable and the return type is deduced automatically. To illustrate what may go wrong, let me introduce to what may be my favorite C++ class: `Lifetime`. This class has all special member functions defined: constructor, destructor, copy constructor, move constructor, copy assignment operator, and move assignment operator. We give each instance a name, and it prints out the name during each special operation along with the operation name. This allows us to see which operations are happening in the code.
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
* Now, consider this innocent little function
```cpp
Lifetime const& foo() {
	static Lifetime const l{"foo"};
	return l;
}
```
* This function creates a static local const variable of type `Lifetime` and returns it by a const reference. It's a typical way to expose some global constants if you want them to be lazily evaluated. What will happen when we call function `foo()` using `my_invoke()`? What will it print in terms of operations performed?
```cpp
auto const& s = my_invoke_bad(foo);
```
Well, this prints
```txt
foo constructor
foo_copy copy constructor
foo_copy destructor
foo destructor
```
* Why does this happen? Why do we get a copy although we explicitly specified a const reference as the receiving variable?
* The reason is that `auto`, when used for return type deduction, infers only the type of the return expression but does not preserve its **value category**. We can verify it with a `static_assert()`.
```cpp
static_assert(std::same_as<Lifetime, decltype(my_invoke_bad(foo))>); // ⚠️
```
* A quick fix for this is to use the somewhat confusing syntax of `decltype(auto)`.
```cpp
template <typename Func>
decltype(auto) my_invoke_good(Func&& func) {
	return func();
}

static_assert(std::same_as<Lifetime const&, decltype(my_invoke_good(foo))>); // ✅
```
* `decltype(auto)` used for the to-be-inferred return type preserves both the type and the value category. This is but a single example of why value categories are important. We will see more examples further in the talk.
* This talk is on value categories. But what are they? Right off the bat, we hit a problem: value categories are not about values in the common sense of objects, but about expressions.
* So we must start with the definitions: what is an expression? The C++ Standard says, "An expression is a sequence of operators and operands that specifies a computation[.](https://eel.is/c++draft/expr#pre-1.sentence-2) An expression can result in a value and can cause side effects[.](https://eel.is/c++draft/expr#pre-1.sentence-3)" This is surprisingly helpful to use. Let's look at a few examples.
![[Pasted image 20261029133545.png]]
* OK, now we know what an expression is. Let me give you another fact: "each C++ expression has a type and a value category". Let's start by discussing the expression type because it's easier to understand.
* Expression type is what `auto` does. Let's take a look at an example. In this statement, `auto i = 1;`, what will `auto` become? `int` right? Why? Because the type of the expression `1` is `int`.
* Let's take a look at this example. In `auto i = 1 + 2;`, what will `auto` become? `int` right? Ok, let me give you another example. What will `auto` become here?
```cpp
int a = 1;
auto i = a;
```
* `int` again, right? Why? Because the type of expression `a` is `int`.
* What about this one?
```cpp
int a = 1;
int& a_ref = a;
auto i = a_ref;
```
* It's `int` again! Why? Because the **type** of the expression is `int`, just as `auto` deduces it.
* To find out what the type of an expression is, we can use `std::decay_t<decltype()>`. `auto` can be thought of as a shorthand of writing `std::decay_t<decltype()>(rhs)` of whatever is on the right-hand side of the assignment.
```cpp
auto i = 1;
std::decay_t<decltype(1)> i = 1;

auto i = 1 + 2;
std::decay_t<decltype(1+2)> i = 1 + 2;
```
* Why do we need `decay`? Because `decay` removes all references and `const` or `volatile` qualifiers from the type. And the type of an expression for the purpose of analysis is never a reference or a `const` or `volatile` type. The standard is explicit about this: ([expr.type])
				* "If an expression initially has the type “reference to T”, the type is adjusted to T prior to any further analysis; the value category of the expression is not altered."
				* "If a prvalue initially has the type “cv T”, where T is a cv-unqualified non-class, non-array type, the type of the expression is adjusted to T prior to any further analysis."
* We can use `static_assert()` to assert on the type of an expression.
```cpp
static_assert(std::same_as<int, std::decay_t<decltype(1)>>);
static_assert(std::same_as<int, std::decay_t<decltype(1+2)>>);
int a = 1;
int& a_ref = a;
static_assert(std::same_as<int, std::decay_t<decltype(a_ref)>>);
```
* By this time, I hope you have noted that the `auto` keyword does not deduce a reference; it's always a reference-less type. The same is true for expression types; they are always without references, too.
* OK, so we know that an expression in C++ is a sequence of operators and operands. It has a type. And it has a value category. So what is the value category of an expression?
* Here we hit another problem. Value categories are not something for you, the developers. It's something for the compiler. The compiler needs to know the value category of the expression to know what it can do with it.
* Fortunately, we as developers can observe the value categories of expressions, albeit indirectly. How? To this end, we need to understand the two modes of the `decltype()` specifier. This can get tricky, so I will give you a simple guideline.
* `decltype()` can give us the type of the entity that's passed as the argument. Consider the following code. https://godbolt.org/z/jnxY9fTPj
```cpp
int i = 0;
static_assert(std::same_as<int, decltype(i)>);

int& i_ref = i;
static_assert(std::same_as<int&, decltype(i_ref)>);

int&& i_ref_ref = std::move(i);
static_assert(std::same_as<int&&, decltype(i_ref_ref)>);
```
* As you can see, `decltype()` returns the type with which each variable was declared.
* That's not the case for `delctype(())` with double parentheses. As a matter of fact, it can be thought of as a completely different operator. It returns a type corresponding to the value category of the expression within the double parentheses. So we get `T` for prvalues, `T&` for `lvalues`, and `T&&` for xvalues. Don't worry if you don't know what these names mean just yet. Let's look at a few examples.
```cpp
static_assert(std::same_as<int, decltype((1))>); // prvalue

int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // lvalue

static_assert(std::same_as<int&&, decltype((std::move(i)))>); // xvalue
```
* This allows us to create the helper that I talked about.
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
* All clear, right? Thank you, end of the talk, goodbye. However, let's look at two more examples that shatter the illusion of "easy".
```cpp
int& i_ref = i;
static_assert(std::same_as<int&, decltype((i_ref))>); // lvalue!

int&& i_ref_ref = std::move(i);
static_assert(std::same_as<int&, decltype((i_ref_ref))>); // lvalue!
```
* OK, why is that the case, and what does it mean? We're almost at the point of explaining value categories, but before we do that, we need to understand how the processor accesses object members.
* Here is a simple struct and code that instantiates this struct. https://godbolt.org/z/66x3dj4Pz
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
* The code that instantiates this struct turns into the following assembly on x86 when using MSVC.
```asm
s$ = 8
; ...
mov DWORD PTR s$[rsp], 1
mov DWORD PTR s$[rsp+4], 2
```
* As you can see, the struct is initialized by placing immediate arguments into memory addresses offset from the struct's memory address. MSVC reserves space on the stack and then can use the stack pointer directly. To access the struct's fields to initialize variables `i` and `j`, the assembly does the same access operation.
```asm
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
* This has a possibly unobvious corollary. When we create a temporary struct instance, even if we just want to access a single member, the compiler needs to create space for the object on the stack. Take a look at this innocent-looking code.
```cpp
auto k = S{}.two;
```
* It produces the following x86 assembly on MSVC.
```asm
$T1 = 24
; ...
mov DWORD PTR $T1[rsp], 1
mov DWORD PTR $T1[rsp+4], 2
mov eax, DWORD PTR $T1[rsp+4]
mov DWORD PTR k$[rsp], eax
```
* As you can see, the assembly initializes the memory using the offsets relative to the temporary, termed here `T1`, then initializes the memory address of variable `k` with the contents of the `two` member. The temporary is destroyed at the end of the line, but since it doesn't have a custom destructor, no assembly code needs to represent it.
* This brings us to the concept of an `lvalue`, which means "an expression whose evaluation results in something that we can take an address of." As we just saw, variables on the stack, including temporaries, must be placed in memory. When they are, they can be accessed using their address and an offset. However, we cannot take the address of temporaries. Let's look at the following code.
```cpp
S s{};
S* s_addr = &s;
```
* This looks fine, and it works. Why does it work? Because the expression `s` is an **lvalue**, so we can take its address. However, let's see what happens when we try to take the address of a temporary of type `S`.
```cpp
S* stemp_addr = &S{}; // error: taking address of rvalue (GCC) | error C2102: '&' requires l-value (MSVC)
```
* When we look at this code, we feel uneasy inside. Taking the address of a temporary will result in a dangling pointer! Fortunately, the compilers don't allow it. But how do they know they cannot allow it? That's because they detect that expression `S{}` is not an lvalue; it is an **rvalue**. What's an rvalue? Well, every expression that's not an lvalue! And that's also where lvalue got its name, originally in the C language: lvalue stood for a **locator value**. Since we can take its address, we can locate it in memory. The temporary is in memory, but we're not allowed to locate it.
* That also makes sense when we go back to how `decltype(())` with double parentheses works; for lvalues, it returns an lvalue reference. https://godbolt.org/z/qabKzTWT4
```cpp
int i = 0;
static_assert(std::same_as<int&, decltype((i))>); // lvalue
```
* That makes intuitive sense, right? We know that a temporary cannot bind to an lvalue reference. This code doesn't compile, and the compiler tells us why: `S{}` is not an lvalue; it is an rvalue.
```cpp
S& s_ref = S{}; // error: cannot bind non-const lvalue reference of type 'S&' to an rvalue of type 'S' (GCC) | A non-const reference may only be bound to an lvalue (MSVC)
```
* So although we may not be well-versed with value categories, we understand that lvalue references can only bind to non-temporaries.
* There is an exception to this, of course. We can bind lvalues as well as temporaries to *const* lvalue references. The following is valid and correct C++ code.
```cpp
S const& s_constref = S{};
```
* Why does it work? The C++ standard explicitly allows const lvalue references to bind to temporaries, causing a **temporary lifetime extension**. That is to say that in this code, the instance of `S` will live as long as the `s_constref` variable. We still say that `s_constref` is an lvalue in this case. To be more precise, I should say that "evaluating the expression `(s_constref)` results in an lvalue," but when I say "`s_constref` is an lvalue", I believe you understand what I mean.
* What are some other examples of lvalues? Well, some are less intuitive than others. Let's look at some of them.
```cpp
int b = 1;
PRINT_VALUE_CATEGORY(b); // lvalue
int& b_ref = b;
PRINT_VALUE_CATEGORY(b_ref); // lvalue
int&& b_ref_ref = std::move(b);
PRINT_VALUE_CATEGORY(b_ref_ref); // lvalue!
```
* Why is `b_ref_ref`, an expression containing only an rvalue reference, an lvalue? Because a named rvalue reference acts exactly as a normal reference. Why? Because the standard says so: "(...) named rvalue references are treated as lvalues and unnamed rvalue references to objects are treated as xvalues; rvalue references to functions are treated as lvalues whether named or not." [basic.lval](https://eel.is/c%2B%2Bdraft/basic.lval?utm_source=chatgpt.com). This is one of the many surprises we will encounter. But it also makes sense, when we consider that we can take the address of the rvalue reference. 
```cpp
assert(&b == &b_ref_ref);
```
* Let's now consider pointers. https://godbolt.org/z/aEndMsEfW
```cpp
S s{};
S* p = &s;
PRINT_VALUE_CATEGORY(p); // lvalue
PRINT_VALUE_CATEGORY(*p); // lvalue
PRINT_VALUE_CATEGORY(p->member); // lvalue
PRINT_VALUE_CATEGORY((*p).member); // lvalue
```
* As you can see, since `p` and `s` are named variables and `p` points to `s`, all combinations that result in a reference to a named variable are lvalues.
* What may be a little surprising is that function names are also lvalues. Why? Because we can take their addresses.
```cpp
void foo() {}

void (*p)() = &foo;
PRINT_VALUE_CATEGORY(foo); // lvalue
```
* In the same vein, function calls that return lvalue references are also lvalues. That makes sense, because we can take their address and we can assign to them.
```cpp
int& a_ref() {
	static int a = 0;
	return a;
}

a_ref() = 5;
PRINT_VALUE_CATEGORY(a_ref()); // lvalue
```
* Array-access expression also results in an lvalue, as we can always take the address of an array element.
```cpp
int a[] = {0, 1, 2};
int* p = a;
PRINT_VALUE_CATEGORY(a); // lvalue
PRINT_VALUE_CATEGORY(p); // lvalue
PRINT_VALUE_CATEGORY(a[0]); // lvalue
PRINT_VALUE_CATEGORY(p[0]); // lvalue
```
* The last two lvalue examples I'd like to give here are the built-in pre-increment operator and a `static_cast` to an lvalue reference (when such a cast is allowed).
* Let's summarize our rules for lvalues: if an expression evaluation results in something that has a name, including functions, it is most likely an lvalue. The distinguishing feature of lvalues is that we can take their address. Finally, lvalues are the only value category you can assign to, unless it's a const lvalue reference.
```cpp
auto const i_const = 1;
i_const = 2; // error, although i_const is an lvalue
auto a = 1;
auto const& a_constref = a;
a_constref = 2; // error, although a_ref_const is an lvalue
```
* That leads us to rvalues, a category of expressions that cannot be assigned to. We further categorize rvalues into xvalues and prvalues. Let's deal with prvalues first.
* prvalues stand for "pure rvalues". Expressions that result in new objects are most often prvalues. Let's consider the following code.
```cpp
int x = 2;
1 = x; // error: lvalue required as left operand of assignment (GCC) | error C2106: '=': left operand must be l-value (MSVC)
```
* Assigning `x` to the value of `1` seems nonsensical. But why? Why cannot it compile? Well, because `1` is not an lvalue; it is a prvalue, a pure rvalue. Everywhere, where as a result of expression we get a new object that's not a reference, we have a prvalue expression. That includes function calls that return by value, lambda expressions, enumeration values, logical expressions, comparisons, address-of expressions, and literals... with one exception.
```cpp
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
* String literals, as they are simply character arrays and we can take their address, are treated as lvalues. That's the only exception: all other literals are prvalues.
* You can see from this list that assigning to prvalues doesn't make any sense. What would it mean if we tried to assign to an enum constant? What would it mean if we tried to assign to a lambda? prvalues save us from such a disaster.
* Since prvalues are temporaries, we also cannot take their addresses. Besides, what would be the address of the boolean constant `true`?
* So that's it: prvalues are expressions of which we cannot take the address, and we cannot assign to. Additionally, a prvalue is never `const` or `volatile`, and function calls returning `void` are also treated as prvalues.
	* Before we move on, I'd like to point out one peculiarity: the `this` pointer. According to what we discussed so far, what should be the value category of the `this` pointer? To me, `this`, represents some object in memory, it has some identity, so it should be an lvalue, just like a regular pointer. It turns out, however, that the standard specifies `this` as a prvalue expression whose value is the address of the "implicit object parameter".
```cpp
struct S {
	void foo() {
		PRINT_VALUE_CATEGORY(this); // prvalue
	}
};

S{}.foo();
```
* That starts to make sense if we think about `this` as a "special expression that returns a pointer". If we treat the pointer returned by `this` as a "new" entity each time, considering `this` a prvalue makes sense.
* Let's consider the other type of rvalues: the xvalue. Simply put, an xvalue is any expression that results in an unnamed rvalue reference. The obvious suspect is the return of `std::move()`; that's because `std::move()` merely casts the input parameter to an rvalue reference.
```cpp
int x = 1;
PRINT_VALUE_CATEGORY(std::move(x)); // xvalue
PRINT_VALUE_CATEGORY(static_cast<int&&>(x)); // xvalue
```
* xvalues were introduced to mark objects whose resources can be reused, so-called "eXpiring values". That's the whole area of move semantics.
## Summary of value categories
* We've discussed the three primary value categories: lvalues, xvalues, and prvalues. These belong to two higher-level categories: glvalues and rvalues. glvalue stands for "generalized lvalue" and it can be either an lvalue or an xvalue. An rvalue can be either an xvalue or a prvalue. The way to think about glvalues is in terms of "identity", or as I prefer to think about it, "placed in memory", and the way to think about rvalues is in terms of movability. We can take resources only from xvalues and prvalues but not from lvalues.
 ![[Pasted image 20261106130314.png]]
## Applications of value category understanding
* Now that we've discussed and understood value categories, a whole new dimension of C++ understanding opens to us. For example, what do we understand under the term "move semantics"? It's the preference of function overloads taking rvalue references for rvalues. https://godbolt.org/z/Pas4c5YTW
* Take a look at the following code. Which overload will be picked if we `std::move(l)` as an argument? `std::move(l)` is an xvalue and, as we now know, an xvalue prefers to bind to an rvalue reference. It cannot bind to an lvalue reference, unless the lvalue reference is const.
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
* Now, consider creating the object directly in the function call. Which function overload will be called now? The freshly created object is a prvalue. The C++ standard specifies the so-called prvalue-to-xvalue conversion that happens whenever we have a prvalue, but we need an xvalue. In this case, the prvalue is converted to an xvalue, and, thus, binds to the rvalue reference.
* Another example is the forwarding reference and `std::forward()`. What is a forwarding reference? https://godbolt.org/z/qr9WjWTP7
```cpp
template <typename T>
void foo(T&& forwarding_ref) {}

int i = 1;
foo(i); // calls foo<int&>(int&)
foo(0); // calls foo<int>(int&&)
foo(std::move(i)); // calls foo<int>(int&&)
```
* It is a reference that can bind to all, lvalues, xvalues, and prvalues alike. The thing is, it just differentiates between lvalues and rvalues; it does not differentiate between xvalues and prvalues. That means that the receiving function doesn't know if it was called with an xvalue or a prvalue; all it sees is an rvalue reference. So if we call `foo()` with a prvalue, a prvalue-to-xvalue conversion will happen, barring us from knowing whether `foo()` was called with a prvalue or an xvalue. That's also why we cannot have overloads taking a prvalue and an xvalue; such overloads are ambiguous.
* What does `std::forward()` do? It passes its arguments, which are always forwarding references, to a function call, preserving their value category. It is used in conjunction with forwarding references. But this isn't true value category preservation because, at the point of the call, we already lost the information on xvalues and prvalues. We just have an rvalue reference. That's why "perfect forwarding" is imperfect.
* An interesting aside is that in C++17 we got a new and simpler syntax for forwarding references.
```cpp
template <typename T>
void foo(T&& forwarding_ref) {}

void foo_cpp17(auto&& forwarding_ref) {}
```
* As you can see, we no longer have to write `template <typename T>` to be able to create forwarding references, which makes templated code much less verbose. But it leads to a new problem: how can we call `std::forward()` on the forwarding reference specified using `auto&&?
```cpp
template <typename T>
void foo(T&& forwarding_ref) {
	bar(std::forward<T>(forwarding_ref));
}

void foo_cpp17(auto&& forwarding_ref) {
	bar(???);
}
```
* `std::forward()` always requires specifying the type so that the value category is preserved. At thinkcell, we have a dedicated macro that fixes this problem.
```cpp
template <typename T>
void foo(T&& forwarding_ref) {
	bar(std::forward<T>(forwarding_ref));
}

void foo_cpp17(auto&& forwarding_ref) {
	bar(tc_move_if_owned(forwarding_ref));
}
```
* `tc_move_if_owned()` is represented in terms of reference collapsing.
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
* If you don't know what reference collapsing is, these are simply rules that explain what happens when you have a reference to a reference. In this notation, an rvalue reference to an lvalue reference will stay an lvalue reference, while an rvalue reference to a value or an rvalue reference will become an rvalue reference. That macro allows preserving the value category of arguments in heavily templated code while benefiting from the terseness of the `auto&&` syntax.
* So let's go back to our initial example.
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
* How to fix it? As you can see, `my_invoke_bad()` returns a value of a type deduced with `auto`. `auto` does not preserve the value category of an expression; it cares only about its type. So, with this use, it will always return a copy of the `Lifetime` object. How to fix it? We cannot know in advance what `func()` returns: a value, an lvalue reference, or an rvalue reference.  Since C++14, we can use `decltype(auto)` as the return type. It does the same thing as `auto` but for expressions, not types.
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
* In the summary of value categories, I told you that you cannot move from lvalues. What you can do is to transform lvalues into xvalues using `std::move()`. https://godbolt.org/z/9seq7Y8n8. But remember that as soon as an xvalue binds to a named rvalue reference, it becomes an lvalue. We can of course move from it again to obtain an xvalue, but a named rvalue reference on its own is an lvalue.
```cpp
void test_rvalue_ref_arg(int&& x_ref_ref) {
	PRINT_VALUE_CATEGORY(x_ref_ref); // lvalue!
	PRINT_VALUE_CATEGORY(std::move(x_ref_ref)); // xvalue
}

int b = 1;
test_rvalue_ref_arg(std::move(b));
test_rvalue_ref_arg(1); // prvalue-to-xvalue conversion
```
* Now, `std::move()` moves from lvalues, but it has certain drawbacks. What happens when you try to move from const objects? The compiler will happily create a copy for you, which is probably not what you want. What happens when you try to move from an lvalue reference? The compiler will allow the move of the referenced-to object, which again probably is something you don't want. https://godbolt.org/z/xc44Pzaq8
```cpp
Lifetime l{"foo"};
Lifetime& l_ref = l;
Lifetime const& l_constref = l;
Lifetime const l_const{"foo_const"};

Lifetime l_constref_moved = std::move(l_constref); // copy
Lifetime l_const_moved = std::move(l_const); // copy
Lifetime l_ref_moved = std::move(l_ref); // move
```
* You may think that these are obvious mistakes to avoid, but why not err on the safe side? To this end, at thinkcell, we're using our own version of `std::move()`. It is implemented similarly to the following code. https://godbolt.org/z/8ddKsY894
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
	safe_move(j_ref_ref)); // OK, because an lvalue
}
```
* How do we achieve this safety? By using a macro that obtains the type of the expression passed (using `decltype()` with single parentheses) and passing the expression as a forwarding reference. In this way, we can ensure that the variable we pass to `safe_move()` was not declared as an lvalue reference. This single check cannot be inferred from the perfectly forwarded expression; it must be specified by the user of the `safe_move()` function; that's why we need a macro. You can find this macro in our library as `tc_move()`.
* If you really want to move from an lvalue reference or an rvalue, we have a function that's called `tc_move_always()`. It assumes that you know what you're doing, but it still doesn't allow you to move from const objects. For that, we have another function called `tc_move_always_even_const()`. But you better be ready to justify using it in the review!
---
### How `tc::temporary()` + `tc_invoke()` allows relocation
* _I could start with the decltype(auto) max(&&, &&) function, show tc_auto_cref() and finally move to `tc::temporary()`_
## Value Category Conversions Summary
* xvalue-to-lvalue: binding to an rvalue reference?
* prvalue -> xvalue: prvalue-to-xvalue conversion
* lvalue -> xvalue: `std::move`
	* copy if const lvalue
* prvalue -> lvalue:
	* an assignment
	* `tc::as_lvalue`
* lvalue -> prvalue: relocation proposal? (it's more oriented toward perfect forwarding of temporaries, but it can also be used with lvalues).
## To consider
* Now that we have discussed all three primary value categories: lvalues, prvalues, and xvalues, it's time for a little test. What are the value categories of the following expressions? [TODO: lack of reference collapsing]
```cpp

```
* Why is that? Because the standard explicitly says, "If E2 designates an entity that is declared to have type “reference to T”, then E1.E2 is an lvalue of type T." [expr.ref](https://eel.is/c%2B%2Bdraft/expr.ref#:~:text=If%20E2%20designates%20an%20entity%20that%20is%20declared%20to%20have%20type%20%E2%80%9Creference%20to%20T%E2%80%9D%2C%20then%20E1.E2%20is%20an%20lvalue%20of%20type%20T).