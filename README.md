A simple Wayland status HUD, inspired by [bread](https://github.com/netfri25/bread).

# Usage
Needs a "content creator", a program that makes the frames
and defines the [options](#options).

```
content_creator | meowhud
```

In addition, can be used manually by just running ```meowhud```,
this is one of the design motiviations behind the final
protocol.

# Build
To build the project, ensure you have the dependencies installed and run:
```bash
make
```

# Dependencies
All dependencies that are not "easily" installed are
included in the project files.
Other dependencies are:
* [fcft](https://codeberg.org/dnkl/fcft)
* pixman
* A Wayland compositor

# Protocol Explanation
## Options
Firstly, the first message should be a sequence of options
data ended by a `DONE` ([list](#option-list)). For example:

```
font_name;Hack Nerd Font:size=24:slant=italic   
width;0
bg_color;80e53939
row_count;5
anchor;1101
row_spacing;20
target_output;all
DONE
```

### Option List

#### Font

* `font_name`: (At least one is required) A string representing an installed font and attributes (the second font and onwards will be used as fallbacks in the given order) format: `Name Of Font:attribute1=value1:attribute2=value2:...`.
* `default_text_color`: The default color that is used when a color isn't given, format: `AARRGGBB` (Hexadecimal).

#### Window

* `width`: (Required) The width of the window. if set to 0, then the compositor will decide the width of the hud (can be useful if combined with left and right anchors).
* `height`: The height of the window (if not given, height
will be calcualted to allow exactly `row_count` rows).
* `row_count`: (Required) The amount of row that will be used.
* `bg_color`: (Required) The color of the background, format: `AARRGGBB` (Hex).
* `anchor`: (Required) Anchoring bit mask in binary format `right|left|bottom|top`. 
    * `1000` = Right
    * `0100` = Left
    * `0010` = Bottom
    * `0001` = Top
    * Examples: `1101` (Top + Left + Right), `0011` (Top + Bottom).
* `target_output`: Defines which monitor(s) to display the HUD on. 
    * `main`: (Default) Shows on the active/focused monitor.
    * `all`: Shows the HUD on all connected monitors simultaneously.
    * `[name1],[name2]`: Comma-separated list of exact monitor names (`DP-1,HDMI-A-1`).

## Frames

Then, every following message should be one containing a
frame, ended by a DONE. For example:

```
1;r;;meow
1;l;ff000000;meow :)
1;l;ff00ff00;green
2;l;;default color
DONE
```

And in general, each line should look like:

`line number (0 < value < row_count);align (r or l);text color (empty for default);text`
