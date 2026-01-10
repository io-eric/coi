# Platform APIs

Coi provides type-safe access to browser APIs through the [WebCC](https://github.com/io-eric/webcc) toolchain. These APIs are **automatically generated** from the WebCC schema.

## Type System

All platform types use a consistent pattern:

- **`type`**: Defines a handle type (like `Canvas`, `Image`, `DOMElement`)
- **`shared def`**: Static/factory methods called on the type itself
- **`def`**: Instance methods called on an instance

```tsx
// Static method
Image photo = Image.load("photo.png");

// Instance method
canvas.setSize(800, 600);
ctx = canvas.getContext2d();
```

## Canvas

2D drawing, paths, text, images, and transformations.

### Setup

```tsx
component CanvasApp {
    mut Canvas canvas;
    mut CanvasContext2D ctx;
    
    mount {
        canvas.setSize(800, 600);
        ctx = canvas.getContext2d();
    }
    
    view {
        <canvas &={canvas}></canvas>
    }
}
```

### Drawing

```tsx
// Shapes
ctx.fillRect(x, y, width, height);
ctx.strokeRect(x, y, width, height);
ctx.clearRect(x, y, width, height);

// Paths
ctx.beginPath();
ctx.moveTo(x, y);
ctx.lineTo(x, y);
ctx.arc(x, y, radius, startAngle, endAngle);
ctx.closePath();
ctx.fill();
ctx.stroke();

// Styles
ctx.setFillStyle(r, g, b);
ctx.setFillStyleStr("#ff0000");
ctx.setStrokeStyle(r, g, b);
ctx.setStrokeStyleStr("#00ff00");
ctx.setLineWidth(2.0);

// Text
ctx.setFont("16px Arial");
ctx.fillText("Hello", x, y);
ctx.strokeText("Hello", x, y);

// Images
ctx.drawImage(image, x, y);
ctx.drawImageScaled(image, x, y, width, height);
```

### Example: Bouncing Ball

```tsx
component AnimatedBall {
    mut Canvas canvas;
    mut CanvasContext2D ctx;
    mut float x = 100.0;
    mut float y = 100.0;
    mut float dx = 3.0;
    mut float dy = 2.0;

    mount {
        canvas.setSize(800, 600);
        ctx = canvas.getContext2d();
    }

    tick(float dt) {
        ctx.clearRect(0, 0, 800, 600);
        ctx.setFillStyle(66, 133, 244);
        ctx.beginPath();
        ctx.arc(x, y, 20, 0, 6.28318);
        ctx.fill();

        x += dx;
        y += dy;
        if (x < 20 || x > 780) dx = -dx;
        if (y < 20 || y > 580) dy = -dy;
    }

    view {
        <canvas &={canvas}></canvas>
    }
}
```

## Image

Load and draw images on canvas.

```tsx
component Gallery {
    mut Canvas canvas;
    mut CanvasContext2D ctx;
    Image photo;

    mount {
        canvas.setSize(400, 300);
        ctx = canvas.getContext2d();
        photo = Image.load("photo.png");
    }

    def draw() : void {
        ctx.drawImage(photo, 0, 0);
    }

    view {
        <div>
            <canvas &={canvas}></canvas>
            <button onclick={draw}>Draw</button>
        </div>
    }
}
```

## Storage

Local storage for persisting data.

```tsx
// Set item
Storage.setItem("key", "value");

// Remove item
Storage.removeItem("key");

// Clear all
Storage.clear();
```

### Example

```tsx
component Settings {
    mut string theme = "light";

    def saveTheme() : void {
        Storage.setItem("theme", theme);
    }

    def toggleTheme() : void {
        theme = theme == "light" ? "dark" : "light";
        saveTheme();
    }

    view {
        <button onclick={toggleTheme}>Theme: {theme}</button>
    }
}
```

## Audio

Audio playback with volume and looping.

```tsx
component MusicPlayer {
    mut Audio music;
    mut bool playing = false;

    mount {
        music = Audio.load("song.mp3");
        music.setVolume(0.8);
        music.setLoop(true);
    }

    def toggle() : void {
        if (playing) {
            music.pause();
        } else {
            music.play();
        }
        playing = !playing;
    }

    view {
        <button onclick={toggle}>
            {playing ? "Pause" : "Play"}
        </button>
    }
}
```

## System

Logging, page title, time, and URL navigation.

```tsx
// Logging
System.log("Debug message");
System.logInt(42);
System.logFloat(3.14);

// Page title
System.setTitle("My App");

// Time (milliseconds since epoch)
int now = System.getTime();

// URL navigation
System.openUrl("https://example.com");
```

## Input

Keyboard and mouse input handling.

```tsx
component Game {
    mut float x = 400;
    mut float y = 300;
    mut float speed = 200;

    tick(float dt) {
        if (Input.isKeyDown(37)) x -= speed * dt;  // Left
        if (Input.isKeyDown(39)) x += speed * dt;  // Right
        if (Input.isKeyDown(38)) y -= speed * dt;  // Up
        if (Input.isKeyDown(40)) y += speed * dt;  // Down
    }

    view {
        <div style="left: {x}px; top: {y}px;"></div>
    }
}
```

## DOMElement

Direct DOM manipulation.

```tsx
component App {
    mut DOMElement container;

    mount {
        DOMElement div = DOMElement.createElement("div");
        div.setInnerHtml("<p>Dynamic content</p>");
        container.appendChild(div);
    }

    view {
        <div &={container}></div>
    }
}
```

## Fetch

HTTP requests.

```tsx
component DataLoader {
    mut string data = "";

    def loadData() : void {
        Fetch.get("https://api.example.com/data", onSuccess, onError);
    }

    def onSuccess(string response) : void {
        data = response;
    }

    def onError(string error) : void {
        System.log("Error: " + error);
    }

    view {
        <div>
            <button onclick={loadData}>Load</button>
            <p>{data}</p>
        </div>
    }
}
```

## WebSocket

Real-time communication.

```tsx
component Chat {
    mut WebSocket ws;
    mut string[] messages;

    mount {
        ws = WebSocket.connect("wss://chat.example.com");
        ws.onMessage(handleMessage);
    }

    def handleMessage(string msg) : void {
        messages.push(msg);
    }

    def send(string text) : void {
        ws.send(text);
    }

    view {
        <div>
            <for msg in messages key={msg}>
                <p>{msg}</p>
            </for>
        </div>
    }
}
```

## Available APIs

| Module       | Description                                      |
|--------------|--------------------------------------------------|
| `Canvas`     | 2D drawing, paths, text, images, transformations |
| `Image`      | Image loading for canvas rendering               |
| `Audio`      | Audio playback, volume, looping                  |
| `Storage`    | Local storage (setItem, removeItem, clear)       |
| `System`     | Logging, page title, time, URL navigation        |
| `Input`      | Keyboard and mouse input, pointer lock           |
| `DOMElement` | Direct DOM manipulation                          |
| `WebGL`      | WebGL context and rendering                      |
| `WGPU`       | WebGPU support                                   |
| `Fetch`      | HTTP requests                                    |
| `WebSocket`  | WebSocket connections                            |

## Next Steps

- [Getting Started](getting-started.md) — Setup and first project
- [Language Guide](language-guide.md) — Types, control flow, operators
- [Components](components.md) — Component lifecycle, state management
