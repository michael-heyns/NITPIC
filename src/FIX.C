/******************************************************************************
      Filename    : FIX.C
      Product     : NITPIC
      Description : Handles post-compile fixes
      Created     : August 4, 1999 [11:28:15]
      Last Edit   : January 17, 2002
*******************************************************************************
                       Copyright (c) 2002 - M.Heyns
                           All rights reserved.
History:
4/8/99	Removed all "strlen(OC[i].oc) > 0" with "OC[i].oc" because
		even if OC[i].oc may be NULL, it's data will still have length !!!
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
 * Given an address, mark the first label pointing there as used
 *----------------------------------------------------------------------*/
set_label_used(int addr)
{
	int k;

	// attempt to mark the 'user' labels first
	for (k = 0; k < NextLabel; k++)
	{
		if (Label[k].Address == addr && 
		    strncmp(Label[k].Name, "L_", 2) != 0)
		    {
			Label[k].Used = 1;
			return;
		}
	}

	// no user label at this addres found - find NITPIC-generated label
	for (k = 0; k < NextLabel; k++)
	{
		if (Label[k].Address == addr)
		{
			Label[k].Used = 1;
			return;
		}
	}
}

/*----------------------------------------------------------------------
 * Optimise labels
 *----------------------------------------------------------------------*/
optimise_labels()
{
	int i, k;
	int TOP = CodePageSize * (TopCodePage+1);

	// look at all the GOTO and CALL destinations
	for (i = 0; i < TOP; i++)
	{
		if (i > 0 && OC[i].oc && strcmp(OC[i].oc, "goto") == 0)
			set_label_used(OC[i].p1);
		else if (i > 0 && OC[i].oc && strcmp(OC[i].oc, "call") == 0)
			set_label_used(OC[i].p1);
	}

	// wipe unused labels
	for (k = 0; k < NextLabel; k++)
	{
		if (!Label[k].Used)
			Label[k].Address = -1;
	}
}

/*----------------------------------------------------------------------
 * Optimise bit tests 
 *----------------------------------------------------------------------*/
optimise_bittests()
{
	int i, start;
	int TOP = CodePageSize * (TopCodePage+1);

	start = 5;			// where to start looking in OC

	i = start;
	while (i < TOP - 1)
	{
		if (i > 0 && OC[i].oc && 
			strcmp(OC[i].oc, "btfss") == 0 &&
			strcmp(OC[i+1].oc, "goto") == 0 &&
		    GetDestination(i+1) == i + 3)
		    {
			strcpy(OC[i].oc, "btfsc");
			DeleteOpcode(i+1);
		}
		else if (i > 0 && OC[i].oc &&
		    strcmp(OC[i].oc, "btfsc") == 0 &&
		    strcmp(OC[i+1].oc, "goto") == 0 &&
		    GetDestination(i+1) == i + 3)
		    {
			strcpy(OC[i].oc, "btfss");
			DeleteOpcode(i+1);
		}
		i++;
	}
}

/*----------------------------------------------------------------------
 * Returns TRUE if given address has a label attached
 *----------------------------------------------------------------------*/
has_label(int addr)
{
	int i;

	if (addr < 0)
		return(0);
	
	for (i = 0; i < NextLabel; i++)
	{
		if (Label[i].Address == addr)
			return(1);
	}
	return(0);
}

/*----------------------------------------------------------------------
 * Optimise code pages
 *----------------------------------------------------------------------*/
