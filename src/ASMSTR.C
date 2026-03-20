/******************************************************************************
      Filename    : ASMSTR.C
      Product     : NITPIC
      Description : Generates assembler string for ASM and LST files
      Created     : August 4, 1999 [11:36:28]
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
#include <time.h>
#include <malloc.h>

#include "general.h"
#include "pic_c.h"

/*----------------------------------------------------------------------
 * Generate ASM string for ASM and LST files
 *----------------------------------------------------------------------*/
gen_asm_str(FILE *fp, int i)
{
        char *buf = new(L_FLDLEN);
		int k;

        strncpy(buf, GetVarName(OC[i].p1), L_FLDLEN-1);
        
        if (strcmp(OC[i].oc, "bcf") == 0 || 
            strcmp(OC[i].oc, "bsf") == 0 ||
            strcmp(OC[i].oc, "btfsc") == 0 ||
			strcmp(OC[i].oc, "btfss") == 0) {

                // only MPASM-formats will comply with this
                if (strncmp(buf, "_Flags", 6) == 0 && (k = FindBitDefined(buf, OC[i].p2)) >= 0) {
                        fprintf(fp, "%-7s %s", OC[i].oc, DefName[k]);
                }
                else {
                        fprintf(fp, "%-7s %s, %d", OC[i].oc, buf, OC[i].p2);
                }
        }
        else if (strcmp(OC[i].oc, "addwf") == 0 || 
                 strcmp(OC[i].oc, "andwf") == 0 ||
				 strcmp(OC[i].oc, "comf") == 0 ||
                 strcmp(OC[i].oc, "decf") == 0 ||
                 strcmp(OC[i].oc, "decfsz") == 0 ||
                 strcmp(OC[i].oc, "incf") == 0 ||
                 strcmp(OC[i].oc, "incfsz") == 0 ||
                 strcmp(OC[i].oc, "iorwf") == 0 ||
                 strcmp(OC[i].oc, "movf") == 0 ||
                 strcmp(OC[i].oc, "rlf") == 0 ||
                 strcmp(OC[i].oc, "rrf") == 0 ||
                 strcmp(OC[i].oc, "subwf") == 0 ||
                 strcmp(OC[i].oc, "swapf") == 0 ||
                 strcmp(OC[i].oc, "xorwf") == 0) {

                fprintf(fp, "%-7s %s, %d", OC[i].oc, buf, OC[i].p2);
        }
        else if (strcmp(OC[i].oc, "andlw") == 0 || 
                 strcmp(OC[i].oc, "iorlw") == 0 ||
                 strcmp(OC[i].oc, "movlw") == 0 ||
                 strcmp(OC[i].oc, "retlw") == 0 ||
                 strcmp(OC[i].oc, "addlw") == 0 ||
                 strcmp(OC[i].oc, "sublw") == 0 ||
                 strcmp(OC[i].oc, "xorlw") == 0) {

                fprintf(fp, "%-7s %d", OC[i].oc, OC[i].p1);
		}
        else if (strcmp(OC[i].oc, "clrf") == 0 || 
                 strcmp(OC[i].oc, "tris") == 0 ||
                 strcmp(OC[i].oc, "movwf") == 0) {

                fprintf(fp, "%-7s %s", OC[i].oc, buf);
        }
        else if (CodeWidth == 12 && strcmp(OC[i].oc, "tris") == 0) {
                fprintf(fp, "%-7s %s", OC[i].oc, buf);
        }
        else if (strcmp(OC[i].oc, "clrw") == 0 || 
                 strcmp(OC[i].oc, "clrwdt") == 0 ||
				 strcmp(OC[i].oc, "option") == 0 ||
                 strcmp(OC[i].oc, "sleep") == 0 ||
                 strcmp(OC[i].oc, "return") == 0 ||
                 strcmp(OC[i].oc, "retfie") == 0 ||
                 strcmp(OC[i].oc, "nop") == 0) {

                fprintf(fp, "%-7s\n", OC[i].oc);
                free(buf);
                return;
        }
        else if (strcmp(OC[i].oc, "call") == 0 || 
                 strcmp(OC[i].oc, "goto") == 0) {
				fprintf(fp, "%-7s %s", OC[i].oc, GetLabel(OC[i].p1));
        }
        else {
                fprintf(fp, "[NITPIC Opcode '%s']\n", OC[i].oc);
                free(buf);
                return;
        }

        fprintf(fp, "\n");
        free(buf);
        return;
}

// given an address, find a variable - either from the CPU registers 
// or user variables
static char *GetVarName(int addr)
{
        int i;
        
        // look at CPU registers
        for (i = 0; i < NextReg; i++) {
                if (RegAddr[i] == addr)
                        return(Reg[i]);
        }

        // look at VARIABLES
        for (i = 0; i < NextVar; i++) {
                if (VarAddr[i] == addr)
                        return(Variable[i]);
        }
        return("[Unknown]");
}

/*----------------------------------------------------------------------
 * Given register name and bit, find '#define' for it
 *----------------------------------------------------------------------*/
static FindBitDefined(char *rname, int bit)
{
		int i;
		char buf[20];

		sprintf(buf, "%s:%d", rname, bit);
		for (i = 0; i < NextDef; i++) {
				if (strcmp(Define[i], buf) == 0)
						return(i);      // yes - got it
		}
		return(-1);                     // not found
}

/******************************************************************************
                              End of ASMSTR.C
******************************************************************************/
