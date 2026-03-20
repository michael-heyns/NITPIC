/* for info to caller */

   #define MAXLINES		500
   #define LINELEN		80

/* prototypes for 'CONFIG.C' */

   int OpenINIfile(char *Filename);
   int CloseINIfile(void);
   char *GetINIvalue(char *Section, char *Title);
   char *GetNEXTvalue(void);
