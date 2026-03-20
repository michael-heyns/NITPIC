/******************************************************************************
      Filename    : GENHEX.C
      Product     : NITPIC
      Description : Generates HEX file
      Created     : August 4, 1999 [11:34:28]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <malloc.h>
#include <io.h>

#include "general.h"
#include "pic_c.h"

static int cbuf[16];

/*----------------------------------------------------------------------
 * Generate HEX file
 *----------------------------------------------------------------------*/
genhex(char *name)
{
        char *buf = new(L_FLDLEN);
        char *ptr;
        int i, len, TOP;
        
        strncpy(buf, name, L_FLDLEN-1);
        ptr = strrchr(buf, '.');
        if (ptr)
                strcpy(ptr, ".HEX");
        else
                strcat(buf, ".HEX");
        unlink(buf);

        if (Errors > 0) {
                free(buf);
                return;
        }

        if (!(hex = fopen(buf, "wt"))) {
                error("Error   %s - : Cannot open '%s' for writing\n", MainFile, buf);
                free(buf);
                return;
        }

        // write HEX file records
        i = 0;
        TOP = CodePageSize * (TopCodePage+1);
        while (i < TOP) {
                len = 0;
                while (OC[i+len].oc && len < 8 && (i+len) < TOP) {	// 4/8/99
                        cbuf[len] = OCREF[i+len].code;
                        len++;
                }
                if (len > 0) {
                        write_record(len, i);
                        i += len;
                }
                else
                        i++;
        }

        // write the fuses
        if (do_fuses) {
                if (CodeWidth == 14)
                        FUSES &= 0x3fff;        // mask extras
                else
                        FUSES &= 0xfff;         // play safe
                cbuf[0] = FUSES;
                write_absolute(1, FuseAddress);
        }
        else if (did_fuses)
                warning("Warning %s %d : Fuses requested ('/F') but not specified\n", INFILE, NR);

        // write the product ID
        if (did_id) {
                sprintf(buf, "%4.4X", PID);
                cbuf[0] = (buf[0] >= 'A') ? buf[0] - 'A' + 10 : buf[0] - '0';
                cbuf[1] = (buf[1] >= 'A') ? buf[1] - 'A' + 10 : buf[1] - '0';
                cbuf[2] = (buf[2] >= 'A') ? buf[2] - 'A' + 10 : buf[2] - '0';
                cbuf[3] = (buf[3] >= 'A') ? buf[3] - 'A' + 10 : buf[3] - '0';
                if (CodeWidth == 14) {
                        cbuf[0] |= 0x1f80;
                        cbuf[1] |= 0x1f80;
                        cbuf[2] |= 0x1f80;
                        cbuf[3] |= 0x1f80;
                }
                write_absolute(4, IDaddress);
        }

        // write stuff the user sends directly to HEX file
        for (i = 0; i < NextHex; i++) {
                cbuf[0] = HexData[i];
                write_absolute(1, HexAddress[i]);
        }

        // write the EOF
        fprintf(hex, ":00000001FF\n");
        free(buf);
}

static write_record(int len, int addr)
{
        unsigned char CRC;
        int j, finCRC;

        // only write records that do something         3/8/99
        for (j = 0; j < len; j++) {
            if (cbuf[j] != 0)
                goto WriteRecord;
        }
        return; // nothing to do

        // record has something to do
WriteRecord:
        fprintf(hex, ":%2.2X%4.4X00", len*2, addr*2);
        CRC = len*2;
        CRC += HighOfInt(addr*2);
        CRC += LowOfInt(addr*2);
        for (j = 0; j < len; j++) {
                fprintf(hex, "%2.2X%2.2X", LowOfInt(cbuf[j]) & 0xff, HighOfInt(cbuf[j]) & 0xff);
                CRC += HighOfInt(cbuf[j]);
                CRC += LowOfInt(cbuf[j]);
        }
        finCRC = (0xffff - CRC) + 1;
        finCRC &= 0xff;
        fprintf(hex, "%2.2X\n", finCRC);
}

static write_absolute(int len, int addr)
{
        unsigned char CRC;
        int j, finCRC;

        // only write records that do something         3/8/99
        for (j = 0; j < len; j++) {
            if (cbuf[j] != 0)
                goto WriteRecord;
        }
        return; // nothing to do

        // record has something to do
WriteRecord:
        fprintf(hex, ":%2.2X%4.4X00", len*2, addr);
        CRC = len*2;
        CRC += HighOfInt(addr);
        CRC += LowOfInt(addr);
        for (j = 0; j < len; j++) {
                fprintf(hex, "%2.2X%2.2X", LowOfInt(cbuf[j]) & 0xff, HighOfInt(cbuf[j]) & 0xff);
                CRC += HighOfInt(cbuf[j]);
                CRC += LowOfInt(cbuf[j]);
        }
        finCRC = (0xffff - CRC) + 1;
        finCRC &= 0xff;
        fprintf(hex, "%2.2X\n", finCRC);
}

/******************************************************************************
                              End of GENHEX.C
******************************************************************************/
