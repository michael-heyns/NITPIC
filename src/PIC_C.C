/******************************************************************************
      Filename    : PIC_C.C
      Product     : NITPIC
      Description : Main startup module
      Created     : August 4, 1999 [11:39:12]
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
#include <conio.h>
#include <io.h>

#include "general.h"
#define SCOPE
#include "pic_c.h"

LOCAL int FirstTime = Yes;
char BusyWithMAC;
int MaxLinesCounter;                        // must never exceed limit
long StartMemory;                           // free memory we start with memory
long LowestFree;                            // lowest free memory

main(int argc, char *argv[])
{
	static time_t t;
	int i, FileIndex = -1;

	// say hello
	Hello();

	// show HELP
	if (argc == 1)
	{
		show_help();
		exit(1);
	}

	// take snapshot of free memory
//                StartMemory = (long)coreleft();
//                LowestFree = StartMemory;

	strcpy(FS, " \t");
	strcpy(INFILE, "");
	t = time(&t);
	strcpy(DATE, ctime(&t));
	busy_with_proc = -1;                                                    // reset
	busy_with_int = -1;                                                     // reset

	strncpy(HelpFile, argv[0], NAMELEN-1);
	strcpy(&HelpFile[strlen(HelpFile)-4], ".HLP");

	strncpy(DefaultInclude, argv[0], NAMELEN-1);
	strcpy(&DefaultInclude[strlen(DefaultInclude)-4], ".MAC");

	strncpy(ConfigFile, argv[0], NAMELEN-1);
	strcpy(&ConfigFile[strlen(ConfigFile)-4], ".INI");

	// find where the name of the file is given on the command line
	for (i = 1; i < argc; i++)
	{
		strupr(argv[i]);
		if (argv[i][0] != '/')
		{
			FileIndex = i;
			break;
		}
	}
	if (FileIndex == -1)
	{
		printf("Error   %s - : Missing filename on command line\n", HelpFile);
		exit(1);
	}

	// Remember main file to compile
	strncpy(MainFile, argv[FileIndex], NAMELEN-1);

	// setup file to capture all errors
	errfile(argv[FileIndex]);               // MUST BE 1ST !!!

	// setup temp file for list file creation later on
	open_tmpfile();

	// wipe files we will re-generate
	wipe_file(argv[FileIndex], ".ASM");
	wipe_file(argv[FileIndex], ".LST");
	wipe_file(argv[FileIndex], ".COD");
	wipe_file(argv[FileIndex], ".HEX");
	wipe_file(argv[FileIndex], ".MAP");

	// setup default fuses
	if (CodeWidth == 14)
		FUSES = 0x1f;
	else
		FUSES = 0x0f;

	INITIALISE(argc, argv, ConfigFile);
	FindPortCount();

	// compile
	if (infile(argv[FileIndex]) == RC_FAIL)
	{
		error("Error   %s - : Cannot read '%s'\n", MainFile, strupr(argv[FileIndex]));
		unlink(TMPFILE);
		exit(1);
	}
	if ((OPTIONS & O_verbatum))
		printf("[ %d lines compiled ]\n", TotalNR);

	if (!Errors)
	{
		if ((OPTIONS & O_verbatum))
			printf("Removing single calls...\n");
		optimise_calls();
	}

	// optimise
	if (!Errors)
	{
		if ((OPTIONS & O_verbatum))
			printf("Optimising...\n");

		// fixup cross-page gotos and calls
		fix_jumps();

		// optimise code page calls
		if (TopCodePage)
			optimise_cpage();

		// optimise bit tests
		optimise_bittests();
	}

	// resolve labels - even if there are errors !!!
	resolve_labels();
	optimise_labels();

	if (!Errors)
	{
		if ((OPTIONS & O_verbatum))
			printf("Assembling...\n");
		gencode();
	}

	// tell of any unused variables
	show_unused_vars();

	// write the MAP file - even if there are errors !!!
	if (OPTIONS & O_domap)
	{
		if (OPTIONS & O_verbatum)
			printf("Generating Map (.MAP) file...\n");
		genmap(argv[FileIndex]);
	}

	// create object code
	if (!Errors)
	{
		if ((OPTIONS & O_verbatum) && !Errors)
			printf("Generating Object (.HEX) file...\n");
		genhex(argv[FileIndex]);
	}

	// generate _asm file
	if (!Errors && OPTIONS & O_doasm)
	{
		if (OPTIONS & O_verbatum)
			printf("Generating Assembler Source (.ASM) file...\n");
		genasm(argv[FileIndex]);
	}

	// generate list file
	if (OPTIONS & O_dolist)
	{
		if (OPTIONS & O_verbatum)
			printf("Generating List (.LST) file...\n");
		genlst(argv[FileIndex]);
	}

	// generate emulator file
	if (!Errors && (OPTIONS & O_docod))
	{
		if (OPTIONS & O_verbatum)
			printf("Generating Emulator (.COD) file...\n");
		gencod(argv[FileIndex]);
		OPTIONS |= O_dolist;
	}

	if (Errors)
	{
		if (err)
			fprintf(err, "<HINT>  %s - : MPLAB users: Double-Click on this line for help\n", HelpFile);
		printf("\n*** %d Errors found\n", Errors);
	}

//                printf("\nDone...  [ %ld kb used / %ld kb spare ]\n\n",
//                                (StartMemory - LowestFree) / 1024, LowestFree / 1024);

	// close all files
	fcloseall();

	// delete unnecessary files
	if (!Errors && !Warnings)
		unlink(ERRFILE);
	unlink(TMPFILE);

	if (OPTIONS & O_verbatum)
		printf("Done...\n");
		
	if (Errors)
		exit(1);
	else
		exit(0);
}

GetFileIndex(char *Name)
{
	int i;

	for (i = 0; i < NextFile; i++)
	{
		if (strcmp(Name, FileList[i]) == 0)
			return(i);
	}
	return(-1);
}

Add2FileList(char *Name)
{
	if ((CurrentFileID = GetFileIndex(Name)) < 0)
	{
		if (NextFile < MAXFILES - 1)
		{
			FileList[NextFile] = new(strlen(Name)+1);
			strcpy(FileList[NextFile], Name);
			CurrentFileID = NextFile;
			NextFile++;
		}
		else
		{
			error("Error   %s %d : File count exceeded: No space for '%s'\n", INFILE, NR, Name);
			CurrentFileID = 0;
			return;
		}
	}
}

infile(char *Name)
{
	/* NO STATIC */ char SavedName[NAMELEN];// ON STACK !!!
	int LastFileID = CurrentFileID;                 // save on stack
	int TopLayer = No;
	int SavedNR;
	FILE *SavedFP;

	strcpy(SavedName, INFILE);                              // save on stack
	SavedFP = fpi;
	SavedNR = NR;
	NR = 0;

	strupr(Name);
	if (!(fpi = fopen(Name, "rt")))
	{
		fpi = SavedFP;
		return(RC_FAIL);
	}
	strcpy(INFILE, Name);
	if (OPTIONS & O_verbatum)
		printf("%s\n", INFILE);

	// if archive specified, add filename
	if (arc)
		fprintf(arc, "copy %s %%1\n", INFILE);

	// remember the name of this file
	Add2FileList(INFILE);

	// take note of this for list file
	if (!no_list && ((OPTIONS & O_dolist) || (OPTIONS & O_doasm)))
	{
		fprintf(tmp, "---[ %s ]---\n", INFILE);
		tmp_index++;
	}

	if (FirstTime == Yes)
	{
		FirstTime = No;
		_Prologue();

		BusyWithMAC = 1;
		infile(DefaultInclude);
		BusyWithMAC = 0;

		// take note of this for list file
		if (!no_list && ((OPTIONS & O_dolist) || (OPTIONS & O_doasm)))
		{
			fprintf(tmp, "---[ Back to %s ]---\n", INFILE);
			tmp_index++;
		}

		MaxLinesCounter = 0;
		TopLayer = Yes;
	}

	while (getline())
		LINE();

	do_lookups();                   // process any outstanding lookups

	// if last statement was a label - add nop to flush label display
	// this is specifically applicable to 'main'
	if (Label[NextLabel-1].Address == OCaddr)
		Opcode("nop", 0, 0, "");

	if (TopLayer == Yes)
	{
		END();
	}

	fclose(fpi);

	CurrentFileID = LastFileID;     // restore previous state
	strcpy(INFILE, SavedName);              // restore previous state
	fpi = SavedFP;
	NR = SavedNR;

	if (TopLayer != Yes && OPTIONS & O_verbatum)
		printf("%s\n", INFILE);

	return(RC_OK);
}