optimise_cpage()
{
	int i, j, thispage, thatpage, start, pbit, pbitend;
	int TOP = CodePageSize * (TopCodePage+1);
	int bp0;		// first bit number
	int preg;
	int has_bcf = -1;	// BCF already found
	int has_bsf = -1;	// BSF already found
	int is_bcf;		// one found
	int where_last = -1;	// where last one was found

	if (CodeWidth == 12)
	{
		preg = find_reg("STATUS");
		bp0 = 5;			// bits 5 and 6
		start = 0;
		if (ResetVector == 0)
			start = 0;
	}
	else
	{
		preg = find_reg("PCLATH");
		bp0 = 3;			// bits 3 and 4
		start = 5;
	}

	if (preg < 0)
	{
		error("Error   %s %d : Invalid code page select register\n", INFILE, NR);
		return;
	}

	if (TopCodePage > 1)
		pbitend = bp0 + 1;
	else
		pbitend = bp0;
		
	for (pbit = bp0; pbit <= pbitend; pbit++)
	{
		i = start;
		while (i < TOP - 1)
		{
			if (OC[i].oc)
			{
				if (i > 0 &&
				    (has_label(i) ||
				     strcmp(OC[i].oc, "goto") == 0 ||
				     strcmp(OC[i].oc, "call") == 0))
				     {
			    		where_last = -1;// reset
			    		has_bcf = 0;	// reset
			    		has_bsf = 0;	// reset
				}
				else if (i > 0 && (i % CodePageSize) != 0 &&
					!has_label(i) && !has_label(where_last) &&
				    (strcmp(OC[i].oc, "bcf") == 0 || strcmp(OC[i].oc, "bsf") == 0) &&
				    OC[i].p1 == preg && OC[i].p2 == pbit)
				    {
				    	if (strcmp(OC[i].oc, "bcf") == 0)
				    		is_bcf = 1;
				    	else
				    		is_bcf = 0;
				    		
				    	if (has_bcf && !is_bcf && where_last >= 0)
				    	{
						DeleteOpcode(i);
						DeleteOpcode(where_last);
				    		i--;			// go one back and test again
				    		where_last = -1;	// reset
				    	}
				    	else if (has_bsf && is_bcf && where_last >= 0)
				    	{
						DeleteOpcode(i);
						DeleteOpcode(where_last);
				    		i--;			// go one back and test again
				    		where_last = -1;	// reset
				    	}
				    	else
				    	{
				    		where_last = i;
				    		has_bcf = 0;	// reset
				    		has_bsf = 0;	// reset
				    		if (is_bcf)
				    			has_bcf = 1;
				    		else
				    			has_bsf = 1;
				    	}
				}
			}
			i++;
		}
	}
}

/*----------------------------------------------------------------------
 * Setup new 'OC[x].oc' string
 *----------------------------------------------------------------------*/
static NewDot_oc(int i, char *name)
{
	if (OC[i].oc)
		free(OC[i].oc);
	OC[i].oc = new(strlen(name) + 1);
	strcpy(OC[i].oc, name);
}

/*----------------------------------------------------------------------
 * Replace single calls with gotos
 *----------------------------------------------------------------------*/
optimise_calls()
{
	char *buf = new(L_FLDLEN);
	int i, j, index, start, SavedOCaddr;
	int TOP = CodePageSize * (TopCodePage+1);

	if (CodeWidth == 12)
		start = 0;
	else
		start = 5;

	i = start;
	while (i < TOP - 1)
	{
		if (OC[i].oc)
		{
			if (strcmp(OC[i].oc, "call") == 0)
			{
				if ((index = find_proc(OC[i].str)) >= 0 && ProcUsageCount[index] == 1)
				{
					if (ProcInline[index])
					{
// no need to tell			warning("Optimising %s - : Single call to '%s' replaced with GOTOs\n", MainFile, Proc[index]);
						NewDot_oc(i, "user_goto");
	
						// create a label to the next address
						strcpy(buf, "_ret_");
						strcat(buf, OC[i].str);
						SavedOCaddr = OCaddr;
						OCaddr = i + 1;	// point to next instruction
						_Label(buf);
						OCaddr = SavedOCaddr;
	
						// replace all 'returns' with gotos
						for (j = ProcCodeStart[index]; j <= ProcCodeEnd[index]; j++)
						{
							if (strcmp(OC[j].oc, "return") == 0 ||
								strcmp(OC[j].oc, "retlw") == 0)
								{
								// setup return values
								if (ProcReturnsValue[index] && strcmp(OC[j].oc, "retlw") == 0)
								{
									InsertOpcode(j, "movlw", OC[j].p1, 0, "", 1);
									j++;	// shift rest up
								}
	
							    	// replace 'retxxx' with 'goto'
							    	NewDot_oc(j, "user_goto");
	
								// give destination to goto
								if (OC[j].str) 
									free(OC[j].str);
								OC[j].str = new(strlen(buf) + 1);
								strcpy(OC[j].str, buf);
							}
						}
					}
					else
						warning("Warning %s - : Procedure '%s' used ONCE only - consider 'inline'\n", MainFile, Proc[index]);
				}
			}
		}
		i++;
	}
	free(buf);
}

/*----------------------------------------------------------------------
 * Fixup gotos and calls
 *----------------------------------------------------------------------*/
