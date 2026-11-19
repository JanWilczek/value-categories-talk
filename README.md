# (Im)Perfect Forwarding: A Practical Guide to Value Categories in C++

Slides and code for Jan Wilczek's talk at **code::dive 2026**.

The talk covers:

- lvalues, prvalues, and xvalues, and how to tell them apart
- `decltype(x)` vs. `decltype((x))`
- move semantics, forwarding references, and `std::forward`
- relocating prvalues (P2785)
- practical applications from the [think-cell library](https://github.com/think-cell/think-cell-library), e.g., `tc::temporary`

## Repository layout

| Path | Contents |
| --- | --- |
| [`slides.md`](./slides.md) | The slide deck ([Slidev](https://sli.dev/)) |
| [`snippets/`](./snippets) | C++ code examples |
| [`godbolt_links.md`](./godbolt_links.md) | Compiler Explorer links to the examples shown in the talk |
| [`assets/`](./assets) | Images used in the slides |

## Running the slides locally

Requires Node.js 24 and [pnpm](https://pnpm.io/).

```bash
pnpm install
pnpm dev
```

Then open <http://localhost:3030>.

### Building and exporting

```bash
pnpm build                     # static site in dist/
pnpm export                    # PDF export
```

Exporting needs `playwright-chromium`. Install it with `pnpm add -D playwright-chromium` if `pnpm export` reports it missing.

## Compiling the snippets

The snippets need a C++20 compiler:

```bash
g++ -std=c++20 snippets/expression_type.cpp
```

