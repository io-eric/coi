# __PROJECT_NAME__

A Coi component library.

## Installation

Copy this library folder into your project and import it.

## Usage

```tsx
// Import the library (re-exports all public components)
import "__PROJECT_NAME__/Lib.coi";

component App {
    view {
        <__PROJECT_NAME__::Button label="Click me" />
    }
}
```

## Structure

```
__PROJECT_NAME__/
├── Lib.coi           # Library entry point (pub imports)
├── registry-entry.json # Ready-to-submit registry metadata template
├── src/
│   ├── ui/           # UI components
│   │   └── Button.coi
│   └── api/          # API utilities
│       └── Client.coi
└── README.md
```

## Exports

**UI Components:**
- `Button` - A customizable button component

**API:**
- `Client` - HTTP client placeholder

## Development

To test components, create a test app that imports this library:

```bash
coi init test-app
cd test-app
# Copy your library into the project and import it
coi dev
```

## Publish to Registry

This template includes `registry-entry.json` — your library's registry file.

Before submitting:

1. Replace `YOUR_GITHUB_USERNAME` with your GitHub username or org in `repository`
2. Fill in `description` and `keywords`
3. Set `compiler-drop.min` (optimistic) and `compiler-drop.tested-on` (verified)
4. Keep your GitHub repo license as MIT (registry CI enforces this)

To submit:

1. Copy `registry-entry.json` to the registry repo as `libraries/__PROJECT_NAME__.json`
2. (Optional) place it in a shard directory, e.g. `libraries/ab/__PROJECT_NAME__.json`
3. Open a PR

When releasing new versions, add entries to the `releases` array in your library file.

## Learn More

- [Language Guide](https://github.com/io-eric/coi/blob/main/docs/language-guide.md)
- [Components](https://github.com/io-eric/coi/blob/main/docs/components.md)
- [Modules & Imports](https://github.com/io-eric/coi/blob/main/docs/language-guide.md#modules-and-imports)
