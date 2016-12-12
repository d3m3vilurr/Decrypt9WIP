#include "fs.h"
#include "draw.h"
#include "hid.h"
#include "platform.h"
#include "gamecart/protocol.h"
#include "gamecart/command_ctr.h"
#include "gamecart/command_ntr.h"
#include "gamecart/command_ak2i.h"
#include "decryptor/aes.h"
#include "decryptor/sha.h"
#include "decryptor/decryptor.h"
#include "decryptor/hashfile.h"
#include "decryptor/keys.h"
#include "decryptor/titlekey.h"
#include "decryptor/nandfat.h"
#include "decryptor/nand.h"
#include "decryptor/game.h"
#include "decryptor/ak2i.h"

u32 DumpAk2iCart(u32 param)
{
    // check if cartridge inserted
    if (REG_CARDCONF2 & 0x1) {
        Debug("Cartridge was not detected");
        return 1;
    }

    Cart_Init();

    u32 cartId = Cart_GetID();
    if (cartId != 0xFC2) {
        Debug("Cartridge is not AK2i");
        return 1;
    }

    u32 hwVer = AK2I_CmdGetHardwareVersion();
    if (hwVer != 0x81 && hwVer != 0x44 ) {
        Debug("Cartridge is not AK2i");
        return 1;
    }

    Debug("Cart: 0x%08X HW: 0x%08X", cartId, hwVer);

    u32 length = hwVer == 0x81 ? 0x1000000 : 0x200000;

    uint8_t* mem = BUFFER_ADDRESS + 0x10000;

    Debug("Reading rom... (length 0x4000)");
    AK2I_CmdReadRom(0, mem, 0x4000);

    Debug("Dump rom...");
    if (FileDumpData("ak2i_rom_0x4000.bin", mem, 0x4000) != 0x4000) {
        Debug("Failed writing ak2i_rom_0x4000.bin");
    }

    AK2I_CmdSetMapTableAddress(AK2I_MTN_NOR_OFFSET, 0);

    // setup flash to 1681
    if (hwVer == 0x81) {
        Debug("Setup flash to 1681");
        AK2I_CmdSetFlash1681_81();
    }

    Debug("Active FAT MAP");
    // have to do this on ak2i before making fatmap, or the first 128k flash data will be screwed up.
    AK2I_CmdActiveFatMap();

    Debug("Unlock flash");
    // this funcion write enable only above 0x40000 on ak2i, write enable all on ak2
    AK2I_CmdUnlockFlash();

    Debug("Unlock ASIC");
    // unlock 0x30000 for save map, see definition of NOR_FAT2_START above
    AK2I_CmdUnlockASIC();

    Debug("Dump flash... (length 0x%08X)", length);

    char *filename = "ak2i_flash.bin";
    DebugFileCreate(filename, true);
    uint8_t *tmp = mem;
    for( u32 i = 0, offset = 0, read = 0; i < length; i += 512 ) {
        ShowProgress(i, length);
        u32 toRead = 512;
        if( toRead > length - i )
            toRead = length - i;
        memset(tmp, 0, toRead);
        AK2I_CmdReadFlash(i, tmp, toRead);
        read += toRead;
        tmp += 512;
        if (toRead < 512 || (read % 0x8000) == 0) {
            DebugFileWrite(mem, read, offset);
            offset += read;
            read = 0;
            tmp = mem;
        }
    }
    FileClose();
    ShowProgress(0, 0);

    Debug("Done");

    return 0;
}