fix_jumps()
{
	char buf[20];
	static int lbl_cnt = 0;
	int i, j, thispage, thatpage, start;
	int TOP = CodePageSize * (TopCodePage+1);
	int preg;
    int bp0;        // first bit number
	char selector;	// b0=1 when 1st bit must be CHANGED
                    // b1=value of 1st bit
                    // b2=1 when 2nd bit must be CHANGED
                    // b3=value of 2nd bit


	if (CodeWidth == 12)
	{
		preg = find_reg("STATUS");
		bp0 = 5;			// bits 5 and 6
		start = 0;
		if (ResetVector == 0)
			start = 0;
	}
	else
	{
		preg = find_reg("PCLATH");
		bp0 = 3;			// bits 3 and 4
		start = 5;
	}

	if (preg < 0)
	{
		error("Error   %s %d : Invalid code page select register\n", INFILE, NR);
		return;
	}

	i = start;
	while (i < TOP - 1)
	{
        if (i > 0 && (i % CodePageSize) == 0)
        {
			thispage = (i - 1) / CodePageSize;
			thatpage = i / CodePageSize;
			if (thispage != thatpage)
			{
				selector = CalculatePage(thispage, thatpage);
				if (selector & 1)
				{
					if (selector & 2)
						InsertOpcode(i, "bsf", preg, bp0, "", 1);
					else
						InsertOpcode(i, "bcf", preg, bp0, "", 1);
					// place a label here and fix it, so it's not deleted
					sprintf(buf, "$$$%X", lbl_cnt++);
					Label[NextLabel].Name = new(strlen(buf)+1);
					strcpy(Label[NextLabel].Name, buf);
					Label[NextLabel].Address = i;
					Label[NextLabel].Used = 1;
					NextLabel++;
					i++;
				}
				if (selector & 4)
				{
					if (selector & 8)
						InsertOpcode(i, "bsf", preg, bp0+1, "", 1);
					else
						InsertOpcode(i, "bcf", preg, bp0+1, "", 1);
					// place a label here and fix it, so it's not deleted
					sprintf(buf, "Int%X", lbl_cnt++);
					Label[NextLabel].Name = new(strlen(buf)+1);
					strcpy(Label[NextLabel].Name, buf);
					Label[NextLabel].Address = i;
					Label[NextLabel].Used = 1;
					NextLabel++;
					i++;
				}
			}
		}
	
		// now see what code resides here
		if (OC[i].oc)
		{
			if (strcmp(OC[i].oc, "pcl_addwf") == 0)
			{
				start = i % CodePageSize;
				if (start > 255)
				{
					error("Error   %s - : Move table '%s' to first 255 locations on this page\n", MainFile, OC[i].str);
				}
				NewDot_oc(i, "addwf");
			}
			else if (strcmp(OC[i].oc, "fix_pclath") == 0)
			{
				DeleteOpcode(i);
				InsertOpcode(i, "clrf", 10, 1, "", 1);
				i++;
				start = i;
				if (TopROM >= 0x100)
				{
					if (start & 0x100)
						InsertOpcode(i, "bsf", 10, 0, "", 1);
				}
	
				if (TopROM >= 0x200)
				{
					if (start & 0x200)
						InsertOpcode(i, "bsf", 10, 1, "", 1);
				}
	
				if (TopROM >= 0x400)
				{
					if (start & 0x400)
						InsertOpcode(i, "bsf", 10, 2, "", 1);
				}
	
				if (TopROM >= 0x800)
				{
					if (start & 0x800)
						InsertOpcode(i, "bsf", 10, 3, "", 1);
				}
			}
			else if (strcmp(OC[i].oc, "goto") == 0)
			{
				thispage = i / CodePageSize;
				thatpage = GetDestination(i) / CodePageSize;
				if (thispage != thatpage)
				{
					if (strncmp(OC[i].str, "L_", 2) == 0)
						error("Error   %s - : GOTO Operation crosses page boundary at address %XH\n", MainFile, i);
					else
						error("Error   %s - : Destination label '%s' must be on same page\n", MainFile, OC[i].str);
				}
			}
			else if (strcmp(OC[i].oc, "user_goto") == 0)
			{
				NewDot_oc(i, "goto");
				thispage = i / CodePageSize;
				thatpage = GetDestination(i) / CodePageSize;
				if (thispage != thatpage)
				{
					selector = CalculatePage(thispage, thatpage);
					if (selector & 1)
					{
						if (selector & 2)
							InsertOpcode(i, "bsf", preg, bp0, "", 1);
						else
							InsertOpcode(i, "bcf", preg, bp0, "", 1);
						i++;
					}
					if (selector & 4)
					{
						if (selector & 8)
							InsertOpcode(i, "bsf", preg, bp0+1, "", 1);
						else
							InsertOpcode(i, "bcf", preg, bp0+1, "", 1);
						i++;
					}
				}
			}
			else if (strcmp(OC[i].oc, "call") == 0)
			{
				thispage = i / CodePageSize;
				thatpage = GetDestination(i) / CodePageSize;
				if (thispage != thatpage)
				{
					selector = CalculatePage(thispage, thatpage);
					if (selector & 1)
					{
						if (selector & 2)
							InsertOpcode(i, "bsf", preg, bp0, "", 1);
						else
							InsertOpcode(i, "bcf", preg, bp0, "", 1);
						i++;
					}
					if (selector & 4)
					{
						if (selector & 8)
							InsertOpcode(i, "bsf", preg, bp0+1, "", 1);
						else
							InsertOpcode(i, "bcf", preg, bp0+1, "", 1);
						i++;
					}
					if (selector & 16)
					{
						if (selector & 32)
							InsertOpcode(i+1, "bsf", preg, bp0, "", 0);
						else
							InsertOpcode(i+1, "bcf", preg, bp0, "", 0);
						i++;
					}
					if (selector & 64)
					{
						if (selector & 128)
							InsertOpcode(i+1, "bsf", preg, bp0+1, "", 0);
						else
							InsertOpcode(i+1, "bcf", preg, bp0+1, "", 0);
						i++;
					}
				}
			}
		}
		i++;
	}
}

