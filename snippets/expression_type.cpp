#include <concepts>

int main() {
  auto i = 1;
  static_assert(std::same_as<decltype(i), decltype(1)>);

  std::decay_t<decltype(1)> i_decl = 1;
  static_assert(std::same_as<decltype(i_decl), decltype(i)>);

  auto j = 1 + 2;
  static_assert(std::same_as<decltype(j), decltype(1+2)>);

  std::decay_t<decltype(1+2)> j_decl = 1+2;
  static_assert(std::same_as<decltype(j_decl), decltype(j)>);

  int a = 1;
  auto k = a;
  static_assert(std::same_as<decltype(k), decltype(a)>);

  std::decay_t<decltype(a)> k_decl = a;
  static_assert(std::same_as<decltype(k_decl), decltype(k)>);

  int b = 1;
  int& b_ref = b;
  auto l = b_ref;
  // static_assert(std::same_as<decltype(l), decltype(b_ref)>); // int != int&
  static_assert(std::same_as<decltype(l), std::decay_t<decltype(b_ref)>>);

  std::decay_t<decltype(b_ref)> l_decl = b_ref;
  static_assert(std::same_as<decltype(l_decl), decltype(l)>);
}
