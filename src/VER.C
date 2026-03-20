/******************************************************************************
      Filename    : VER.C
      Product     : NITPIC
      Description : Prints file headers
      Created     : August 4, 1999 [11:39:50]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#include <stdio.h>

#include "pic_c.h"
#include "version.h"

char verstr[50];
extern FILE *assm;
extern FILE *map;
extern FILE *lst;
extern int LSTline;

print_map_header()
{
	fprintf(map, "********************************************************************\n");
	fprintf(map, "*** Compiled by NITPIC - Version %s.%s.%s - Copyright (c) 2002      ***\n", PR, MAJ, MIN);
	fprintf(map, "********************************************************************\n\n");
}

print_asm_header()
{
	fprintf(assm, ";********************************************************************\n");
	fprintf(assm, ";*** Compiled by NITPIC - Version %s.%s.%s - Copyright (c) 2002      ***\n", PR, MAJ, MIN);
	fprintf(assm, ";*** Intermediate file - Do NOT place under configuration control ***\n");
	fprintf(assm, ";********************************************************************\n\n");
}

print_lst_header()
{
	fprintf(lst, ";********************************************************************\n");
	fprintf(lst, ";*** Compiled by NITPIC - Version %s.%s.%s - Copyright (c) 2002      ***\n", PR, MAJ, MIN);
	fprintf(lst, ";*** Use this file with Microchip (c) MPLAB Emulator/Simulator    ***\n");
	fprintf(lst, ";********************************************************************\n\n");
	LSTline += 6;
}

Hello()
{
	printf("********************************************************************\n");
	printf("*** NITPIC PIC16C5x/xx Compiler - Version %s.%s.%s                  ***\n", PR, MAJ, MIN);
	printf("********************************************************************\n");
}

set_verstr(char *str)
{
	sprintf(str, "NITPIC - Version %s.%s.%s\n", PR, MAJ, MIN);
}
/******************************************************************************
                               End of VER.C
******************************************************************************/