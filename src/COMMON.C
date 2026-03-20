/******************************************************************************
      Filename    : COMMON.C
      Product     : NITPIC
      Description : Some common routines
      Created     : August 4, 1999 [11:37:13]
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
 * Given an address of a GOTO or CALL, find the address where it's jumping to
 *----------------------------------------------------------------------*/
GetDestination(int addr)
{
	int i;

	for (i = 0; i < NextLabel; i++) {
		if (strcmp(Label[i].Name, OC[addr].str) == 0)
			return(Label[i].Address);
	}
	error("Error   %s %d : Unresolved label '%s'\n", 
	      FileList[OCREF[addr].fileid], OCREF[addr].line, OC[addr].str);
	return(0);
}

/*----------------------------------------------------------------------
 * Given a code address, find label that belongs here
 *----------------------------------------------------------------------*/
char *GetLabel(int addr)
{
	int k;

	// try and find the non-NITPIC label names first
	for (k = 0; k < NextLabel; k++) {
		if (Label[k].Address == addr && Label[k].Used == 1 &&
		    strncmp(Label[k].Name, "L_", 2) != 0)
			return(Label[k].Name);
	}

	// else, find the label given by NITPIC
	for (k = 0; k < NextLabel; k++) {
		if (Label[k].Address == addr && Label[k].Used == 1)
			return(Label[k].Name);
	}

	// not found
	return(NULL);
}
/*----------------------------------------------------------------------
 * Print specified number of spaces into a file
 *----------------------------------------------------------------------*/
PrintSpaces(FILE *fp, int n)
{
	int i;
	for (i = 0; i < n; i++)
		fprintf(fp, " ");
}

/*----------------------------------------------------------------------
 * Given the name, find a procedure index
 *----------------------------------------------------------------------*/
find_proc(char *name)
{
	int i;
	
	for (i = 0; i < NextProc; i++) {
		if (strcmp(Proc[i], name) == 0)
			return(i);
	}
	return(-1);
}


/*----------------------------------------------------------------------
 * strcat
 *----------------------------------------------------------------------*/
char *StrCat(char *dest, char *source)
/*
 *	Concatenate <source> on the end of <dest>.  The terminator of
 *	<dest> will be overwritten by the first character of <source>.
 *	The termintor from <source> will be copied.  A pointer to
 *	the modified <dest> is returned.
 */
{
	char *p = dest;

	while(*dest)
		++dest;
	while(*dest++ = *source++)
		;
	return(p);
}

/*----------------------------------------------------------------------
 * strstr
 *----------------------------------------------------------------------*/
char *StrChr(char *string, int symbol)
/*
 *	Return a pointer to the first occurance of <symbol> in <string>.
 *	NULL is returned if <symbol> is not found.
 */
{
	do {
		if(*string == symbol)
			return(string);
	} while(*string++);
	return(NULL);
}

/******************************************************************************
                              End of COMMON.C
******************************************************************************/