<div align="center">
  <img src="docs/images/logo.png" alt="Coi Logo" width="200"/>

# Coi

A component-based language for high-performance web apps. 
**Fast. Minimal. Type-safe.**

Compiles to WASM, JS, and HTML with tiny binaries and efficient updates for DOM, Canvas, and beyond.
</div>

## Features

> [!WARNING]
> This project is a **work in progress** and is not recommended for use in production projects. The language features and syntax are not yet set in stone and are subject to change.

- **Minimal & Fast**: Compiles to tiny WASM binaries with no heavy runtime. High-performance updates for DOM, Canvas, and more.
- **Type-Safe Components**: Catch errors at compile time. Props and state are strictly typed.
- **HTML & CSS**: Write standard HTML and scoped CSS directly in your components.
- **Update Loop**: Integrated `tick` lifecycle method for animations and updates.
- **Component Composition**: Build complex UIs from small, reusable components with typed `prop`s.
- **App Configuration**: Define your app entry point with a simple `app` block.

## Example

```coi
component Button {
    prop string label;
    
    style {
        button {
            background: #6c5ce7;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
            transition: opacity 0.2s;
        }
        button:hover {
            opacity: 0.9;
        }
    }

    view {
        <button onclick={onclick}>{label}</button>
    }
}

component Counter {
    int count = 0;

    def increment() : void {
        count++;
    }

    style {
        .container {
            padding: 20px;
            font-family: sans-serif;
            text-align: center;
        }
        h1 { color: #2d3436; }
    }

    view {
        <div class="container">
            <h1>Count: {count}</h1>
            <Button label="Increment" onclick={increment} />
        </div>
    }
}

app {
    root = Counter
}
```

## Building

To build the compiler:

```bash
./build.sh
```

## Usage

To compile a `.coi` file:

```bash
coi example/test.coi
```

## VS Code Extension

A VS Code extension for syntax highlighting is included in the `vscode-extension/` directory. To use it:

1. Copy the `vscode-extension/` folder to your VS Code extensions directory (usually `~/.vscode/extensions/`).
2. Or, open the `vscode-extension/` folder in VS Code and press `F5` to run a development instance with the extension enabled.

## Technical Details

Coi is built on top of the [WebCC](https://github.com/io-eric/webcc) toolchain and framework. It generates optimized C++ code which is then compiled to minimal WebAssembly binaries and JavaScript glue code.