errfile(char *name)
{
	char *ptr;

	strcpy(ERRFILE, name);
	ptr = strrchr(ERRFILE, '.');
	if (ptr)
		strcpy(ptr, ".ERR");
	else
		strcat(ERRFILE, ".ERR");

	if (!(err = fopen(ERRFILE, "wt")))
	{
		printf("Error   %s - : Cannot open '%s'\n", MainFile, ERRFILE);
		return;
	}
}

open_tmpfile()
{
	if (!(tmp = fopen(TMPFILE, "wt")))
	{
		error("Error   %s - : Cannot open '%s'\n", MainFile, TMPFILE);
		exit(1);
	}
}

// this procedure assumes RS to be '\n' always
getline()
{
	char *ptr, *end, *comment;
	int i;
	int LineStart;

	// make sure 1st character is a space (for macros & defines)
	S[0][0] = ' ';

	// read line from file
	if (!fgets(&S[0][1], L_FLDLEN, fpi))
		return(0);
	NR++;
	if (OPTIONS & O_verbatum && (NR % 10) == 0)
		printf("%d\r", NR);

	// note where this line starts
	LineStart = NR;

	TotalNR++;

	ptr = S[0];
	comment = strchr(S[0], ';');
	while ((ptr = strchr(ptr, '\\')) && !isalpha(*(ptr+1)))
	{
		if (comment != NULL && comment < ptr) break;

		*ptr++ = ' ';   // to allow for 'define' replacements
		*ptr++ = '\\';  // put back the nl indicator
		*ptr = '\0';
		if (!fgets(ptr, L_FLDLEN - strlen(S[0]), fpi))
		{
			error("Error   %s %d : Line width of %d exceeded\n", INFILE, NR, L_FLDLEN);
			return(0);
		}
		NR++;
		if (OPTIONS & O_verbatum && (NR % 10) == 0)
			printf("%d\r", NR);

		TotalNR++;
		if ((end = strchr(S[0], ';')))
			*end = '\0';
	}

	// replace tabs with spaces
	for (i = 0; i < strlen(S[0]) - 1; i++)
	{
		if (isspace(S[0][i]))
			S[0][i] = ' ';
	}

	// remove double spaces
	while (replace(S[0], "  ", " "));

	// remove '\n'
	ptr = strchr(S[0], '\n');
	if (ptr)
	        *ptr = '\0';

	if (!no_list && ((OPTIONS & O_dolist) || (OPTIONS & O_doasm)))
	{
		fprintf(tmp, "%5d: %s\n", LineStart, start_of(S[0]));
		tmp_index++;
	}

	// remove comments only now (after inserting into TMP file)
	if ((end = strchr(S[0], ';')))
		*end = '\0';

	return(1);
}

