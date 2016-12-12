// Copyright 2014 Normmatt
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
//
// modifyed by osilloscopion (2 Jul 2016)
//

#include "command_ntr.h"
#include "command_ak2i.h"
#include "protocol_ntr.h"
#include "card_ntr.h"
#include "delay.h"

void AK2I_SendCommand(const u32 command[2], u32 pageSize, u32 latency, void* buffer)
{
    REG_NTRCARDMCNT = NTRCARD_CR1_ENABLE;

    for( u32 i=0; i<2; ++i )
    {
        REG_NTRCARDCMD[i*4+0] = command[i]>>24;
        REG_NTRCARDCMD[i*4+1] = command[i]>>16;
        REG_NTRCARDCMD[i*4+2] = command[i]>>8;
        REG_NTRCARDCMD[i*4+3] = command[i]>>0;
    }

    pageSize -= pageSize & 3; // align to 4 byte

    u32 pageParam = NTRCARD_PAGESIZE_4K;
    u32 transferLength = 4096;

    // make zero read and 4 byte read a little special for timing optimization(and 512 too)
    switch (pageSize) {
        case 0:
            transferLength = 0;
            pageParam = NTRCARD_PAGESIZE_0;
            break;
        case 4:
            transferLength = 4;
            pageParam = NTRCARD_PAGESIZE_4;
            break;
        case 512:
            transferLength = 512;
            pageParam = NTRCARD_PAGESIZE_512;
            break;
        case 8192:
            transferLength = 8192;
            pageParam = NTRCARD_PAGESIZE_8K;
            break;
        case 16384:
            transferLength = 16384;
            pageParam = NTRCARD_PAGESIZE_16K;
            break;
        default:
            break; //Using 4K pagesize and transfer length by default
    }

    // go
    REG_NTRCARDROMCNT = 0x10000000;
    REG_NTRCARDROMCNT = NTRKEY_PARAM | NTRCARD_ACTIVATE | NTRCARD_nRESET | pageParam | latency;

    u8 * pbuf = (u8 *)buffer;
    u32 * pbuf32 = (u32 * )buffer;
    bool useBuf = ( NULL != pbuf );
    bool useBuf32 = (useBuf && (0 == (3 & ((u32)buffer))));

    u32 count = 0;
    u32 cardCtrl = REG_NTRCARDROMCNT;

    if(useBuf32)
    {
        while( (cardCtrl & NTRCARD_BUSY) && count < pageSize)
        {
            cardCtrl = REG_NTRCARDROMCNT;
            if( cardCtrl & NTRCARD_DATA_READY  ) {
                u32 data = REG_NTRCARDFIFO;
                *pbuf32++ = data;
                count += 4;
            }
        }
    }
    else if(useBuf)
    {
        while( (cardCtrl & NTRCARD_BUSY) && count < pageSize)
        {
            cardCtrl = REG_NTRCARDROMCNT;
            if( cardCtrl & NTRCARD_DATA_READY  ) {
                u32 data = REG_NTRCARDFIFO;
                pbuf[0] = (unsigned char) (data >>  0);
                pbuf[1] = (unsigned char) (data >>  8);
                pbuf[2] = (unsigned char) (data >> 16);
                pbuf[3] = (unsigned char) (data >> 24);
                pbuf += sizeof (unsigned int);
                count += 4;
            }
        }
    }
    else
    {
        while( (cardCtrl & NTRCARD_BUSY) && count < pageSize)
        {
            cardCtrl = REG_NTRCARDROMCNT;
            if( cardCtrl & NTRCARD_DATA_READY  ) {
                u32 data = REG_NTRCARDFIFO;
                (void)data;
                count += 4;
            }
        }
    }

    // if read is not finished, ds will not pull ROM CS to high, we pull it high manually
    if( count != transferLength ) {
        // MUST wait for next data ready,
        // if ds pull ROM CS to high during 4 byte data transfer, something will mess up
        // so we have to wait next data ready
        do { cardCtrl = REG_NTRCARDROMCNT; } while(!(cardCtrl & NTRCARD_DATA_READY));
        // and this tiny delay is necessary
        ioAK2Delay(33);
        // pull ROM CS high
        REG_NTRCARDROMCNT = 0x10000000;
        REG_NTRCARDROMCNT = NTRKEY_PARAM | NTRCARD_ACTIVATE | NTRCARD_nRESET/* | 0 | 0x0000*/;
    }
    // wait rom cs high
    do { cardCtrl = REG_NTRCARDROMCNT; } while( cardCtrl & NTRCARD_BUSY );
    //lastCmd[0] = command[0];lastCmd[1] = command[1];
}

u32 AK2I_CmdGetHardwareVersion(void)
{
    u32 cmd[2] = {0xD1000000, 0x00000000};
    u32 ver = 0;

    AK2I_SendCommand(cmd, 4, 0, &ver);
    return ver & 0xFF;
}

