/******************************************************************************
      Filename    : GENMAP.C
      Product     : NITPIC
      Description : Generates MAP file
      Created     : August 4, 1999 [11:32:53]
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

genmap(char *name)
{
	char *buf = new(L_FLDLEN);
	char *ptr;
	int i, j, k, addr;
	int procsize, totalsize = 0;
	int TOP = CodePageSize * (TopCodePage+1);

	strcpy(buf, name);
	ptr = strrchr(buf, '.');
	if (ptr)
		strcpy(ptr, ".MAP");
	else
		strcat(buf, ".MAP");
	if (!(map = fopen(buf, "wt"))) {
		error("Error   %s - : Cannot open map file '%s'\n", MainFile, buf);
		free(buf);
		return;
	}

	print_map_header();

	fprintf(map, "Standard variables:\n");
	for (i = 0; i < NextReg; i++) {
		fprintf(map, "\t%s%s=\t%2.2xH\n", 
			Reg[i], (strlen(Reg[i]) < 8) ? "\t\t" : "\t", RegAddr[i]);
	}

	fprintf(map, "\nUser-defined variables:\n");
	for (i = 0; i < NextVar; i++) {
		fprintf(map, "\t%s%s=\t%2.2xH\n", 
			Variable[i], (strlen(Variable[i]) < 8) ? "\t\t" : "\t", VarAddr[i]);
	}
	fprintf(map, "\n*** %d Unused registers ***\n", MaxVariables - NextVar);

	fprintf(map, "\nAutomatic bit assignments:\n");
	for (i = 0; i < NextDef; i++) {
		if (strncmp(Define[i], "_Flags", 6) == 0) {
			fprintf(map, "\t%s%s=\t%s\n", 
				DefName[i], (strlen(DefName[i]) < 8) ? "\t\t" : "\t", Define[i]);
		}
	}

	fprintf(map, "\nLabel and procedure addresses:\n");
	for (i = 0; i < NextLabel; i++) {
		// only print used labels
		if (Label[i].Used) {
			fprintf(map, "\t%s%s=\t%4.4xH\n", 
				Label[i].Name, 
				(strlen(Label[i].Name) < 8) ? "\t\t" : "\t", 
				Label[i].Address);
		}
	}

	fprintf(map, "\n#define's:\n");
	for (i = 0; i < NextDef; i++) {
		fprintf(map, "\t%s%s=\t'%s'\n", 
			DefName[i], 
			(strlen(DefName[i]) < 8) ? "\t\t" : "\t", 
			Define[i]);
	}

	fprintf(map, "\n#macros's:\n");
	for (i = 0; i < NextMac; i++) {
		fprintf(map, "\t%s%s=\t'%s'\n", 
			MacName[i], 
			(strlen(MacName[i]) < 8) ? "\t\t" : "\t", 
			Macro[i]);
	}

	fprintf(map, "\nCode:\tNITPIC Procedure:\tSize:\tDeclared in:\n");
	fprintf(map,   "-----\t-----------------\t-----\t------------\n");
	for (i = 0; i < NextProc; i++) {
		if (ProcCodeEnd[i] > 0) {
			procsize = ProcCodeEnd[i] - ProcCodeStart[i] + 1;
			totalsize += procsize;
			fprintf(map, "%c\t%-20s\t%5d\tLines %3.3d - %3.3d in %s\n", 
				get_proc_code(i), Proc[i], procsize, ProcStart[i], ProcEnd[i], FileList[ProcFile[i]]);
		}
	}
	fprintf(map,   "\t\t\t\t-----\n");
	fprintf(map,   "\t\tNon-procedural: %5d\n", TOP - find_free_space() - totalsize);
	fprintf(map,   "\t\t    Procedures: %5d\n", totalsize);
	fprintf(map,   "\t\t          FREE: %5d\n", find_free_space());

	fprintf(map, "\nMemory usage map:   (See codes above. '#' = Outside proc, '-' = Unused)\n");
	fprintf(map,   "-----------------");

	i = 0;
	while (i < TOP) {
		fprintf(map, "\n%4.4X :", i);
		for (j = 0; j < 4; j++) {
			fprintf(map, " ");
			for (k = 0; k < 16; k++) {
				addr = i + (j * 16) + k;
				if (!OC[addr].oc)
					fprintf(map, "-");
				else
					fprintf(map, "%c", get_proc_code(addr_is_inside_proc(addr)));
			}
		}
		i += 64;
	}
	fprintf(map, "\n");

	fprintf(map, "\nFiles processed:\n");
	fprintf(map,   "----------------\n");
	for (i = 0; i < NextFile; i++)
		fprintf(map, "%s\n", FileList[i]);

	fprintf(map, "\n--- End of MAP file ---\n");
	fclose(map);
	free(buf);
}
/******************************************************************************
                              End of GENMAP.C
******************************************************************************/
