#include <nds.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // 1. Force the 2D engines on
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    // 2. Initialize the text console on the bottom screen (SUB)
    consoleDemoInit();

    // Print our text exactly once
    printf("Hardware palette test!\n\n");
    printf("Hello World!\n");
    printf("If you see this, it worked!\n");

    int frameCount = 0;
    int colorIndex = 0;
    
    // The DS hardware uses 15-bit color (values range from 0 to 31)
    // We can define the raw hardware colors using the RGB15 macro
    u16 colors[] = {
        RGB15(31, 0, 0),   // Red
        RGB15(0, 31, 0),   // Green
        RGB15(0, 0, 31),   // Blue
        RGB15(31, 31, 0),  // Yellow
        RGB15(31, 0, 31),  // Magenta
        RGB15(0, 31, 31),  // Cyan
        RGB15(31, 31, 31)  // White
    };
    int numColors = 7;

    // 3. Main loop 
    while(1) {
        swiWaitForVBlank();
        scanKeys();
        
        frameCount++;
        
        // Every 60 frames (1 second), change the hardware palette
        if (frameCount >= 60) {
            frameCount = 0; 
            
            colorIndex++;
            if (colorIndex >= numColors) colorIndex = 0; 
            
            // THE HARDWARE TRICK: 
            // consoleDemoInit() assigns the default text color to 
            // palette index 255 on the Sub screen. 
            // We just overwrite that exact memory address!
            BG_PALETTE_SUB[255] = colors[colorIndex];
        }

        if(keysDown() & KEY_START) break;
    }

    return 0;
}