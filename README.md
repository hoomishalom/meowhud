# meowline 🐱

A simple status line for your terminal - because cats like to keep things organized!

## Features

- Display username, hostname, current path, time, and date
- Customizable components
- Watch mode for continuous updates
- Lightweight and easy to use

## Installation

```bash
pip install -e .
```

Or run directly:

```bash
python3 meowline.py
```

## Usage

### Basic usage

Display default status line:
```bash
meowline
```

Output:
```
👤 runner | 🖥️  hostname | 📁 ~/work/project | 🕐 13:18:38
```

### Custom components

Choose which components to display:
```bash
meowline --components user time date
```

Available components:
- `user` - Current username
- `host` - Hostname
- `path` - Current working directory
- `time` - Current time (HH:MM:SS)
- `date` - Current date (YYYY-MM-DD)

### Watch mode

Continuously update the status line:
```bash
meowline --watch
```

With custom update interval (in seconds):
```bash
meowline --watch --interval 2.0
```

## Examples

```bash
# Show user and time only
meowline --components user time

# Show all available components
meowline --components user host path time date

# Watch mode with 0.5 second updates
meowline --watch --interval 0.5
```

## License

MIT