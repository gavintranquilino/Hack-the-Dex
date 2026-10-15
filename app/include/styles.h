#ifndef STYLES_H
#define STYLES_H

#include <nds.h>

/*
 * Shared visual language for every bitmap UI screen.  Keep layout decisions
 * in each screen module; put reusable colours and chrome here instead.
 */
#define UI_SCREEN_WIDTH 256
#define UI_SCREEN_HEIGHT 192
#define UI_SCREEN_PIXELS (UI_SCREEN_WIDTH * UI_SCREEN_HEIGHT)

#define UI_COLOR_BACKGROUND (RGB15(31, 31, 31) | BIT(15))
#define UI_COLOR_INK        (RGB15(4, 5, 8) | BIT(15))
#define UI_COLOR_FRAME      (RGB15(28, 14, 15) | BIT(15))
#define UI_COLOR_HEADING    (RGB15(21, 5, 7) | BIT(15))
#define UI_COLOR_CARD       (RGB15(31, 31, 31) | BIT(15))
#define UI_COLOR_SELECTED   (RGB15(31, 21, 21) | BIT(15))
#define UI_COLOR_MUTED      (RGB15(22, 27, 31) | BIT(15))
#define UI_COLOR_HIGHLIGHT  (RGB15(31, 29, 19) | BIT(15))
#define UI_COLOR_SUCCESS    (RGB15(23, 29, 23) | BIT(15))
#define UI_COLOR_INFO       (RGB15(3, 13, 10) | BIT(15))
#define UI_COLOR_ERROR      (RGB15(23, 4, 6) | BIT(15))
#define UI_COLOR_SECONDARY  (RGB15(12, 13, 14) | BIT(15))
#define UI_COLOR_SHADOW     UI_COLOR_INK

typedef enum {
    UI_SURFACE_CARD,
    UI_SURFACE_SELECTED,
    UI_SURFACE_MUTED,
    UI_SURFACE_HIGHLIGHT,
    UI_SURFACE_SUCCESS,
    UI_SURFACE_ERROR
} UiSurface;

/* Every visible UI element is described with data before it is drawn. */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} UiRect;

typedef struct {
    const char* value;
    int x;
    int y;
    u16 color;
    int scale;
} UiText;

typedef struct {
    UiRect bounds;
    int radius;
    UiSurface surface;
} UiCard;

typedef struct {
    UiCard card;
    UiText label;
} UiButton;

typedef struct {
    UiText title;
    UiText hint;
} UiPageHeader;

void draw_rounded_box(u16* screen, int x, int y, int width, int height,
                      int radius, u16 fill, u16 border);

void ui_clear(u16* screen);
void ui_begin_screen(u16* screen);
void ui_draw_text(u16* screen, const UiText* text);
void ui_draw_card(u16* screen, const UiCard* card);
void ui_draw_raised_card(u16* screen, const UiCard* card);
void ui_draw_button(u16* screen, const UiButton* button);
void ui_draw_page_header(u16* screen, const UiPageHeader* header);

#endif
