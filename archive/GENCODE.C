/******************************************************************************
      Filename    : GENCODE.C
      Product     : NITPIC
      Description : Generates the opcodes
      Created     : August 4, 1999 [11:38:09]
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

#include "general.h"
#include "pic_c.h"

/*----------------------------------------------------------------------
 * Convert opcode strings to actual opcode values
 *----------------------------------------------------------------------*/
gencode()
{
	int i;

	for (i = 0; i < CodePageSize * (TopCodePage+1); i++) {
		if (OC[i].oc) {
			OCREF[i].code = getcode(i);
		}
	}	
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static f_limit(int reg)
{
	if (reg >= DataPageSize) {
		return(reg & (DataPageSize - 1));
	}
	return(reg);
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static n_limit(int i, int value)
{
	if (value > 255) {
		error("Error   %s %d : Numeric constant out of range - Must be 0 - 255 \n", 
			FileList[OCREF[i].fileid], OCREF[i].line);
		return(0);
	}
	return(value);
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static o_limit(int i, int value)
{
	if (value != 0)
		error("Error   %s %d : Operand must be 0 or 1\n",
			FileList[OCREF[i].fileid], OCREF[i].line);
	return(0);
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static b_limit(int i, int value)
{
	if (value > 7) {
		error("Error   %s %d : Invalid bit number - must be 0 - 7\n",
			FileList[OCREF[i].fileid], OCREF[i].line);
		return(0);
	}
	return(value);
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static addr_limit(int i, int addr)
{
	int TOP = CodePageSize * (TopCodePage + 1);
	
	if (addr > TOP) {
		error("Error   %s %d : Address out of range - Must be 0 - %d\n",
			FileList[OCREF[i].fileid], OCREF[i].line, TOP);
		return(0);
	}
	else if (addr >= CodePageSize) {
//		warning("Warning %s %d : Address in upper page - Ensure page is selected\n",
//			FileList[OCREF[i].fileid], OCREF[i].line);
		return(addr & (CodePageSize-1));
	}
	return(addr);
}

/*----------------------------------------------------------------------
 * 
 *----------------------------------------------------------------------*/
static getcode(int i)
{
	int code = 0;
	char DoUsage = 1;	// assume opcode has variable to be counted

	if (CodeWidth == 12) {
		if (strcmp(OC[i].oc, "addwf") == 0) {
			code = 0x1c0;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "andwf") == 0) {
			code = 0x140;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "clrf") == 0) {
			code = 0x060;
			code |= f_limit(OC[i].p1);
		}
		else if (strcmp(OC[i].oc, "clrw") == 0) {
			code = 0x040;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "comf") == 0) {
			code = 0x240;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "decf") == 0) {
			code = 0x0c0;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "decfsz") == 0) {
			code = 0x2c0;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "incf") == 0) {
			code = 0x280;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "incfsz") == 0) {
			code = 0x3c0;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "iorwf") == 0) {
			code = 0x100;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "movf") == 0) {
			code = 0x200;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "movwf") == 0) {
			code = 0x020;
			code |= f_limit(OC[i].p1);
		}
		else if (strcmp(OC[i].oc, "nop") == 0) {
			code = 0x00;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "rlf") == 0) {
			code = 0x340;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "rrf") == 0) {
			code = 0x300;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "subwf") == 0) {
			code = 0x080;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "swapf") == 0) {
			code = 0x380;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "xorwf") == 0) {
			code = 0x180;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x20;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "bcf") == 0) {
			code = 0x400;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 5;
		}
		else if (strcmp(OC[i].oc, "bsf") == 0) {
			code = 0x500;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 5;
		}
		else if (strcmp(OC[i].oc, "btfsc") == 0) {
			code = 0x600;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 5;
		}
		else if (strcmp(OC[i].oc, "btfss") == 0) {
			code = 0x700;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 5;
		}
		else if (strcmp(OC[i].oc, "andlw") == 0) {
			code = 0xe00;
			code |= f_limit(OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "call") == 0) {
			code = 0x900;
			code |= addr_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "clrwdt") == 0) {
			code = 0x004;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "goto") == 0) {
			code = 0xa00;
			code |= addr_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "iorlw") == 0) {
			code = 0xd00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "movlw") == 0) {
			code = 0xc00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "option") == 0) {
			code = 0x002;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "retlw") == 0) {
			code = 0x800;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "sleep") == 0) {
			code = 0x003;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "tris") == 0) {
			code = 0x00;
			code |= f_limit(OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "xorlw") == 0) {
			code = 0xf00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else {
			error("Error   %s %d : Internal Error: Addr %x Invalid opcode '%s'\n", 
				FileList[OCREF[i].fileid], OCREF[i].line, i, OC[i].oc);
			exit(1);
		}
	}
	else if (CodeWidth == 14) {
		if (strcmp(OC[i].oc, "addwf") == 0) {
			code = 0x700;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "andwf") == 0) {
			code = 0x500;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "clrf") == 0) {
			code = 0x180;
			code |= f_limit(OC[i].p1);
		}
		else if (strcmp(OC[i].oc, "clrw") == 0) {
			code = 0x100;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "comf") == 0) {
			code = 0x900;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "decf") == 0) {
			code = 0x300;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "decfsz") == 0) {
			code = 0xb00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "incf") == 0) {
			code = 0xa00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "incfsz") == 0) {
			code = 0xf00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "iorwf") == 0) {
			code = 0x400;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "movf") == 0) {
			code = 0x800;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "movwf") == 0) {
			code = 0x080;
			code |= f_limit(OC[i].p1);
		}
		else if (strcmp(OC[i].oc, "nop") == 0) {
			code = 0x00;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "rlf") == 0) {
			code = 0xd00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "rrf") == 0) {
			code = 0xc00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "subwf") == 0) {
			code = 0x200;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "swapf") == 0) {
			code = 0xe00;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "xorwf") == 0) {
			code = 0x600;
			code |= f_limit(OC[i].p1);
			if (OC[i].p2 == 1)
				code |= 0x80;
			else 
				o_limit(i, OC[i].p2);
		}
		else if (strcmp(OC[i].oc, "bcf") == 0) {
			code = 0x1000;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 7;
		}
		else if (strcmp(OC[i].oc, "bsf") == 0) {
			code = 0x1400;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 7;
		}
		else if (strcmp(OC[i].oc, "btfsc") == 0) {
			code = 0x1800;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 7;
		}
		else if (strcmp(OC[i].oc, "btfss") == 0) {
			code = 0x1c00;
			code |= f_limit(OC[i].p1);
			code |= b_limit(i, OC[i].p2) << 7;
		}
		else if (strcmp(OC[i].oc, "addlw") == 0) {
			code = 0x3e00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "andlw") == 0) {
			code = 0x3900;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "call") == 0) {
			code = 0x2000;
			code |= addr_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "clrwdt") == 0) {
			code = 0x0064;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "goto") == 0) {
			code = 0x2800;
			code |= addr_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "iorlw") == 0) {
			code = 0x3800;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "movlw") == 0) {
			code = 0x3000;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "retfie") == 0) {
			code = 0x0009;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "retlw") == 0) {
			code = 0x3400;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "return") == 0) {
			code = 0x0008;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "sleep") == 0) {
			code = 0x0063;
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "sublw") == 0) {
			code = 0x3c00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else if (strcmp(OC[i].oc, "xorlw") == 0) {
			code = 0x3a00;
			code |= n_limit(i, OC[i].p1);
			DoUsage = 0;	// no variable associated
		}
		else {
			error("Error   %s %d : Internal Error: Addr %x Invalid opcode '%s'\n", 
				FileList[OCREF[i].fileid], OCREF[i].line, i, OC[i].oc);
			exit(1);
		}
	}

	// update usage counter
	if (DoUsage)
		IncUsage(OC[i].p1);
	
	return(code);
}

// given an address, find user-defined variable and update usage counter
IncUsage(int addr)
{
	int i;
	
	// look at VARIABLES
	for (i = 0; i < NextVar; i++) {
		if (VarAddr[i] == addr)
			UsageCount[i]++;
	}
}
/******************************************************************************
                             End of GENCODE.C
******************************************************************************/
