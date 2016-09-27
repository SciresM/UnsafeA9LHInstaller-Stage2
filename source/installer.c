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

static const u8 sectorHash[0x20] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
};

static const u8 firm1Hash[0x20] = {
    0xD2, 0x53, 0xC1, 0xCC, 0x0A, 0x5F, 0xFA, 0xC6, 0xB3, 0x83, 0xDA, 0xC1, 0x82, 0x7C, 0xFB, 0x3B,
    0x2D, 0x3D, 0x56, 0x6C, 0x6A, 0x1A, 0x8E, 0x52, 0x54, 0xE3, 0x89, 0xC2, 0x95, 0x06, 0x23, 0xE5
};

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
    if(!mountSD())
        shutdown(1, "You bricked: failed to mount the SD card");

    const char *path;

    //Setup the key sector de/encryption with the SHA register
    setupKeyslot0x11();

    //Calculate the CTR for the 3DS partitions
    getNandCTR();

    //Get NAND FIRM0 and test that the CTR is correct
    readFirm0((u8 *)FIRM0_OFFSET, FIRM0_SIZE);
    if(memcmp((void *)FIRM0_OFFSET, "FIRM", 4) != 0)
        shutdown(1, "You bricked: failed to setup FIRM encryption");

    // If this doesn't work we're kind of fucked anyway, so.
    getSector((u8 *)SECTOR_OFFSET);

    //Generate and encrypt a per-console A9LH key sector
    generateSector((u8 *)SECTOR_OFFSET, 0);

    //Read FIRM1
    path = "a9lh/firm1.bin";
    if(fileRead((void *)FIRM1_OFFSET, path) != FIRM1_SIZE)
        shutdown(1, "You bricked: firm1.bin doesn't exist or has a wrong size");

    if(!verifyHash((void *)FIRM1_OFFSET, FIRM1_SIZE, firm1Hash))
        shutdown(1, "You bricked: firm1.bin is invalid or corrupted");

    posY = drawString("Installing...", 10, posY + SPACING_Y, COLOR_WHITE);

    //Point of no return, install stuff in the safest order
    writeFirm((u8 *)FIRM1_OFFSET, 1, FIRM1_SIZE);
    sdmmc_nand_writesectors(0x96, 1, (vu8 *)SECTOR_OFFSET);

    shutdown(2, "Full install: success!");
}