// returns a bit pattern based on how the pages are jumped as follows
// b0 = 1 when a change has to be made to bit 0 GOING to THATPAGE	1
// b1 = value of bit 0							2
// b2 = 1 when a change has to be made to bit 1 GOING to THATPAGE	4
// b3 = value of bit 1							8
// b4 = 1 when a change has to be made to bit 0 RETURNING to THISPAGE	16
// b5 = value of bit 0							32
// b6 = 1 when a change has to be made to bit 1 RETURNING to THISPAGE	64
// b7 = value of bit 1							128
static CalculatePage(int thispage, int thatpage)
{
	int RC = 0;

	// FROM PAGE 0..........
	if (thispage == 0 && thatpage == 1)
		RC |= (1 + 2 + 16 + 0);
	else if (thispage == 0 && thatpage == 2)
		RC |= (4 + 8 + 64 + 0);
	else if (thispage == 0 && thatpage == 3)
		RC |= (1+4 + 2+8 + 16+64 + 0);

	// FROM PAGE 1..........
	else if (thispage == 1 && thatpage == 0)
		RC |= (1 + 0 + 16 + 32);
	else if (thispage == 1 && thatpage == 2)
		RC |= (1+4 + 8 + 16+64 + 32);
	else if (thispage == 1 && thatpage == 3)
		RC |= (4 + 8 + 64 + 0);

	// FROM PAGE 2..........
	else if (thispage == 2 && thatpage == 0)
		RC |= (4 + 0 + 64 + 128);
	else if (thispage == 2 && thatpage == 1)
		RC |= (1+4 + 2 + 16+64 + 128);
	else if (thispage == 2 && thatpage == 3)
		RC |= (1 + 2 + 16 + 0);

	// FROM PAGE 3..........
	else if (thispage == 3 && thatpage == 0)
		RC |= (1+4 + 0 + 16+64 + 32+128);
	else if (thispage == 3 && thatpage == 1)
		RC |= (4 + 0 + 64 + 128);
	else if (thispage == 3 && thatpage == 2)
		RC |= (1 + 0 + 16 + 32);

	return(RC);
}

static SpaceError(int addr)
{
	static char HasSpaceError = 0;

	if (!HasSpaceError)
	{
		error("Error   %s %d : Program too BIG - Exceeding space on page %d !!!\n", 
			INFILE, NR, addr / CodePageSize);
		warning("*************** (Suggestion: re-organise program)\n");
		HasSpaceError = 1;
	}
}