fracture()
{
	char *buf = new(L_FLDLEN);
	char *str, *ptr;
	int i = 1;                                              // start at index 1 of S

	strcpy(buf, S[0]);                              // for 'strtok' to work in

	ptr = buf;
	while ((str = strtok(ptr, FS)))
	{
		if (i >= L_FLDCNT)
		{
			error("Error   %s %d : Maximum of %d fields per line exceeded\n", INFILE, NR, L_FLDCNT);
			break;
		}
		strncpy(S[i++], str, L_FLDLEN);
		ptr = NULL;                     // for subsequent calls to 'strtok'
	}

	NF = i - 1;
	free(buf);
	return(NF);                                     // number of tokens found
}

void rebuild()
{
	int i;

	i = 0;
	while (isspace(S[0][i])) i++;
	S[0][i] = '\0';
	for (i = 1; i <= NF; i++)
	{
		if (strlen(S[0]) + strlen(S[i]) > L_FLDLEN)
		{
			error("Error   %s %d : Line too long\n", INFILE, NR);
			return;
		}
		strcat(S[0], S[i]);
		strcat(S[0], " ");
	}
}

void error(char *Fmt, ...)
{
	char errbuf[150];
	va_list argptr;

	va_start(argptr, Fmt);
	vfprintf(stderr, Fmt, argptr);
	va_end(argptr);

	va_start(argptr, Fmt);
	vsprintf(errbuf, Fmt, argptr);
	va_end(argptr);
	fprintf(tmp, ">>>> %s", errbuf);
	tmp_index++;

	if (err)
	{
		va_start(argptr, Fmt);
		vfprintf(err, Fmt, argptr);
		va_end(argptr);
	}

	Errors++;
}

void warning(char *Fmt, ...)
{
	char errbuf[150];
	va_list argptr;

	va_start(argptr, Fmt);
	vfprintf(stderr, Fmt, argptr);
	va_end(argptr);

	va_start(argptr, Fmt);
	vsprintf(errbuf, Fmt, argptr);
	va_end(argptr);
	fprintf(tmp, ">>>> %s", errbuf);
	tmp_index++;

	if (err)
	{
		va_start(argptr, Fmt);
		vfprintf(err, Fmt, argptr);
		va_end(argptr);
	}

	Warnings++;
}

replace(char line[], char *pattern, char *newstr)
{
	char *buf = new(L_FLDLEN);
	char *l;
	char *start;

	if ((start = strchr(line, *pattern)))
	{
		for (l = start; *l; l++)
		{
			if (strnicmp(l, pattern, strlen(pattern)) == 0)
			{
				strcpy(buf, l+strlen(pattern));
				strcpy(l, newstr);
				if (strlen(l) + strlen(buf) > L_FLDLEN)
				{
					error("Error   %s %d : Line too long\n", INFILE, NR);
					return(0);
				}
				strcat(l, buf);
				free(buf);
				return(1);
			}
		}
	}
	free(buf);
	return(0);
}

islegal(char ch)
{
	if (isalnum(ch) || ch == '_')
		return(1);
	return(0);
}

char *new(unsigned long size)
{
	char *ptr;
//              long fm;

	if (!(ptr = (char *)malloc(size)))
	{
		error("Error   %s %d : Not enough memory\n", INFILE, NR);
		exit(1);
	}

	// if this causes lowest free memory, take note
//                if ((fm = (long)coreleft()) < LowestFree)
//                                LowestFree = fm;

	return(ptr);
}

wipe_file(char *name, char *ext)
{
	char *ptr;
	char *buf = new(L_FLDLEN);

	strncpy(buf, name, L_FLDLEN-1);
	ptr = strrchr(buf, '.');
	if (ptr)
		strcpy(ptr, ext);
	else
		strcat(buf, ext);
	unlink(buf);
	free(buf);
}

/******************************************************************************
                              End of PIC_C.C
******************************************************************************/
