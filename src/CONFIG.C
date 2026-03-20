/******************************************************************************
      Filename    : CONFIG.C
      Product     : NITPIC
      Description : Handles .INI file reading
      Created     : August 4, 1999 [11:37:22]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>

#include "general.h"
#include "pic_c.h"

// .INI file parameters
#define MAXLINES				5000
#define LINELEN					90		// must be >= 80 !!!

char *Name = NULL;

char **Line;
char SearchTitle[80];
int LineCount = 0;
int CurLine;

/*----------------------------------------------------------------------
 * OpenINIfile
 *----------------------------------------------------------------------*/
int OpenINIfile(char *Filename)
{
	char *buf = new(LINELEN);
	char *Ptr;

	if (LineCount) {			// INI file already open
		free(buf);
	    	return(RC_FAIL);
	}

	// allocate space for the line pointers
	Line = (char **)new(sizeof(char *) * MAXLINES);

	Name = Filename;		// may be updated

	if (!(ini = fopen(Filename, "rt"))) {
		free(buf);
		return(RC_FAIL);
	}

	LineCount = 0;
	for (;;) {
		if (!fgets(buf, LINELEN-1, ini)) {
			break;
		}
		buf[LINELEN-1] = '\0';		// stops 'strchr' from falling off
		Ptr = strchr(buf, '\n');
		if (Ptr)
			*Ptr = '\0';
		Ptr = strchr(buf, ';');
		if (Ptr)
			*Ptr = '\0';
		Line[LineCount] = new(strlen(buf) + 1);
		strcpy(Line[LineCount], buf);
		LineCount++;
	}
	fclose(ini);
	free(buf);
	return(RC_OK);
}

/*----------------------------------------------------------------------
 * CloseINIfile
 *----------------------------------------------------------------------*/
int CloseINIfile(void)
{
	int i;

	if (!LineCount)			// no file read yet
	    return(RC_FAIL);

	for (i = 0; i < LineCount; i++)
		free(Line[i]);

	// free the line pointers
	free(Line);

	Name = NULL;			// done with close
    LineCount = 0;
	fclose(ini);
	return(RC_OK);
}

/*----------------------------------------------------------------------
 * GetINIvalue
 *----------------------------------------------------------------------*/
char *GetINIvalue(char *Section, char *Title)
{
	char *Ptr, *ValPtr;
	int i = 0, j;

	if (!LineCount || !ini)
		return(NULL);

	if (strlen(Section) > 0) {
		for (i = 0; i < LineCount; i++) {
			j = 0;
			while (isspace(Line[i][j]))
				j++;
			if (Line[i][j] == '[' && 
			    strnicmp(&Line[i][j+1], Section, strlen(Section)) == 0 && 
			    Line[i][j+strlen(Section)+1] == ']') {
				i++;	// start looking at next line
				    break;
			}
		}
	}

	while (i < LineCount) {	
		j = 0;
		while (isspace(Line[i][j]))
			j++;
		if (Line[i][j] == '[')
			break;
		if ((Ptr = strchr(&Line[i][j], '='))) {
			ValPtr = Ptr + 1;
			while (isspace(*--Ptr));	// ontop of last char
			    if (strnicmp(&Line[i][j], Title, Ptr-&Line[i][j]+1) == 0) {
				while (isspace(*ValPtr))
					ValPtr++;
				CurLine = i;	// where we found it
				strcpy(SearchTitle, Title);
				return(ValPtr);
			}
		}
		i++;
	}
	CurLine = 0;
	return(NULL);
}

/*----------------------------------------------------------------------
 * GetINIvalue
 *----------------------------------------------------------------------*/
char *GetNEXTvalue(void)
{
	char *Ptr, *ValPtr;
	int i, j;

	if (!LineCount || !CurLine || !ini)
		return(NULL);

	// start where we left off
	i = CurLine + 1;

	while (i < LineCount) {	
		j = 0;
		while (isspace(Line[i][j]))
			j++;
		if (Line[i][j] == '[')
			break;
		if ((Ptr = strchr(&Line[i][j], '='))) {
			ValPtr = Ptr + 1;
			while (isspace(*--Ptr));	// ontop of last char
			    if (strnicmp(&Line[i][j], SearchTitle, Ptr-&Line[i][j]+1) == 0) {
				while (isspace(*ValPtr))
					ValPtr++;
				CurLine = i;	// where we found it
				return(ValPtr);
			}
		}
		i++;
	}
	CurLine = 0;
	return(NULL);
}
/******************************************************************************
                              End of CONFIG.C
******************************************************************************/