static InsertOpcode(int addr, char *oc, int p1, int p2, char *str, int before)
{
	int TOP = CodePageSize * (TopCodePage + 1);
	int i, j, stop;
	int savedOCaddr = OCaddr;
	int addrlimit;

	addrlimit = CodePageSize - 5;
	if (addr >= TOP || (addr % CodePageSize) > addrlimit)
	{
		SpaceError(addr);
		return;
	}

	if (!OC[addr].oc)
	{
		OCaddr = addr;
		Opcode(oc, p1, p2, str);
		OCaddr = savedOCaddr;
		OCREF[addr].fileid = OCREF[addr-1].fileid;
		OCREF[addr].line = OCREF[addr-1].line;
		OCREF[addr].tmp_index = OCREF[addr-1].tmp_index;
		return;
	}
	
	// find out where to stop shifting everything UP
	for (i = addr+1; i < TOP; i++)
	{
		if (!OC[i].oc)
			break;
	}
	stop = i;
	
	if (stop >= TOP || (stop % CodePageSize) > addrlimit)
	{
		SpaceError(stop);
		return;
	}

	// shift the table entries UP
	for (j = stop; j > addr; j--)
	{
		OC[j].oc = OC[j-1].oc;
		OC[j].str = OC[j-1].str;
		OC[j].p1 = OC[j-1].p1;
		OC[j].p2 = OC[j-1].p2;
		OCREF[j].fileid = OCREF[j-1].fileid;
		OCREF[j].line = OCREF[j-1].line;
		OCREF[j].tmp_index = OCREF[j-1].tmp_index;
	}

	// insert the new opcode 
	// NewDot_oc does not work here - does not like the 'free' part 5/8/99
    OC[addr].oc = new(strlen(oc) + 1);    // replaced by NewDot_oc
    strcpy(OC[addr].oc, oc);              //         "

	// insert the new text string
	if (strlen(str) > 0)
	{
		OC[addr].str = new(strlen(str) + 1);
		strcpy(OC[addr].str, str);
	}
	else
		OC[addr].str = NULL;

	// insert new parameters and file ID
	OC[addr].p1 = p1 & 0xff;
	OC[addr].p2 = p2;
	if (before)
	{
		OCREF[addr].fileid = OCREF[addr+1].fileid;
		OCREF[addr].line = OCREF[addr+1].line;
		OCREF[addr].tmp_index = OCREF[addr+1].tmp_index;
	}
	else
	{
		OCREF[addr].fileid = OCREF[addr-1].fileid;
		OCREF[addr].line = OCREF[addr-1].line;
		OCREF[addr].tmp_index = OCREF[addr-1].tmp_index;
	}

	// fix up labels as well
	if (before)
	{
		for (i = 0; i < NextLabel; i++)
		{
			if (Label[i].Address > addr && Label[i].Address < stop)
				Label[i].Address++;
		}
	}
	else
	{
		for (i = 0; i < NextLabel; i++)
		{
			if (Label[i].Address >= addr && Label[i].Address < stop)
				Label[i].Address++;
		}
	}

	// fix procedure start and ends
	if (before)
	{
		for (i = 0; i < NextProc; i++)
		{
			if (ProcCodeStart[i] > addr && ProcCodeStart[i] < stop)
				ProcCodeStart[i]++;
			if (ProcCodeEnd[i] > addr && ProcCodeEnd[i] < stop)
				ProcCodeEnd[i]++;
		}
	}
	else
	{
		for (i = 0; i < NextProc; i++)
		{
			if (ProcCodeStart[i] >= addr && ProcCodeStart[i] < stop)
				ProcCodeStart[i]++;
			if (ProcCodeEnd[i] >= addr && ProcCodeEnd[i] < stop)
				ProcCodeEnd[i]++;
		}
	}
}

static DeleteOpcode(int addr)
{
	int TOP = CodePageSize * (TopCodePage + 1);
	int i, j, stop;

	if (!OC[addr].oc)
		return;
	// find out where to stop shifting everything DOWN
	for (i = addr+1; i < TOP; i++)
	{
		if (!OC[i].oc)
			break;
	}
	stop = i;

	// shift the table entries DOWN
	for (j = addr; j < stop; j++)
	{
		OC[j].oc = OC[j+1].oc;
		OC[j].str = OC[j+1].str;
		OC[j].p1 = OC[j+1].p1;
		OC[j].p2 = OC[j+1].p2;
		OCREF[j].fileid = OCREF[j+1].fileid;
		OCREF[j].line = OCREF[j+1].line;
		OCREF[j].tmp_index = OCREF[j+1].tmp_index;
	}

	// wipe the top opcode
	free(OC[stop-1].oc);
	OC[stop-1].oc = NULL;
	free(OC[stop-1].str);
	OC[stop-1].str = NULL;

	// fix up labels as well
	for (i = 0; i < NextLabel; i++)
	{
		if (Label[i].Address > addr && Label[i].Address < stop)
			Label[i].Address--;
	}

	// fix procedure start and ends
	for (i = 0; i < NextProc; i++)
	{
		if (ProcCodeStart[i] > addr && ProcCodeStart[i] < stop)
			ProcCodeStart[i]--;
		if (ProcCodeEnd[i] > addr && ProcCodeEnd[i] < stop)
			ProcCodeEnd[i]--;
	}
}
/******************************************************************************
                               End of FIX.C
******************************************************************************/
