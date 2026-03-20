/******************************************************************************
      Filename    : RESOLVE.C
      Product     : NITPIC
      Description : Resolves labels
      Created     : August 4, 1999 [11:39:39]
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
 * Resolve all labels
 *----------------------------------------------------------------------*/
resolve_labels()
{
	int i;

	// make sure all 'main' and 'INT_handler' are within range
	for (i = 0; i < NextLabel; i++)
	{
		if (strcmp(Label[i].Name, "main") == 0)
		{
			if (Label[i].Address >= ProcRange)
				error("Error   %s - : '%s' must start on page 0 (offset 0 - %d)\n", MainFile, Label[i].Name, ProcRange);
		}
		else if (strcmp(Label[i].Name, "INT_handler") == 0)
		{
			if (Label[i].Address != INTvector)
				error("Error   %s - : '%s' must be the very FIRST procedure in your program\n", MainFile, Label[i].Name, ProcRange);
		}
	}

	// make sure all procedures are within range
	for (i = 0; i < NextProc; i++)
	{
		if (ProcUsageCount[i] > 1 && get_addr(Proc[i]) % CodePageSize >= ProcRange)
			error("Error   %s - : CPU-specific: Page address of proc '%s' greater than %d\n", MainFile, Proc[i], ProcRange);
	}

	// find the label matching the given address
	for (i = 0; i < CodePageSize * (TopCodePage+1); i++)
	{
		if (OC[i].oc)
		{
			if (strcmp(OC[i].oc, "goto") == 0 || 
			    strcmp(OC[i].oc, "call") == 0)
		    {
		    	if (OC[i].str && strlen(OC[i].str) > 0)
			    {
		    		OC[i].p1 = GetDestination(i);
			    	free(OC[i].str);
			    	OC[i].str = NULL;
			    }
			}
		}
	}
}

/*----------------------------------------------------------------------
 * find the address given the label name
 *----------------------------------------------------------------------*/
static get_addr(char *pname)
{
	int i;

	for (i = 0; i < NextLabel; i++)
	{
		if (strcmp(pname, Label[i].Name) == 0)
			return(Label[i].Address);
	}	
	error("Error   %s - : Internal. Unresolved label '%s'\n", MainFile, pname);
	return(-1);
}

/******************************************************************************
                             End of RESOLVE.C
******************************************************************************/
