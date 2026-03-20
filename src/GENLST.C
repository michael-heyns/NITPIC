/******************************************************************************
      Filename    : GENLST.C
      Product     : NITPIC
      Description : Generates LST file
      Created     : August 4, 1999 [11:34:49]
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

/*----------------------------------------------------------------------
 * Generate LIST file
 *----------------------------------------------------------------------*/
genlst(char *name)
{
        char *buf = new(L_FLDLEN);
        char *lbl;
        int i, j, line, hasasm;
        int TOP = CodePageSize * (TopCodePage + 1);
        char *ptr;
        
        // Open .LST file........................
        strncpy(buf, name, L_FLDLEN-1);
        ptr = strrchr(buf, '.');
        if (ptr)
                strcpy(ptr, ".LST");
        else
                strcat(buf, ".LST");
        unlink(buf);

        if (!(lst = fopen(buf, "wt"))) {
                warning("Warning %s - : Cannot open '%s'\n", MainFile, buf);
                return;
        }

        // Open .TMP file containing the code........................
        if (tmp) {
                fclose(tmp);
                tmp = NULL;
        }
        if (!(inpf = fopen(TMPFILE, "rt"))) {
                warning("Warning %s - : Cannot open '%s' (to create LST file)\n", MainFile, TMPFILE);
                free(buf);
                return;
        }

        // write header
        print_lst_header();

        // write code to .LST file
        line = 1;
        while (fgets(S[0], L_FLDLEN, inpf)) {
                fprintf(lst, "%s", S[0]);
                LSTline++;
                hasasm = 0;
                for (i = 0; i < TOP; i++) {
                        if (OC[i].oc && OC[i].oc && OCREF[i].tmp_index == line) {	// 4/8/99
                                if (OCREF[i].lst_index == 0)
                                        OCREF[i].lst_index = LSTline;

                                lbl = GetLabel(i);
                                if (lbl) {
                                        fprintf(lst, "\t\t\t   %s\n", lbl);
                                        LSTline++;
                                }

                                fprintf(lst, "\t\t%4.4X- %4.4X\t", i, OCREF[i].code);
                                gen_asm_str(lst, i);
                                hasasm = 1;
                        }
                }
                // we DID use this line
                if (hasasm) {
                        fprintf(lst, "\n");
                        LSTline++;
                }
                line++;
        }

        // write symbols
        fprintf(lst, "\n    *** Processor registers ***\n");
        for (i = 0; i < NextReg; i++) {
                fprintf(lst, "%10s%4.4X  -  %s\n", " ", RegAddr[i], Reg[i]);
        }

        // look at VARIABLES
        fprintf(lst, "\n    *** User-defined registers ***\n");
        fprintf(lst, "\t\t   [Used]\t[Name]\n");
        for (i = 0; i < NextVar; i++)
                fprintf(lst, "%10s%4.4X  -  %d\t\t%s\n", " ", 
                        VarAddr[i], UsageCount[i], Variable[i]);

        // write automatic bit assignments
        fprintf(lst, "\n    *** Automatic bit assignments ***\n");
        for (i = 0; i < NextDef; i++) {
                if (strncmp(Define[i], "_Flags", 6) == 0)
                        fprintf(lst, "%10s%s  -  %s\n", " ", Define[i], DefName[i]);
        }
        fprintf(lst, "\n    *** [End of file] ***\n");
        free(buf);
}
/******************************************************************************
                              End of GENLST.C
******************************************************************************/
