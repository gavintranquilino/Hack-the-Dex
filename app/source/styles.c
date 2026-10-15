#include "styles.h"

#include "drawing.h"

static void rounded_fill(u16* screen, int x, int y, int width, int height,
                         int radius, u16 color) {
    if (width <= 0 || height <= 0) return;
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;
    if (radius < 0) radius = 0;

    for (int dy = 0; dy < height; dy++) {
        int py = y + dy;
        if (py < 0 || py >= UI_SCREEN_HEIGHT) continue;
        for (int dx = 0; dx < width; dx++) {
            int px = x + dx;
            if (px < 0 || px >= UI_SCREEN_WIDTH) continue;
            int corner_x = dx < radius ? radius - 1 - dx
                         : (dx >= width - radius ? dx - (width - radius) : 0);
            int corner_y = dy < radius ? radius - 1 - dy
                         : (dy >= height - radius ? dy - (height - radius) : 0);
            if (corner_x * corner_x + corner_y * corner_y <= radius * radius)
                screen[py * UI_SCREEN_WIDTH + px] = color | BIT(15);
        }
    }
}

void draw_rounded_box(u16* screen, int x, int y, int width, int height,
                      int radius, u16 fill, u16 border) {
    rounded_fill(screen, x, y, width, height, radius, border);
    rounded_fill(screen, x + 2, y + 2, width - 4, height - 4,
                 radius - 2, fill);
}


static u16 surface_color(UiSurface surface) {
    switch (surface) {
        case UI_SURFACE_SELECTED: return UI_COLOR_SELECTED;
        case UI_SURFACE_MUTED: return UI_COLOR_MUTED;
        case UI_SURFACE_HIGHLIGHT: return UI_COLOR_HIGHLIGHT;
        case UI_SURFACE_SUCCESS: return UI_COLOR_SUCCESS;
        case UI_SURFACE_ERROR: return UI_COLOR_ERROR;
        case UI_SURFACE_CARD:
        default: return UI_COLOR_CARD;
    }
}

void ui_clear(u16* screen) {
    dmaFillHalfWords(UI_COLOR_BACKGROUND, screen,
                     UI_SCREEN_PIXELS * sizeof(u16));
}

void ui_begin_screen(u16* screen) {
    ui_clear(screen);
    draw_rounded_box(screen, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 10,
                     UI_COLOR_BACKGROUND, UI_COLOR_FRAME);
}

void ui_draw_text(u16* screen, const UiText* text) {
    if (!text || !text->value) return;
    print_text(text->value, text->x, text->y, screen,
               text->color, text->scale);
}

void ui_draw_card(u16* screen, const UiCard* card) {
    if (!card) return;
    draw_rounded_box(screen, card->bounds.x, card->bounds.y,
                     card->bounds.width, card->bounds.height, card->radius,
                     surface_color(card->surface), UI_COLOR_INK);
}

void ui_draw_raised_card(u16* screen, const UiCard* card) {
    if (!card) return;
    draw_rounded_box(screen, card->bounds.x, card->bounds.y + 3,
                     card->bounds.width, card->bounds.height, card->radius,
                     UI_COLOR_SHADOW, UI_COLOR_SHADOW);
    ui_draw_card(screen, card);
}

void ui_draw_button(u16* screen, const UiButton* button) {
    if (!button) return;
    ui_draw_raised_card(screen, &button->card);
    ui_draw_text(screen, &button->label);
}

void ui_draw_page_header(u16* screen, const UiPageHeader* header) {
    if (!header) return;
    ui_draw_text(screen, &header->title);
    ui_draw_text(screen, &header->hint);
}
