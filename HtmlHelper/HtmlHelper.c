#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#pragma warning(disable:4996)

void process (const char *filename)
{
   char newfile[FILENAME_MAX];

   char varname[FILENAME_MAX];


   strcpy_s  (newfile, sizeof newfile, filename);
   strcat_s (newfile, sizeof newfile, ".cpp");

   char *pos = strrchr (filename, '\\');
   if (pos != NULL)
      strcpy_s (varname, sizeof varname, pos+1);
   else
      strcpy_s (varname, sizeof varname, filename);

   pos = strchr (varname, '.');
   if (pos != NULL)
      *pos = '_';

   FILE *fin = fopen (filename, "r");
   FILE *fout = fopen (newfile, "w");

   if (fin != NULL && fout != NULL)
   {
      fputs ("#include <Arduino.h>\n", fout);
      fputs ("#include \"BMWifi.h\"\n", fout);
      fprintf (fout, "const char %s[] PROGMEM = \n", varname);
      int prevch = -1;
      while (!feof (fin) && !ferror (fin))
      {
         char input [255];
         if (NULL != fgets (input, sizeof input, fin))
         {
            fputs ("\"", fout);

            //pos = strstr (input, "//");
            //if (pos != NULL)
            //   *pos = '\0';

            for (char* ch = input; *ch != '\0'; ch++)
            {
               if (*ch == '\"')
                  fputs ("\\\"", fout);

               else if (*ch == '\n')
                  fputs ("\\n", fout);

               else if (*ch != ' ' || prevch != ' ')
                  fputc (*ch, fout);

               prevch = *ch;

            }

            fputs ("\"\n", fout);
         }
      }

      fputs (";\n", fout);
   }

   if (fin == NULL)
      perror (filename);
   else
      fclose (fin);

   if (fout == NULL)
      perror (newfile);
   else
      fclose (fout);
}


void main (int argc, char *argv[])
{
   for (int i=1; i<argc; i++)
   {
      printf ("%s\n", argv[i]);
      process (argv[i]);
   }

   exit (EXIT_SUCCESS);
}