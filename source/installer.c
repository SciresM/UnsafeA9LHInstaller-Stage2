/*
*   installer.c
*/

#include "installer.h"
#include "memory.h"
#include "fs.h"
#include "crypto.h"
#include "screeninit.h"
#include "draw.h"
#include "utils.h"
#include "fatfs/sdmmc/sdmmc.h"

int posY;

u32 console;

void main(void)
{
    initScreens();
    sdmmc_sdcard_init();

    drawString(TITLE, 10, 10, COLOR_TITLE);
    posY = drawString("Installing A9LH...", 10, 40, COLOR_WHITE);
    installer();

    shutdown(0, NULL);
}

static inline void installer(void)
{
    //Setup the key sector de/encryption with the SHA register
    setupKeyslot0x11();

    // If this doesn't work we're kind of fucked anyway, so.
    getSector((u8 *)SECTOR_OFFSET);

    //Generate and encrypt a per-console A9LH key sector
    generateSector((u8 *)SECTOR_OFFSET, 0);

    //Point of no return, install stuff in the safest order
    sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET);
    
    posY = drawString("Replace arm9loaderhax.bin with Luma3DS!", 10, posY + SPACING_Y, COLOR_WHITE);

    shutdown(2, "Full install: success!");
}