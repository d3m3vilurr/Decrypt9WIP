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

#define AK2I_81_BOOTROM_LENGTH 0x1000000
#define AK2I_44_BOOTROM_LENGTH 0x200000
#define AK2I_PATCH_LENGTH 0x20000
#define AK2I_PAYLOAD_OFFSET 0x2000
#define AK2I_PAYLOAD_LENGTH 0x1000

unsigned short crc16tab[] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

struct ntrcardhax_info n_ak2i_ntrcardhax_infos[6] = {
    { 17120, 0x80e2b34, 0x80e5e4c, 0x80ed3d0 },
    { 18182, 0x80e1974, 0x80e4c8c, 0x80ec210 },
    { 19218, 0x80e1974, 0x80e4c8c, 0x80ec210 },
    { 20262, 0x80e1974, 0x80e4c8c, 0x80ec210 },
    { 21288, 0x80f9d34, 0x80fcc4c, 0x81041d0 },
    { 22313, 0x80f9d34, 0x80fcc4c, 0x81041d0 },
};

uint16_t calcCrc(u8 *data, uint32_t length)
{
    uint16_t crc = (uint16_t)~0;
    for(unsigned int i = 0; i<length; i++)
    {
        crc = (crc >> 8) ^ crc16tab[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

//	 ldr sp,=0x22140000
//
//	 ;Disable IRQ
//	 mrs r0, cpsr
//	 orr r0, #(1<<7)
//	 msr cpsr_c, r0
//
//	 adr r0, kernelmode
//	 swi 0x7B
//
//kernelmode:
//	 mov r2, #0x22
//	 msr CPSR_c, #0xDF
//	 ldr r3, =0x33333333 ;R/W
//	 mcr p15, 0, r3,c5,c0, 2
//	 mov r2, #0xCC
//	 mcr p15, 0, r3,c5,c0, 3
//	 ldr r0, =0x23F00000
//	 bx r0
u8 loader_bin[0x44] =
{
    0x30, 0xD0, 0x9F, 0xE5, 0x00, 0x00, 0x0F, 0xE1, 0x80, 0x00, 0x80, 0xE3, 0x00, 0xF0, 0x21, 0xE1,
    0x00, 0x00, 0x8F, 0xE2, 0x7B, 0x00, 0x00, 0xEF, 0x22, 0x20, 0xA0, 0xE3, 0xDF, 0xF0, 0x21, 0xE3,
    0x14, 0x30, 0x9F, 0xE5, 0x50, 0x3F, 0x05, 0xEE, 0xCC, 0x20, 0xA0, 0xE3, 0x70, 0x3F, 0x05, 0xEE,
    0x08, 0x00, 0x9F, 0xE5, 0x10, 0xFF, 0x2F, 0xE1, 0x00, 0x00, 0x14, 0x22, 0x33, 0x33, 0x33, 0x33,
    0x00, 0x00, 0xF0, 0x23,
};


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

    u32 length = hwVer == 0x81 ? AK2I_81_BOOTROM_LENGTH : AK2I_44_BOOTROM_LENGTH;

    u8* mem = BUFFER_ADDRESS + 0x10000;

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

    //Debug("Unlock flash");
    //// this funcion write enable only above 0x40000 on ak2i, write enable all on ak2
    //AK2I_CmdUnlockFlash();

    //Debug("Unlock ASIC");
    //// unlock 0x30000 for save map, see definition of NOR_FAT2_START above
    //AK2I_CmdUnlockASIC();

    Debug("Dump flash... (length 0x%08X)", length);

    DebugFileCreate("ak2i_flash.bin", true);
    u8 *tmp = mem;
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
    Debug("Done");

    return 0;
}

int8_t selectDeviceType() {
    DebugColor(COLOR_ASK, "Use arrow keys and <A> to choose device type");

    //u8 is_new = 0;
    u8 is_new = 1;
    while (true) {
        DebugColor(COLOR_SELECT, "\r%s", is_new ? "NEW 3DS" : "OLD 3DS");
        u32 pad_state = InputWait();
        if (pad_state & (BUTTON_UP | BUTTON_DOWN)) {
            // TODO
            //is_new = (is_new + 1) & 1;
            is_new = 1;
        } else if (pad_state & BUTTON_A) {
            DebugColor(COLOR_ASK, "%s", is_new ? "NEW 3DS" : "OLD 3DS");
            return is_new;
        } else if (pad_state & BUTTON_B) {
            DebugColor(COLOR_ASK, "(cancelled by user)");
            return -1;
        }
    }
}

int32_t selectFirmVersion(int8_t device) {
    DebugColor(COLOR_ASK, "Use arrow keys and <A> to choose target firm version");

    const u32 o_ver[6] = {17120, 18182, 19216, 20262, 21288, 22313};
    const u32 n_ver[6] = {17120, 18182, 19218, 20262, 21288, 22313};

    const char* versions[6] = {
        "9.0 - 9.2",
        "9.3 - 9.4",
        "9.5",
        "9.6 - 9.9",
        "10.0 - 10.1",
        "10.2 - 10.3"
    };

    int idx = 0;
    while (true) {
        DebugColor(COLOR_SELECT, "\rv%d (%s)", (device ? n_ver[idx] : o_ver[idx]), versions[idx]);
        u32 pad_state = InputWait();
        if (pad_state & BUTTON_UP) {
            idx -= 1;
            if (idx < 0) {
                idx = 0;
            }
        } else if (pad_state & BUTTON_DOWN) {
            idx += 1;
            if (idx > 5) {
                idx = 5;
            }
        } else if (pad_state & BUTTON_A) {
            DebugColor(COLOR_SELECT, "v%d (%s)", (device ? n_ver[idx] : o_ver[idx]), versions[idx]);
            return device ? n_ver[idx] : o_ver[idx];
        } else if (pad_state & BUTTON_B) {
            DebugColor(COLOR_ASK, "(cancelled by user)");
            return -1;
        }
    }
}

int32_t patchBootrom(u8* bootrom, const struct ntrcardhax_info *info) {
    u8 *payload = bootrom + AK2I_PATCH_LENGTH + AK2I_PAYLOAD_LENGTH;
    memcpy(payload, bootrom + AK2I_PAYLOAD_OFFSET, AK2I_PAYLOAD_LENGTH);

    // see https://github.com/peteratebs/rtfsprofatfilesystem/blob/b0003c4/include/rtfstypes.h#L1468-L1549
    int rtfsCfgAdrDiff = info->rtfs_cfg_addr - info->ntrcard_header_addr;
    // use ~ drno_to_dr_map
    int rtfsCopyLen = 0x144;

    int wrappedAdr = (rtfsCfgAdrDiff) & 0xFFF;

    if ((wrappedAdr >= 0x0) && (wrappedAdr <= 0x10)) {
        Debug("Conflict ntrcard header");
        return -1;
    }
    if ((wrappedAdr >= 0x2A8) && (wrappedAdr <= 0x314)) {
        Debug("Conflict rtfs struct");
        return -1;
    }

    uint32_t rtfs_cfg[0x144] = {0};
    // cfg_NFINODES
    rtfs_cfg[5] = 1;
    // mem_region_pool
    rtfs_cfg[17] = (uint32_t)(info->ntrcard_header_addr + 0x4);
    int i;
    // drno_to_dr_map
    for (i = 0; i < 26; i++)
        rtfs_cfg[55 + i] = (uint32_t)(info->ntrcard_header_addr + 0);

    uint32_t* prtfs_cfg32 = rtfs_cfg;

    for (i = 0; i < rtfsCopyLen; i+=4) {
        wrappedAdr = (rtfsCfgAdrDiff + i) & 0xFFF;
        //printf("addr: %08X data: %08X\n", wrappedAdr, prtfs_cfg32[i/4]);
        if((wrappedAdr >= 0x14) && (wrappedAdr <= 0x60)) {
            //if(i < 0xFC) {
            //    Debug("Not enough buffer");
            //    return -1;
            //}
            break;
        }
        *(uint32_t*)&payload[wrappedAdr] = prtfs_cfg32[i/4];
    }

    *(uint32_t*)&payload[0x2EC] = info->rtfs_handle_addr; //Some handle rtfs uses
    *(uint32_t*)&payload[0x2F0] = 0x41414141; //Bypass FAT corruption error
    *(uint32_t*)&payload[0x31C] = info->ntrcard_header_addr + 0x2A8; //This is the PC we want to jump to (from a BLX)

    memcpy(&payload[0x2A8], loader_bin, 0x44);

    uint16_t crc = calcCrc(payload, 0x15E);
    *(uint16_t*)&payload[0x15E] = crc;

    memcpy(bootrom + AK2I_PAYLOAD_OFFSET, payload, AK2I_PAYLOAD_LENGTH);

    return 0;
}

u32 InjectAk2iCart(u32 param)
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

    u8* buffer = BUFFER_ADDRESS + AK2I_PATCH_LENGTH;
    memset(buffer, 0x41, AK2I_PATCH_LENGTH);

    Debug("Load data");

    if (!DebugFileOpen("ak2i_patch.bin")) {
        return 1;
    }
    if (!DebugFileRead(buffer, AK2I_PATCH_LENGTH, 0)) {
        FileClose();
        return 1;
    };
    FileClose();

    //DebugFileCreate("ak2i_ntrcardhax.bin", true);
    //DebugFileWrite(buffer, AK2I_PATCH_LENGTH, 0);
    //FileClose();
    //return 1;

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

    Debug("Erase flash");
    for (u32 i = 0; i < AK2I_PATCH_LENGTH; i += 64 * 1024) {
        if (hwVer == 0x81) {
            AK2I_CmdEraseFlashBlock_81(i);
        } else {
            AK2I_CmdEraseFlashBlock_44(i);
        }
    }
    //ioAK2EraseFlash( 0, CHIP_ERASE );

    //buffer[0x200C] = 'B';

    Debug("Writing...");

    for( u32 i = 0; i < AK2I_PATCH_LENGTH; i += 512 ) {
        ShowProgress(i, AK2I_PATCH_LENGTH);
        if (hwVer == 0x81) {
            AK2I_CmdWriteFlash_81(i, buffer + i, 512);
        } else {
            AK2I_CmdWriteFlash_44(i, buffer + i, 512);
        }

        if (!AK2I_CmdVerifyFlash(buffer + i, i, 512)) {
            Debug("verify failed at %08X", i);
        }
    }
    ShowProgress(0, 0);

    Debug("Lock flash");
    AK2i_CmdLockFlash();

//    AK2I_CmdSetMapTableAddress(AK2I_MTN_NOR_OFFSET, 0);
//
//    //u32 acek_sign = 0x4B454341; // "ACEK"
//    //u32 ak2k_sign = 0x4B324B41; // "AK2K"
//
//    //ioAK2WriteFlash( NOR_FAT2_START, &ak2k_sign, 4 );
//
//    //end writting
//
//    u8* mem = BUFFER_ADDRESS + 0x10000;
//    memset(mem, 0x41, 0x10000);
//
//    u32 length = hwVer == 0x81 ? AK2I_81_BOOTROM_LENGTH : AK2I_44_BOOTROM_LENGTH;
//    Debug("Dump patched flash... (length 0x%08X)", length);
//
//    DebugFileCreate("ak2i_flash_patched.bin", true);
//    u8 *tmp = mem;
//    for( u32 i = 0, offset = 0, read = 0; i < length; i += 512 ) {
//        ShowProgress(i, length);
//        u32 toRead = 512;
//        if( toRead > length - i )
//            toRead = length - i;
//        memset(tmp, 0, toRead);
//        AK2I_CmdReadFlash(i, tmp, toRead);
//        read += toRead;
//        tmp += 512;
//        if (toRead < 512 || (read % 0x8000) == 0) {
//            DebugFileWrite(mem, read, offset);
//            offset += read;
//            read = 0;
//            tmp = mem;
//        }
//    }
//
//    FileClose();
//    ShowProgress(0, 0);
//
    Debug("Done");

    return 0;
}

u32 PatchAndInjectAk2iCart(u32 param)
{
    u8* buffer = BUFFER_ADDRESS + AK2I_PATCH_LENGTH;
    memset(buffer, 0x41, AK2I_PATCH_LENGTH);

    Debug("Load data");

    if (!DebugFileOpen("ak2i_flash.bin")) {
        return 1;
    }
    if (!DebugFileRead(buffer, AK2I_PATCH_LENGTH, 0)) {
        FileClose();
        return 1;
    };
    FileClose();

    int8_t device = selectDeviceType();
    if (device < 0) {
        return 1;
    }
    int32_t version = selectFirmVersion(device);
    if (version < 0) {
        return 1;
    }

    struct ntrcardhax_info *info;
    // TODO
    if (device == 0) {
        return 1;
    }
    for (int i = 0; i < 6; i++) {
        if (n_ak2i_ntrcardhax_infos[i].version == version) {
            info = &n_ak2i_ntrcardhax_infos[i];
            break;
        }
    }

    patchBootrom(buffer, info);

    DebugFileCreate("ak2i_patch.bin", true);
    DebugFileWrite(buffer, AK2I_PATCH_LENGTH, 0);
    FileClose();

    return InjectAk2iCart(param);
}

u32 AutoAk2iCart(u32 param) {
    u32 ret;
    if ((ret = DumpAk2iCart(param))) {
        return ret;
    }
    return PatchAndInjectAk2iCart(param);
}
