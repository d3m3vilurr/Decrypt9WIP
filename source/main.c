#include "common.h"
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "menu.h"
#include "platform.h"
#include "i2c.h"
#include "decryptor/keys.h"
#include "decryptor/game.h"
#include "decryptor/ak2i.h"
#include "decryptor/nand.h"
#include "decryptor/nandfat.h"
#include "decryptor/titlekey.h"
#include "decryptor/selftest.h"
#include "decryptor/xorpad.h"
#include "decryptor/transfer.h"

#define SUBMENU_START 8

MenuInfo menu[] =
{
    {
        "XORpad Generator Options", 9,
        {
            { "NCCH Padgen",                  &NcchPadgen,            0 },
            { "SD Padgen (SDinfo.bin)",       &SdPadgen,              0 },
            { "SD Padgen (SysNAND dir)",      &SdPadgenDirect,        0 },
            { "SD Padgen (EmuNAND dir)",      &SdPadgenDirect,        N_EMUNAND },
            { "Any Padgen (anypad.bin)",      &AnyPadgen,             0 },
            { "CTRNAND Padgen",               &CtrNandPadgen,         0 },
            { "CTRNAND Padgen (slot0x4)",     &CtrNandPadgen,         PG_FORCESLOT4 },
            { "TWLNAND Padgen",               &TwlNandPadgen,         0 },
            { "FIRM0FIRM1 Padgen",            &Firm0Firm1Padgen,      0 }
        }
    },
    {
        "Ticket/Titlekey Options", 8,
        {
            { "Titlekey Decrypt (file)",      &CryptTitlekeysFile,    0 },
            { "Titlekey Encrypt (file)",      &CryptTitlekeysFile,    TK_ENCRYPTED },
            { "Titlekey Decrypt (SysNAND)",   &DumpTicketsTitlekeys,  0 },
            { "Titlekey Decrypt (EmuNAND)",   &DumpTicketsTitlekeys,  N_EMUNAND },
            { "Titlekey Dump (SysNAND)",      &DumpTicketsTitlekeys,  TK_ENCRYPTED },
            { "Titlekey Dump (EmuNAND)",      &DumpTicketsTitlekeys,  N_EMUNAND | TK_ENCRYPTED },
            { "Ticket Dump (SysNAND)",        &DumpTicketsTitlekeys,  TK_TICKETS },
            { "Ticket Dump (EmuNAND)",        &DumpTicketsTitlekeys,  N_EMUNAND | TK_TICKETS }
        }
    },
    {
        "SysNAND Options", 9,
        {
            { "SysNAND Backup/Restore...",    NULL,                   SUBMENU_START +  0 },
            { "CTRNAND transfer...",          NULL,                   SUBMENU_START +  2 },
            { "Partition Dump...",            NULL,                   SUBMENU_START +  4 },
            { "Partition Inject...",          NULL,                   SUBMENU_START +  6 },
            { "System File Dump...",          NULL,                   SUBMENU_START +  8 },
            { "System File Inject...",        NULL,                   SUBMENU_START + 10 },
            { "System Save Dump...",          NULL,                   SUBMENU_START + 12 },
            { "System Save Inject...",        NULL,                   SUBMENU_START + 14 },
            { "Miscellaneous...",             NULL,                   SUBMENU_START + 16 }
        }
    },
    {
        "EmuNAND Options", 9,
        {
            { "EmuNAND Backup/Restore...",    NULL,                   SUBMENU_START +  1 },
            { "CTRNAND transfer...",          NULL,                   SUBMENU_START +  3 },
            { "Partition Dump...",            NULL,                   SUBMENU_START +  5 },
            { "Partition Inject...",          NULL,                   SUBMENU_START +  7 },
            { "System File Dump...",          NULL,                   SUBMENU_START +  9 },
            { "System File Inject...",        NULL,                   SUBMENU_START + 11 },
            { "System Save Dump...",          NULL,                   SUBMENU_START + 13 },
            { "System Save Inject...",        NULL,                   SUBMENU_START + 15 },
            { "Miscellaneous...",             NULL,                   SUBMENU_START + 17 }
        }
    },
    {
        "Content Decryptor Options", 5,
        {
            { "NCCH/NCSD File Options...",    NULL,                   SUBMENU_START + 18 },
            { "CIA File Options...",          NULL,                   SUBMENU_START + 19 },
            { "BOSS File Options...",         NULL,                   SUBMENU_START + 20 },
            { "SD File Options...",           NULL,                   SUBMENU_START + 21 },
            { "CIA Builder Options...",       NULL,                   SUBMENU_START + 22 }
        }
    },
    {
        "Gamecart Dumper Options", 6,
        {
            { "Dump Cart (full)",             &DumpGameCart,          0 },
            { "Dump Cart (trim)",             &DumpGameCart,          CD_TRIM },
            { "Dump & Decrypt Cart (full)",   &DumpGameCart,          CD_DECRYPT },
            { "Dump & Decrypt Cart (trim)",   &DumpGameCart,          CD_DECRYPT | CD_TRIM },
            { "Dump Cart to CIA",             &DumpGameCart,          CD_DECRYPT | CD_MAKECIA },
            { "Dump Private Header",          &DumpPrivateHeader,     0 }
        }
    },
    {
        "NDS Flashcart Options", 5,
        {
            { "Auto NTRCARDHAX to AK2i",      &AutoAk2iCart,          0 },
            { "Dump AK2i",                    &DumpAk2iCart,          0 },
            { "Inject AK2i",                  &InjectAk2iCart,        0 },
            { "Inject NTRCARDHAX to AK2i",    &PatchAndInjectAk2iCart,0 },
            { "Restore AK2i bootrom",         &RestoreAk2iCart,       0 }
        }
    },
    {
        "Maintenance Options", 5,
        {
            { "System Info",                  &SystemInfo,            0 },
            { "Create Selftest Reference",    &SelfTest,              ST_REFERENCE },
            { "Run Selftest",                 &SelfTest,              0 },
            { "Build Key Database",           &BuildKeyDb,            KEY_ENCRYPT },
            { "De/Encrypt Key Database",      &CryptKeyDb,            0 }
        }
    },
    // everything below is not contained in the main menu
    {
        "SysNAND Backup/Restore Options", 6, // ID 0
        {
            { "NAND Backup",                  &DumpNand,              0 },
            { "NAND Backup (min size)",       &DumpNand,              NB_MINSIZE },
            { "NAND Restore",                 &RestoreNand,           N_NANDWRITE | N_A9LHWRITE },
            { "NAND Restore (forced)",        &RestoreNand,           N_NANDWRITE | N_A9LHWRITE | NR_NOCHECKS },
            { "NAND Restore (keep a9lh)",     &RestoreNand,           N_NANDWRITE | NR_KEEPA9LH },
            { "Validate NAND Dump",           &ValidateNandDump,      0 }
        }
    },
    {
        "EmuNAND Backup/Restore Options", 5, // ID 1
        {
            { "NAND Backup",                  &DumpNand,              N_EMUNAND },
            { "NAND Backup (min size)",       &DumpNand,              N_EMUNAND | NB_MINSIZE },
            { "NAND Restore",                 &RestoreNand,           N_NANDWRITE | N_EMUNAND | N_FORCEEMU },
            { "NAND Restore (forced)",        &RestoreNand,           N_NANDWRITE | N_EMUNAND | N_FORCEEMU | NR_NOCHECKS },
            { "Validate NAND Dump",           &ValidateNandDump,      0 } // same as the one in SysNAND backup & restore
        }
    },
    {
        "SysNAND Transfer Options", 4, // ID 2
        {
            { "Auto CTRNAND Transfer",        &NandTransfer,          N_NANDWRITE },
            { "Forced CTRNAND Transfer",      &NandTransfer,          N_NANDWRITE | TF_FORCED },
            { "Dump transferable CTRNAND",    &DumpTransferable,      0 },
            { "Autofix CTRNAND",              &AutoFixCtrnand,        N_NANDWRITE }
        }
    },
    {
        "EmuNAND Transfer Options", 4, // ID 3
        {
            { "Auto CTRNAND Transfer",        &NandTransfer,          N_NANDWRITE | N_EMUNAND },
            { "Forced CTRNAND Transfer",      &NandTransfer,          N_NANDWRITE | N_EMUNAND | TF_FORCED },
            { "Dump transferable CTRNAND",    &DumpTransferable,      N_EMUNAND },
            { "Autofix CTRNAND",              &AutoFixCtrnand,        N_NANDWRITE | N_EMUNAND }
        }
    },
    {
        "Partition Dump... (SysNAND)", 8, // ID 4
        {
            { "Dump TWLN Partition",          &DecryptNandPartition,  P_TWLN },
            { "Dump TWLP Partition",          &DecryptNandPartition,  P_TWLP },
            { "Dump AGBSAVE Partition",       &DecryptNandPartition,  P_AGBSAVE },
            { "Dump FIRM0 Partition",         &DecryptNandPartition,  P_FIRM0 },
            { "Dump FIRM1 Partition",         &DecryptNandPartition,  P_FIRM1 },
            { "Dump CTRNAND Partition",       &DecryptNandPartition,  P_CTRNAND },
            { "Dump Sector 0x96",             &DecryptSector0x96,     0 },
            { "Dump NAND Header",             &DumpNandHeader,        0 }
        }
    },
    {
        "Partition Dump...(EmuNAND)", 8, // ID 5
        {
            { "Dump TWLN Partition",          &DecryptNandPartition,  N_EMUNAND | P_TWLN },
            { "Dump TWLP Partition",          &DecryptNandPartition,  N_EMUNAND | P_TWLP },
            { "Dump AGBSAVE Partition",       &DecryptNandPartition,  N_EMUNAND | P_AGBSAVE },
            { "Dump FIRM0 Partition",         &DecryptNandPartition,  N_EMUNAND | P_FIRM0 },
            { "Dump FIRM1 Partition",         &DecryptNandPartition,  N_EMUNAND | P_FIRM1 },
            { "Dump CTRNAND Partition",       &DecryptNandPartition,  N_EMUNAND | P_CTRNAND },
            { "Dump Sector 0x96",             &DecryptSector0x96,     N_EMUNAND },
            { "Dump NAND Header",             &DumpNandHeader,        N_EMUNAND }
        }
    },
    {
        "Partition Inject... (SysNAND)", 8, // ID 6
        {
            { "Inject TWLN Partition",        &InjectNandPartition,   N_NANDWRITE | P_TWLN },
            { "Inject TWLP Partition",        &InjectNandPartition,   N_NANDWRITE | P_TWLP },
            { "Inject AGBSAVE Partition",     &InjectNandPartition,   N_NANDWRITE | P_AGBSAVE },
            { "Inject FIRM0 Partition",       &InjectNandPartition,   N_NANDWRITE | N_A9LHWRITE | P_FIRM0 },
            { "Inject FIRM1 Partition",       &InjectNandPartition,   N_NANDWRITE | N_A9LHWRITE | P_FIRM1 },
            { "Inject CTRNAND Partition",     &InjectNandPartition,   N_NANDWRITE | P_CTRNAND },
            { "Inject Sector 0x96",           &InjectSector0x96,      N_NANDWRITE | N_A9LHWRITE },
            { "Restore NAND Header",          &RestoreNandHeader,     N_NANDWRITE | N_A9LHWRITE }
        }
    },
    {
        "Partition Inject... (EmuNAND)", 8, // ID 7
        {
            { "Inject TWLN Partition",        &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_TWLN },
            { "Inject TWLP Partition",        &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_TWLP },
            { "Inject AGBSAVE Partition",     &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_AGBSAVE },
            { "Inject FIRM0 Partition",       &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_FIRM0 },
            { "Inject FIRM1 Partition",       &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_FIRM1 },
            { "Inject CTRNAND Partition",     &InjectNandPartition,   N_NANDWRITE | N_EMUNAND | P_CTRNAND },
            { "Inject Sector0x96",            &InjectSector0x96,      N_NANDWRITE | N_EMUNAND },
            { "Restore NAND Header",          &RestoreNandHeader,     N_NANDWRITE | N_EMUNAND }
        }
    },
    {
        "System File Dump... (SysNAND)", 7, // ID 8
        {
            { "Dump ticket.db",               &DumpNandFile,          F_TICKET },
            { "Dump title.db",                &DumpNandFile,          F_TITLE },
            { "Dump import.db",               &DumpNandFile,          F_IMPORT },
            { "Dump certs.db",                &DumpNandFile,          F_CERTS },
            { "Dump SecureInfo_A",            &DumpNandFile,          F_SECUREINFO },
            { "Dump LocalFriendCodeSeed_B",   &DumpNandFile,          F_LOCALFRIEND },
            { "Dump movable.sed",             &DumpNandFile,          F_MOVABLE }
        }
    },
    {
        "System File Dump... (EmuNAND)", 7, // ID 9
        {
            { "Dump ticket.db",               &DumpNandFile,          N_EMUNAND | F_TICKET },
            { "Dump title.db",                &DumpNandFile,          N_EMUNAND | F_TITLE },
            { "Dump import.db",               &DumpNandFile,          N_EMUNAND | F_IMPORT },
            { "Dump certs.db",                &DumpNandFile,          N_EMUNAND | F_CERTS },
            { "Dump SecureInfo_A",            &DumpNandFile,          N_EMUNAND | F_SECUREINFO },
            { "Dump LocalFriendCodeSeed_B",   &DumpNandFile,          N_EMUNAND | F_LOCALFRIEND },
            { "Dump movable.sed",             &DumpNandFile,          N_EMUNAND | F_MOVABLE }
        }
    },
    {
        "System File Inject... (SysNAND)", 7, // ID 10
        {
            { "Inject ticket.db",             &InjectNandFile,        N_NANDWRITE | F_TICKET },
            { "Inject title.db",              &InjectNandFile,        N_NANDWRITE | F_TITLE },
            { "Inject import.db",             &InjectNandFile,        N_NANDWRITE | F_IMPORT },
            { "Inject certs.db",              &InjectNandFile,        N_NANDWRITE | F_CERTS },
            { "Inject SecureInfo_A",          &InjectNandFile,        N_NANDWRITE | F_SECUREINFO },
            { "Inject LocalFriendCodeSeed_B", &InjectNandFile,        N_NANDWRITE | F_LOCALFRIEND },
            { "Inject movable.sed",           &InjectNandFile,        N_NANDWRITE | F_MOVABLE }
        }
    },
    {
        "System File Inject... (EmuNAND)", 7, // ID 11
        {
            { "Inject ticket.db",             &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_TICKET },
            { "Inject title.db",              &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_TITLE },
            { "Inject import.db",             &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_IMPORT },
            { "Inject certs.db",              &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_CERTS },
            { "Inject SecureInfo_A",          &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_SECUREINFO },
            { "Inject LocalFriendCodeSeed_B", &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_LOCALFRIEND },
            { "Inject movable.sed",           &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_MOVABLE }
        }
    },
    {
        "System Save Dump... (SysNAND)", 5, // ID 12
        {
            { "Dump seedsave.bin",            &DumpNandFile,          F_SEEDSAVE },
            { "Dump nagsave.bin",             &DumpNandFile,          F_NAGSAVE },
            { "Dump nnidsave.bin",            &DumpNandFile,          F_NNIDSAVE },
            { "Dump friendsave.bin",          &DumpNandFile,          F_FRIENDSAVE },
            { "Dump configsave.bin",          &DumpNandFile,          F_CONFIGSAVE }
        }
    },
    {
        "System Save Dump... (EmuNAND)", 5, // ID 13
        {
            { "Dump seedsave.bin",            &DumpNandFile,          N_EMUNAND | F_SEEDSAVE },
            { "Dump nagsave.bin",             &DumpNandFile,          N_EMUNAND | F_NAGSAVE },
            { "Dump nnidsave.bin",            &DumpNandFile,          N_EMUNAND | F_NNIDSAVE },
            { "Dump friendsave.bin",          &DumpNandFile,          N_EMUNAND | F_FRIENDSAVE },
            { "Dump configsave.bin",          &DumpNandFile,          N_EMUNAND | F_CONFIGSAVE }
        }
    },
    {
        "System Save Inject... (SysNAND)", 5, // ID 14
        {
            { "Inject seedsave.bin",          &InjectNandFile,        N_NANDWRITE | F_SEEDSAVE },
            { "Inject nagsave.bin",           &InjectNandFile,        N_NANDWRITE | F_NAGSAVE },
            { "Inject nnidsave.bin",          &InjectNandFile,        N_NANDWRITE | F_NNIDSAVE },
            { "Inject friendsave.bin",        &InjectNandFile,        N_NANDWRITE | F_FRIENDSAVE },
            { "Inject configsave.bin",        &InjectNandFile,        N_NANDWRITE | F_CONFIGSAVE }
        }
    },
    {
        "System Save Inject... (EmuNAND)", 5, // ID 15
        {
            { "Inject seedsave.bin",          &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_SEEDSAVE },
            { "Inject nagsave.bin",           &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_NAGSAVE },
            { "Inject nnidsave.bin",          &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_NNIDSAVE },
            { "Inject friendsave.bin",        &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_FRIENDSAVE },
            { "Inject configsave.bin",        &InjectNandFile,        N_NANDWRITE | N_EMUNAND | F_CONFIGSAVE }
        }
    },
    {
        "Miscellaneous... (SysNAND)", 8, // ID 16
        {
            { "Health&Safety Dump",           &DumpHealthAndSafety,   0 },
            { "Health&Safety Inject",         &InjectHealthAndSafety, N_NANDWRITE },
            { "GBA VC Save Dump",             &DumpGbaVcSave,         0 },
            { "GBA VC Save Inject",           &InjectGbaVcSave,       N_NANDWRITE },
            { "Update SeedDB",                &UpdateSeedDb,          0 },
            { "Dump Config (for Citra)",      &DumpCitraConfig,       0 },
            { "NCCH FIRMs Dump",              &DumpNcchFirms,         0 },
            { "FIRM ARM9 Decryptor",          &DecryptFirmArm9File,   0 }
        }
    },
    {
        "Miscellaneous... (EmuNAND)", 6, // ID 17
        {            
            { "Health&Safety Dump",           &DumpHealthAndSafety,   N_EMUNAND },
            { "Health&Safety Inject",         &InjectHealthAndSafety, N_NANDWRITE | N_EMUNAND },
            { "Update SeedDB",                &UpdateSeedDb,          N_EMUNAND },
            { "Dump Config (for Citra)",      &DumpCitraConfig,       N_EMUNAND },
            { "NCCH FIRMs Dump",              &DumpNcchFirms,         N_EMUNAND },
            { "FIRM ARM9 Decryptor",          &DecryptFirmArm9File,   0 }
        }
    },
    {
        "NCCH/NCSD File Options", 2, // ID 18
        {
            { "NCCH/NCSD Decryptor",          &CryptGameFiles,        GC_NCCH_PROCESS },
            { "NCCH/NCSD Encryptor",          &CryptGameFiles,        GC_NCCH_PROCESS | GC_NCCH_ENC0x2C }
        }
    },
    {
        "CIA File Options", 4, // ID 19
        {
            { "CIA Decryptor (shallow)",      &CryptGameFiles,        GC_CIA_PROCESS },
            { "CIA Decryptor (deep)",         &CryptGameFiles,        GC_CIA_PROCESS | GC_CIA_DEEP },
            { "CIA Decryptor (CXI only)",     &CryptGameFiles,        GC_CIA_PROCESS | GC_CIA_DEEP | GC_CXI_ONLY },
            { "CIA Encryptor (NCCH)",         &CryptGameFiles,        GC_CIA_PROCESS | GC_NCCH_ENC0x2C }
        }
    },
    {
        "BOSS File Options", 2, // ID 20
        {
            { "BOSS Decryptor",               &CryptGameFiles,        GC_BOSS_PROCESS },
            { "BOSS Encryptor",               &CryptGameFiles,        GC_BOSS_PROCESS | GC_BOSS_ENCRYPT }
        }
    },
    {
        "SD File Options", 5, // ID 21
        {
            { "SD Decryptor/Encryptor",       &CryptSdFiles,          0 },
            { "SD Decryptor (SysNAND dir)",   &DecryptSdFilesDirect,  0 },
            { "SD Decryptor (EmuNAND dir)",   &DecryptSdFilesDirect,  N_EMUNAND },
            { "SD CXI Dumper (SysNAND dir)",  &DecryptSdToCxi,        0 },
            { "SD CXI Dumper (EmuNAND dir)",  &DecryptSdToCxi,        N_EMUNAND }
        }
    },
    {
         "CIA Builder Options", 5, // ID 22
        {
            { "Build CIA from NCCH/NCSD",     &ConvertNcsdNcchToCia,  0 },
            { "CIA Builder (SysNAND/orig.)",  &ConvertSdToCia,        0 },
            { "CIA Builder (EmuNAND/orig.)",  &ConvertSdToCia,        N_EMUNAND },
            { "CIA Builder (SysNAND/decr.)",  &ConvertSdToCia,        GC_CIA_DEEP },
            { "CIA Builder (EmuNAND/decr.)",  &ConvertSdToCia,        GC_CIA_DEEP | N_EMUNAND }
        }
    },
    {
        NULL, 0, { { 0 } } // empty menu to signal end
    }
};


