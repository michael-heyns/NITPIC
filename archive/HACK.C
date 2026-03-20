/******************************************************************************
      Filename    : HACK.C
      Product     : NITPIC
      Description : Does HACK checking
      Created     : August 4, 1999 [11:34:28]
      Last Edit   : January 30, 2002
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

// procedure to calculate values:
// uncomment (1) below.
// compile & run
// fill in the value
// compile & test
// comment out (1) again

#define STR_CRC		0x2138			// !!!!!!!!!!!!!!!!!!!!!!!!

char *DemoMarking = "™™™†‘»…”†…”†¡†ƒ≈Õœ†÷≈“”…œŒ†œ∆†Œ…‘–…√†                            ™™™";
#define KEYOFFSET	241

/*----------------------------------------------------------------------
 * Reads the KEY from the KEY file
 *----------------------------------------------------------------------*/
read_key()
{
	FILE *fp;
	char *ptr = Markings;
	char buf[L_FLDLEN];
	char Name[L_FLDLEN];
	int i, j, len;
	unsigned char CRC = 0;

	// assume we are still in DEMO mode
	strcpy(Markings, DemoMarking);

	// try and open KEY file
	if ((fp = fopen(KeyFile, "rt")))
	{
		// read first line
		if (!fgets(buf, L_FLDLEN - 1, fp))
			goto ShowMarkings;		// no match

		// pick out the length
		len = buf[0] & 127;			// drop high bit

		// pick out the name
		j = 2;	// where the name actually starts
		for (i = 0; i < len; i++)
		{
			Name[i] = buf[j];
			j += 4;		// move to next character position
			CRC += Name[i];
			Name[i] = KEYOFFSET - Name[i];
			ReadLicense++;										// TEST 1: must be > 0
		}
		Name[i] = '\0';

		// test CRC
		CRC |= 1;
		if (CRC != buf[1])
			goto ShowMarkings;		// no match

		// OK, all safe, so use the name
		sprintf(Markings, "™™™†ÃÈ„ÂÓÛÂ‰†ÙÔ∫†%-47s†™™™", Name);
		Licensed++;												// TEST 2: must be > 0
	}

ShowMarkings:

	// convert the hidden text to readable format
	while (*ptr)
		*ptr++ &= 127;				// drop the high bit

	// close the file
	fclose(fp);
}

/*----------------------------------------------------------------------
 * Tries to detect HACKS on the above routine
 *----------------------------------------------------------------------*/
hack_check1()
{
	unsigned short CRC = 0;
	char *ptr;

	// calculate CRC for DEMO markings
	ptr = DemoMarking;
	while (*ptr)
		CRC += *ptr++;

	// test for hacking
	if (CRC != STR_CRC)
		Hacked++;												// TEST 3: must be == 0
// (1) DEBUG
// printf("Expected CRC: %x - got %x\n", CRC, STR_CRC);
// exit(0);
}