# Getting Started

This guide will help you set up Coi and create your first component.

## Installation

### Prerequisites

Coi requires [WebCC](https://github.com/io-eric/webcc) to be installed. The build script will automatically initialize and build the WebCC submodule if it is not found on your system.

### Building from Source

To build the compiler and the toolchain:

```bash
./build.sh
```

This will:
1. Initialize the WebCC submodule (if needed)
2. Build the WebCC toolchain
3. Build the Coi compiler

## Usage

To compile a `.coi` file:

```bash
coi App.coi --out ./dist
```

This generates:
- `dist/index.html` — Entry HTML file
- `dist/app.js` — JavaScript runtime
- `dist/app.wasm` — WebAssembly binary

To keep the intermediate C++ file for debugging:

```bash
coi App.coi --out ./dist --keep-cc
```

This also generates `dist/App.cc` so you can inspect the generated C++ code.

To run your app locally:

```bash
cd dist
python3 -m http.server
```

Then open `http://localhost:8000` in your browser.

## Your First Component

Create a file called `App.coi`:

```tsx
component App {
    mut int count = 0;

    def increment() : void {
        count += 1;
    }

    style {
        .container {
            padding: 20px;
            font-family: system-ui;
        }
        button {
            padding: 8px 16px;
            cursor: pointer;
        }
    }

    view {
        <div class="container">
            <h1>Count: {count}</h1>
            <button onclick={increment}>+1</button>
        </div>
    }
}

app { root = App; }
```

Compile and run:

```bash
coi App.coi --out ./dist
cd dist && python3 -m http.server
```

## Project Structure

For larger projects, organize your code into multiple files:

```
my-app/
├── src/
│   ├── App.coi
│   ├── components/
│   │   ├── Button.coi
│   │   └── Card.coi
│   └── layout/
│       ├── Header.coi
│       └── Footer.coi
├── dist/           # Generated output
└── build.sh
```

## Imports

Coi supports importing other `.coi` files:

```tsx
import "components/Button.coi";
import "layout/Header.coi";
import "../shared/Utils.coi";
```

Imports are relative to the current file's location. All components defined in imported files become available in the current file.

## Next Steps

- [Language Guide](language-guide.md) — Types, control flow, operators
- [Components](components.md) — Component syntax, lifecycle, props
- [Styling](styling.md) — Scoped and global CSS
- [View Syntax](view-syntax.md) — JSX-like templates, conditionals, loops
- [Platform APIs](api-reference.md) — Canvas, Storage, Audio, and more
