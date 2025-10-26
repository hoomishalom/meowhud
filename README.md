A simple status HUD.

# Protocol Explanation
## Options
Firstly, the first message should be a sequence of options
data ended by an `DONE` ([list](#option-list)). For example:

```
font_count;2
font_name;First Font Name:size=12
font_name;Second Font Name:size=14:slant=italic
bg_color;ff001122
anchor;1001
DONE
```

### Option List

#### Font

* `font_count`: The amount of fonts that will be given.
* `font_name`: A string representing an installed font and attributes (the second font and onwards will be used as fallbacks in the given order) format: `Name Of Font:attribute2=value1:attribute1=value2:...`.

#### Window

* `width`: The width of the window.
* `height`: The height of the window (if not given, height
will be calcualted to allow exactly `row_count` rows).
* `row_count`: The amount of row that will be used.
* `bg_color`: The color of the background, format: `AARRGGBB`.
* `default_text_color`: The default color that is used when a color isn't given, format: `AARRGGBB`.
* `anchor`: Anchoring bit mask `right||left||bottom||top` (all 0's for centered, example top-right `1001`).

## Frames

Then, every following message should be one containing a
frame, ended by a DONE. For example:

```
1;r;4;;meow
1;l;7;ff000000;meow :)
1;l;5;ff00ff00;green
2;l;13;;default color
DONE
```

And in general, each line should look like:

`line number;align (r or l);text length;text color (empty for default);text`