void AK2I_CmdReadRom(u32 address, u8 *buffer, u32 length)
{
    length &= ~(0x03);
    u32 cmd[2] = {0xB7000000 | (address >> 8), (address & 0xff) << 24};
    AK2I_SendCommand(cmd, length, 2, buffer);
}

void AK2I_CmdReadFlash(u32 address, u8 *buffer, u32 length)
{
    length &= ~(0x03);
    u32 cmd[2] = { 0xB7000000 | (address >> 8), (address & 0xff) << 24 | 0x00100000 };
    AK2I_SendCommand(cmd, length, 2, buffer);
}

void AK2I_CmdSetMapTableAddress(u32 tableName, u32 tableInRamAddress)
{
    tableName &= 0x0F;
    u32 cmd[2] = {0xD0000000 | (tableInRamAddress >> 8),
        ((tableInRamAddress & 0xff) << 24) | ((u8)tableName << 16) };

    AK2I_SendCommand(cmd, 0, 0, NULL);
}

void AK2I_CmdSetFlash1681_81(void)
{
    u32 cmd[2] = {0xD8000000 , 0x0000c606};
    AK2I_SendCommand(cmd, 0, 20, NULL);
}

void AK2I_CmdUnlockFlash(void)
{
    u32 cmd[2] = {0xC2AA55AA, 0x55000000};
    AK2I_SendCommand(cmd, 0, 0, NULL);
}

void AK2I_CmdUnlockASIC(void)
{
    u32 cmd[2] = { 0xC2AA5555, 0xAA000000 };
    AK2I_SendCommand(cmd, 4, 0, NULL);
}

void AK2i_CmdLockFlash(void) {
    u32 cmd[2] = { 0xC2AAAA55, 0x55000000 };
    AK2I_SendCommand(cmd, 0, 0, NULL);
}

void AK2I_CmdActiveFatMap(void)
{
    u32 cmd[2] = {0xC255AA55, 0xAA000000};
    AK2I_SendCommand(cmd, 4, 0, NULL);
}

static void waitFlashBusy()
{
    u32 state = 0;
    u32 cmd[2] = {0xC0000000, 0x00000000};
    do {
        //ioAK2Delay( 16 * 10 );
        AK2I_SendCommand(cmd, 4, 4, &state);
        state &= 1;
    } while(state != 0);
}

void AK2I_CmdEraseFlashBlock_44(u32 address)
{
    u32 cmd[2] = {0xD4000000 | (address & 0x001fffff), (u32)(1<<16)};
    AK2I_SendCommand(cmd, 0, 0, NULL);
    waitFlashBusy();
}

void AK2I_CmdEraseFlashBlock_81(u32 address)
{
    u32 cmd[2] = {0xD4000000 | (address & 0x001fffff), (u32)((0x30<<24) | (0x80<<16) | (0<<8) | (0x35))};
    AK2I_SendCommand(cmd, 0, 20, NULL);
    waitFlashBusy();
}

void AK2I_CmdWriteFlashByte_44(u32 address, u8 data)
{
    u32 cmd[2] = {0xD4000000 | (address & 0x001fffff), (u32)((data<<24) | (3<<16))};
    AK2I_SendCommand(cmd, 0, 20, NULL);
    waitFlashBusy();
}

void AK2I_CmdWriteFlash_44(u32 address, const void *data, u32 length)
{
    u8 * pbuffer = (u8 *)data;
    for(u32 i = 0; i < length; ++i)
    {
        AK2I_CmdWriteFlashByte_44(address, *(pbuffer + i));
        address++;
    }
}

void AK2I_CmdWriteFlashByte_81(u32 address, u8 data)
{
    u32 cmd[2] = { 0xD4000000 | (address & 0x001fffff), (u32)((data<<24) | (0xa0<<16) | (0<<8) | (0x63)) };
    AK2I_SendCommand(cmd, 0, 20, NULL);
    waitFlashBusy();
}

void AK2I_CmdWriteFlash_81(u32 address, const void *data, u32 length)
{
    u8 * pbuffer = (u8 *)data;
    for (u32 i = 0; i < length; ++i)
    {
        AK2I_CmdWriteFlashByte_81(address, *(pbuffer + i));
        address++;
    }
}

bool AK2I_CmdVerifyFlash(void *src, u32 dest, u32 length)
{
    u8 verifyBuffer[512];
    u8 * pSrc = (u8 *)src;
    for (u32 i = 0; i < length; i += 512) {
        u32 toRead = 512;
        if (toRead > length - i)
            toRead = length - i;
        AK2I_CmdReadFlash(dest + i, verifyBuffer, toRead);

        for (u32 j = 0; j < toRead; ++j) {
            if(verifyBuffer[j] != *(pSrc + i + j))
                return false;
        }
    }
    return true;
}

