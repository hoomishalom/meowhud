A simple Wayland status HUD, inspired by [bread](https://github.com/netfri25/bread).

# Usage
Needs a "content creator", a program that makes the frames
and defines the [options](optoins).

```
content_creator | meowhud
```

In addition, can be used manually by just running ```meowhud```,
this is one of the design motiviations behind the final
protocol.

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
data ended by an `DONE` ([list](#option-list)). For example:

```
font_count_max;2
font_name;First Font Name:size=12
font_name;Second Font Name:size=14:slant=italic
bg_color;ff001122
anchor;1001
DONE
```

### Option List

#### Font

* `font_count_max`: (Required) An upper bound on the amount of fonts that will be given.
* `font_name`: (At least one is required) A string representing an installed font and attributes (the second font and onwards will be used as fallbacks in the given order) format: `Name Of Font:attribute2=value1:attribute1=value2:...`.
* `default_text_color`: The default color that is used when a color isn't given, format: `AARRGGBB`.

#### Window

* `width`: (Required) The width of the window.
* `height`: The height of the window (if not given, height
will be calcualted to allow exactly `row_count` rows).
* `row_count`: (Required) The amount of row that will be used.
* `bg_color`: (Required) The color of the background, format: `AARRGGBB`.
* `anchor`: (Required) Anchoring bit mask `right||left||bottom||top` (all 0's for centered, example: top-right `1001`).

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
