# (Im)Perfect Forwarding: A Practical Guide to Value Categories in C++

Slides and code for Jan Wilczek's talk at **code::dive 2026**.

📄 **[Download the slides as PDF](./slides-export.pdf)**

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
| [`slides-export.pdf`](./slides-export.pdf) | PDF export of the slide deck |
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
pnpm export                    # PDF export to slides-export.pdf
```

`pnpm export` regenerates `slides-export.pdf`, so re-run it after changing the slides. The export options live in the `export:` block of `slides.md`'s headmatter: one page per slide, with all click animations revealed (`perSlide: true`, `withClicks: false`).

Do not enable `withClicks`. With Slidev 53.1, it shrinks the slides on a black background and drops the last slides from the PDF.

## Compiling the snippets

The snippets need a C++20 compiler:

```bash
g++ -std=c++20 snippets/expression_type.cpp
```

