A simple status HUD.

# Protocol Explanation
## Options
Firstly, the first message should be a sequence of options
data ended by an `DONE` ([list](#option-list)). For example:

```
font_count;2
font_size;24
font_name;First Font Name
font_name;Second Font Name
bg_color;0xffff000011112222
anchor;1001
DONE
```

### Option List

#### Font

* `font_count`: The amount of fonts that will be given.
* `font_name`: A string representing an installed font (the second font and onwards will be used as fallbacks in the given order).
* `font_size`: The size of the font (uniform accross main
and fallback fonts).

#### Window

* `width`: The width of the window.
* `height`: The height of the window (if not given, height
will be calcualted to allow exactly `line_count` lines).
* `line_count`: The amount of lines that will be used.
* `bg_color`: The color of the background, format is: `0xAAAARRRRGGGGBBBB`.
* `anchor`: Anchoring bit mask `top||bottom||left||right` (all 0's for centered, example top-right `1001`).

## Frames

Then, every following message should be one containing a
frame. For example:

```
1;4;meow;6;meow :)
2;11;I like cats;0;
```

And in general, each line should look like:

`line number;left length;left text;right length;right text`
