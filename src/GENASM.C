/******************************************************************************
      Filename    : GENASM.C
      Product     : NITPIC
      Description : Generate ASM file
      Created     : August 4, 1999 [11:35:12]
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
#include <time.h>
#include <malloc.h>
#include <io.h>

#include "general.h"
#include "pic_c.h"

genasm(char *name)
{
        char *lbl;
        char *buf = new(L_FLDLEN);
        int i, line;
        int TOP = CodePageSize * (TopCodePage + 1);
        char *ptr;
        
        // Open .ASM file........................
        strncpy(buf, name, L_FLDLEN-1);
        ptr = strrchr(buf, '.');
        if (ptr)
                strcpy(ptr, ".ASM");
        else
                strcat(buf, ".ASM");
        unlink(buf);

        if (Errors > 0) {
                free(buf);
                return;
        }

        if (!(assm = fopen(buf, "wt"))) {
                warning("Warning %s - : Cannot open '%s'\n", MainFile, buf);
                free(buf);
                return;
        }

        print_asm_header();

        // write processor detail
        fprintf(assm, "\t\tlist\t\tw=1\n");
        fprintf(assm, "\t\tprocessor\t%s\n", CPU);
        fprintf(assm, "\t\tradix\t\tdec\n");

        // write symbols
        fprintf(assm, "\n\t\t;*** Processor registers ***\n");
        for (i = 0; i < NextReg; i++)
                fprintf(assm, "\t\t#define %-16s\t%d\t;%XH\n", Reg[i], RegAddr[i], RegAddr[i]); // removed % DataPageSize

        // write the VARIABLES
        fprintf(assm, "\n\t\t;*** User-defined registers ***\t\t\t[Usage count]\n");
        for (i = 0; i < NextVar; i++) {
                if (strchr(Variable[i], '[')) {
                        sprintf(buf, "\t\t%-21s equ \t%d\t;", Variable[i], VarAddr[i], VarAddr[i]); // % DataPageSize);
                        replace(buf, "[", "_");
                        replace(buf, "]", " ");
                        fprintf(assm, "%s%XH = %s\t%d\n", 
                                buf, VarAddr[i], Variable[i], UsageCount[i]);
                }
                else
                        fprintf(assm, "\t\t#define %-16s\t%d\t;%XH\t%d\n", 
                                Variable[i], VarAddr[i], VarAddr[i], UsageCount[i]); // % DataPageSize);
        }

        // write automatic bit assignments
        fprintf(assm, "\n\t\t;*** Automatic bit assignments ***\n");
        for (i = 0; i < NextDef; i++) {
                if (strncmp(Define[i], "_Flags", 6) == 0) {
                        strcpy(buf, Define[i]);
                        if ((ptr = strrchr(buf, ':')))
                                *ptr = ',';
                        fprintf(assm, "\t\t#define %-20s\t%s\n", DefName[i], buf);
                }
        }

        // start the code
        fprintf(assm, "\n\t\t;*** Assembler source ***\n");
        fprintf(assm, "\t\torg\t0\n");

        // Open .TMP file containing the code........................
        if (tmp) {
                fclose(tmp);
                tmp = NULL;
        }
        if (!(inpf = fopen(TMPFILE, "rt"))) {
                warning("Warning %s - : Cannot open '%s' (to create ASM file)\n", MainFile, TMPFILE);
                free(buf);
                return;
        }

        // write code to .ASM file
        line = 1;
        while (fgets(S[0], L_FLDLEN, inpf)) {
                fprintf(assm, ";%s", S[0]);
                for (i = 0; i < TOP; i++) {
                        if (OC[i].oc && OCREF[i].tmp_index == line) {
                                if (i > 0 && !OC[i-1].oc)
                                        fprintf(assm, "\t\torg\t%d\t\t\t;%XH\n", i, i);
                                lbl = GetLabel(i);
                                if (lbl)
                                        fprintf(assm, "%s\n\t\t", lbl);
                                else
                                        fprintf(assm, "\t\t");

                                replace(buf, "[", "_");
                                replace(buf, "]", "");
                                
                                gen_asm_str(assm, i);
                        }
                }
                line++;
        }

        // terminate file
        fprintf(assm, "\n\t\tend\n");
        fprintf(assm, "\n\t\t;*** [End of file] ***\n");
        free(buf);
}

/******************************************************************************
                              End of GENASM.C
******************************************************************************/
