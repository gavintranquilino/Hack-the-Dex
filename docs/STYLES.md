# DS UI Styles

The DSi application draws directly into 256x192 16-bit bitmap buffers. It
does not use a GUI toolkit, so `app/include/styles.h` provides a small shared
UI vocabulary. Each visible element is represented by a struct and drawn by
`app/source/styles.c`.

Screen modules own layout and interaction. The styles module owns recurring
visual treatment: colours, rounded borders, shadows, cards, buttons, and
page headers.

## Start a screen

Include `styles.h` in the screen module and begin each static redraw with:

```c
ui_begin_screen(screen);
```

This clears the bitmap to the shared background and draws the rounded outer
frame. Use it for normal app pages. Do not use it over a live camera preview
or a canvas that must be preserved. `ui_clear(screen)` is available when only
the background needs to be reset.

## Element data

All public UI elements are structs. This keeps geometry, content, and visual
state together instead of spreading a component's coordinates through a long
function call.

### `UiRect`

```c
UiRect bounds = { .x = 8, .y = 48, .width = 240, .height = 61 };
```

`UiRect` is the position and size shared by cards and buttons. Coordinates are
pixels from the screen's upper-left corner. Keep bounds inside the 256x192
screen when possible; the low-level rounded renderer clips safely.

### `UiText`

```c
UiText caption = {
    .value = "LINK STATUS",
    .x = 18, .y = 57,
    .color = UI_COLOR_INK,
    .scale = 1
};
ui_draw_text(screen, &caption);
```

Use `UiText` for text that is itself a UI element: titles, hints, and button
labels. `value` is the text, `x` and `y` are its bitmap position, `color` is a
shared `UI_COLOR_*` value, and `scale` is currently 1 or 2. The renderer
ignores null text values, so an optional button label may be empty.

### `UiCard`

```c
UiCard status_card = {
    .bounds = { .x = 8, .y = 48, .width = 240, .height = 99 },
    .radius = 9,
    .surface = UI_SURFACE_CARD
};
ui_draw_card(screen, &status_card);
```

A card groups related static content, such as a status region, form group, or
profile section. `ui_draw_card()` draws the normal rounded border. Use
`ui_draw_raised_card()` when a card needs the dark three-pixel shadow used for
a raised decorative panel but is not an action the player can invoke.

### `UiButton`

```c
UiButton save_button = {
    .card = {
        .bounds = { .x = 8, .y = 153, .width = 150, .height = 25 },
        .radius = 7,
        .surface = UI_SURFACE_SUCCESS
    },
    .label = {
        .value = "A SAVE DRAWING",
        .x = 41, .y = 162,
        .color = UI_COLOR_INK,
        .scale = 1
    }
};
ui_draw_button(screen, &save_button);
```

Use a button for a visible action with matching input handling elsewhere in
the screen: an A-button action or a touch target. It draws a raised card and
then its label. The struct describes appearance only; input stays in the
screen module so touch bounds and navigation remain close to screen state.

### `UiPageHeader`

```c
UiPageHeader header = {
    .title = {
        .value = "FIND A FRIEND",
        .x = 12, .y = 10,
        .color = UI_COLOR_HEADING,
        .scale = 2
    },
    .hint = {
        .value = "01 SCAN > 02 PHOTO > 03 DRAW",
        .x = 12, .y = 32,
        .color = UI_COLOR_INK,
        .scale = 1
    }
};
ui_draw_page_header(screen, &header);
```

Use a page header when a page has both a title and a short instructional or
progress hint. It keeps the two pieces of standard page chrome paired. For a
title with no hint, draw one `UiText` instead.

## Surface states

`UiSurface` selects a themed fill while keeping the same dark border. Choose
by meaning, not by the exact RGB value:

| Surface | Use |
| --- | --- |
| `UI_SURFACE_CARD` | Default content groups and neutral controls. |
| `UI_SURFACE_SELECTED` | Current selection or a strong destructive/cancel action. |
| `UI_SURFACE_MUTED` | Secondary, inactive, or waiting actions. |
| `UI_SURFACE_HIGHLIGHT` | Focused value, pending action, or attention callout. |
| `UI_SURFACE_SUCCESS` | A confirm, save, or keep action. |
| `UI_SURFACE_ERROR` | An error state or failure callout. |

The shared `UI_COLOR_*` constants are for direct text and low-level drawing.
Prefer a surface enum for a card or button so screens do not repeat palette
values.

## Low-level primitive

`draw_rounded_box()` is the lowest-level shared renderer. It accepts raw
coordinates and colours and is appropriate only when the standard elements
cannot express a component, such as the drawing tool's coloured ink swatches
or bitmap canvas frame. Normal app chrome should use `UiCard` or `UiButton`.

## C syntax note

Short-lived UI elements can be passed as C99 compound literals:

```c
ui_draw_card(screen, &(UiCard){
    .bounds = { 8, 48, 240, 99 },
    .radius = 9,
    .surface = UI_SURFACE_CARD
});
```

This is standard C99. For an element used more than once, or one with dynamic
state, prefer a named local `UiCard`, `UiButton`, or `UiPageHeader` variable.