void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


void PowerOff()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while (true);
}


u32 InitializeD9()
{
    u32 errorlevel = 0; // 0 -> none, 1 -> autopause, 2 -> critical
    
    ClearScreenFull(true, true);
    DebugClear();
    #ifndef BUILD_NAME
    DebugColor(COLOR_ACCENT, "-- Decrypt9 --");
    #else
    DebugColor(COLOR_ACCENT, "-- %s --", BUILD_NAME);
    #endif
    
    // a little bit of information about the current menu
    if (sizeof(menu)) {
        u32 n_submenus = 0;
        u32 n_features = 0;
        for (u32 m = 0; menu[m].n_entries; m++) {
            n_submenus = m;
            for (u32 e = 0; e < menu[m].n_entries; e++)
                n_features += (menu[m].entries[e].function) ? 1 : 0;
        }
        Debug("Counting %u submenus and %u features", n_submenus, n_features);
    }
    
    Debug("Initializing, hold L+R to pause");
    Debug("");    
    
    if (InitFS()) {
        Debug("Initializing SD card... success");
        FileGetData("d9logo.bin", BOT_SCREEN, 320 * 240 * 3, 0);
        Debug("Build: %s", BUILD_NAME);
        Debug("Work directory: %s", GetWorkDir());
        Debug("Game directory: %s", GetGameDir());
        SetupSector0x96Key0x11(); // Sector0x96 key - no effect on error level
        if (SetupTwlKey0x03() != 0) // TWL KeyX / KeyY
            errorlevel = 2;
        if ((GetUnitPlatform() == PLATFORM_N3DS) && (SetupCtrNandKeyY0x05() != 0))
            errorlevel = 2; // N3DS CTRNAND KeyY
        if (LoadKeyFromFile(0x25, 'X', NULL)) // NCCH 7x KeyX
            errorlevel = (errorlevel < 1) ? 1 : errorlevel;
        if (LoadKeyFromFile(0x18, 'X', NULL)) // NCCH Secure3 KeyX
            errorlevel = (errorlevel < 1) ? 1 : errorlevel;
        if (LoadKeyFromFile(0x1B, 'X', NULL)) // NCCH Secure4 KeyX
            errorlevel = (errorlevel < 1) ? 1 : errorlevel;
        if (SetupAgbCmacKeyY0x24()) // AGBSAVE CMAC KeyY
            errorlevel = (errorlevel < 1) ? 1 : errorlevel;
        Debug("Finalizing Initialization...");
        RemainingStorageSpace();
    } else {
        Debug("Initializing SD card... failed");
            errorlevel = 2;
    }
    Debug("");
    Debug("Initialization: %s", (errorlevel == 0) ? "success!" : (errorlevel == 1) ? "partially failed" : "failed!");
    
    if (CheckButton(BUTTON_L1|BUTTON_R1) || (errorlevel > 1)) {
        DebugColor(COLOR_ASK, "(A to %s)", (errorlevel > 1) ? "exit" : "continue");
        while (!(InputWait() & BUTTON_A));
    }
    
    return errorlevel;
}


int main()
{
    u32 menu_exit = MENU_EXIT_REBOOT;
    
    if (InitializeD9() <= 1) {
        menu_exit = ProcessMenu(menu, SUBMENU_START);
    }
    DeinitFS();
    
    ClearScreenFull(true, true);
    (menu_exit == MENU_EXIT_REBOOT) ? Reboot() : PowerOff();
    return 0;
}
