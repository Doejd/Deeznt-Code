# Deeznt-Code

A terminal emulator built for the **Godot Engine**, written in C++ using GDExtension.

The project aims to provide a native-feeling Linux terminal inside Godot, with ANSI color support, command history, scrollback, text highlighting, and efficient handling of large amounts of terminal output.

> 🚧 This project is currently under active development.

---

## Demo
[▶ Watch the demo] https://youtu.be/Lqh1pO1gkgA

---

## Features

* Linux PTY-based terminal
* Bash shell integration
* ANSI escape sequence parsing
* 16-color ANSI support
* 256-color ANSI support
* RGB / True Color support
* Foreground and background colors
* Command history
* Keyboard navigation
* Terminal scrollback
* Custom syntax highlighting
* Fixed-width terminal rendering
* Automatic line wrapping
* Efficient line storage using a ring buffer
* Configurable maximum scrollback size
* Godot `TextEdit` integration

---

## How It Works

The terminal launches a Linux shell through a pseudo-terminal:

```text
Godot
  │
  ▼
LinuxHost
  │
  ├── openpty()
  │
  ▼
PTY Master
  │
  ▼
Bash
```

Output received from the PTY is processed before being displayed:

```text
Raw PTY Output
      │
      ▼
ANSI Parser
      │
      ├── Text
      └── Styling information
              │
              ▼
        Segment Ring Buffer
              │
              ▼
       Godot TextEdit
```

The terminal text itself is stored by Godot's `TextEdit`, while highlighting information is stored separately as `Segment` objects.

---

## ANSI Highlighting

Terminal output can contain ANSI escape sequences such as:

```text
ESC[31m
ESC[38;5;196m
ESC[38;2;255;120;40m
```

The parser separates these control sequences from the displayed text and stores the active styling information in a `Segment`.

Example:

```cpp
struct Segment {
    godot::String text{""};

    uint32_t color{0xffffff};
    uint32_t bg_color{0x000000};

    int32_t starting_column{0};

    bool hasBg{false};
    bool bold{false};
};
```

Each terminal line can contain multiple segments:

```text
Line
 │
 ├── Segment 1: white
 ├── Segment 2: red
 ├── Segment 3: green
 └── Segment 4: white
```

The custom Godot syntax highlighter then applies the stored styles to the corresponding regions of the `TextEdit`.

---

## Requirements

* Linux
* Godot 4.x
* C++17 or newer
* Godot C++ bindings / GDExtension
* A C++ compiler supported by Godot
* Bash

---

## Building

Clone the repository:

```bash
git clone https://github.com/Doejd/Deeznt-Code.git
cd Deeznt-Code
```

Switch to the development branch if required:

```bash
git checkout main
```

Build the GDExtension using the build system configured for the project.

```bash
scons platform=windows target=template_release
```

---

## Usage

Add the terminal node to your Godot project and start the terminal:

```cpp
startTerminal();
```

Input is written to the PTY while shell output is continuously read and inserted into the Godot `TextEdit`.

---

## Current Limitations

This project is still experimental.

Some terminal behavior may not yet be fully implemented, including:

* advanced cursor movement escape sequences
* alternate screen buffers
* full VT100 / VT220 compatibility
* terminal resize signaling
* complex Unicode width handling
* advanced text attributes
* interactive TUI application compatibility


## Development Status

The project is currently focused on building the core terminal rendering pipeline:

```text
PTY
 ↓
ANSI parsing
 ↓
Segment generation
 ↓
Ring buffer
 ↓
Godot rendering
```

Performance and correctness are currently prioritized over complete terminal emulation.

---

## Contributing

Contributions, bug reports, and suggestions are welcome.

If you find an issue, please include:

* Godot version
* Linux distribution/Windows version
* steps to reproduce
* terminal output or command that triggered the issue
* screenshots or video when applicable

---

## Author

Developed by [Doejd](https://github.com/Doejd)

---

## Acknowledgements

Built with:

* [Godot Engine](https://godotengine.org/)
* [godot-cpp](https://github.com/godotengine/godot-cpp)
* Linux pseudo-terminal APIs

