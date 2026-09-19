#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define WIDTH 256
#define HEIGHT 192
#define WHITE (RGB15(31, 31, 31) | BIT(15))
#define BLACK (BIT(15))

static u16 *canvas;
static int last_x = -1;
static int last_y = -1;

static void pixel(int x, int y, u16 color)
{
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        canvas[y * WIDTH + x] = color;
}

static void line(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;
    int err = (dx >= 0 ? dx : -dx) - (dy >= 0 ? dy : -dy);

    while (true)
    {
        pixel(x0, y0, BLACK);
        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;
        if (e2 > -(dy >= 0 ? dy : -dy))
        {
            err -= (dy >= 0 ? dy : -dy);
            x0 += sx;
        }
        if (e2 < (dx >= 0 ? dx : -dx))
        {
            err += (dx >= 0 ? dx : -dx);
            y0 += sy;
        }
    }
}

static void clear_canvas(void)
{
    for (int i = 0; i < WIDTH * HEIGHT; i++)
        canvas[i] = WHITE;

    last_x = -1;
    last_y = -1;
}

static void put_u16(unsigned char *p, uint16_t value)
{
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
}

static void put_u32(unsigned char *p, uint32_t value)
{
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
}

static bool save_bmp(const char *path)
{
    FILE *file = fopen(path, "wb");
    if (!file)
        return false;

    const uint32_t data_size = WIDTH * HEIGHT * 3;
    unsigned char header[54];
    memset(header, 0, sizeof(header));

    header[0] = 'B';
    header[1] = 'M';
    put_u32(&header[2], 54 + data_size);
    put_u32(&header[10], 54);
    put_u32(&header[14], 40);
    put_u32(&header[18], WIDTH);
    put_u32(&header[22], HEIGHT);
    put_u16(&header[26], 1);
    put_u16(&header[28], 24);
    put_u32(&header[34], data_size);

    if (fwrite(header, 1, sizeof(header), file) != sizeof(header))
    {
        fclose(file);
        return false;
    }

    for (int y = HEIGHT - 1; y >= 0; y--)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            u16 p = canvas[y * WIDTH + x];
            unsigned char r = ((p >> 0) & 0x1f) << 3;
            unsigned char g = ((p >> 5) & 0x1f) << 3;
            unsigned char b = ((p >> 10) & 0x1f) << 3;

            fputc(b, file);
            fputc(g, file);
            fputc(r, file);
        }
    }

    fclose(file);
    return true;
}

int main(void)
{
    consoleDemoInit();

    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_3_2D);

    int bg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgShow(bg);
    canvas = (u16 *)bgGetGfxPtr(bg);

    bool sd_ready = fatInitDefault();

    printf("\x1b[1;1HSignature Pad");
    printf("\x1b[3;1HUse stylus to draw");
    printf("\x1b[5;1HA = Save   B = Clear");

    clear_canvas();

    while (1)
    {
        scanKeys();

        if (keysDown() & KEY_B)
            clear_canvas();

        if (keysDown() & KEY_A)
        {
            if (!sd_ready)
                printf("\x1b[7;1HSD card unavailable   ");
            else if (save_bmp("/signature.bmp"))
                printf("\x1b[7;1HSaved signature.bmp   ");
            else
                printf("\x1b[7;1HSave failed           ");
        }

        touchPosition touch;
        touchRead(&touch);

        if (touch.px > 0 && touch.py > 0)
        {
            if (last_x >= 0 && last_y >= 0)
                line(last_x, last_y, touch.px, touch.py);
            else
                pixel(touch.px, touch.py, BLACK);

            last_x = touch.px;
            last_y = touch.py;
        }
        else
        {
            last_x = -1;
            last_y = -1;
        }

        swiWaitForVBlank();
    }
}