/******************************************************************************
      Filename    : OPCODE.C
      Product     : NITPIC
      Description : Creates an opcode for an instruction
      Created     : August 4, 1999 [11:38:37]
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

extern char IgnoreOFtest;
static done_full;

/*----------------------------------------------------------------------
 * Prepares a low-level opcode
 *----------------------------------------------------------------------*/
Opcode(char *oc, int p1, int p2, char *str)
{
	int i;
	
	if (strcmp(oc, "org") == 0) {
		OCaddr = p1;
		return;
	}

	if (OCaddr >= MAXCODELINES) {
		error("Error   %s %d : Output code exceeds limit of %d words\n", INFILE, NR, MAXCODELINES);
		exit(1);
	}

	if (!done_full) {
		for (i = 0; i <= TopCodePage; i++) {
			if (!IgnoreOFtest) {
				if (OCaddr == ((i + 1) * CodePageSize) - 1) {
					error("Error   %s %d : Page %d too full - Use more procedures + 'page' command\n", INFILE, NR, i);
					done_full = 1;
					return;
				}
			}
		}
	}
	else
		return;
	
	if (OC[OCaddr].oc)
		free(OC[OCaddr].oc);
	OC[OCaddr].oc = new(strlen(oc) + 1);
	strcpy(OC[OCaddr].oc, oc);

	if (str && *str) {
		if (OC[OCaddr].str)
			free(OC[OCaddr].str);
		OC[OCaddr].str = new(strlen(str) + 1);
		strcpy(OC[OCaddr].str, str);
	}
	else
		OC[OCaddr].str = NULL;

	OC[OCaddr].p1 = p1 & 0xff;
	OC[OCaddr].p2 = p2;
	OCREF[OCaddr].fileid = CurrentFileID;
	OCREF[OCaddr].line = NR;
	OCREF[OCaddr].tmp_index = tmp_index;

	OCaddr++;

	// test for page crossings and open blocks
	if ((OCaddr % CodePageSize) == 0) {
		if (Level > 0) {
			error("Error   %s %d : Block construct spans two code pages\n", INFILE, NR);
		}
		else if (busy_with_proc >= 0) {
			error("Error   %s %d : Procedure spans two code pages\n", INFILE, NR);
		}
		else if (busy_with_int >= 0) {
			error("Error   %s %d : Interrupt handler spans two code pages\n", INFILE, NR);
		}
		else if (CodeWidth != 12)	// fix 27/2/98
			error("Error   %s %d : Crossing page boundary at address '%X'\n", INFILE, NR, OCaddr);
	}
}
/******************************************************************************
                              End of OPCODE.C
******************************************************************************/
