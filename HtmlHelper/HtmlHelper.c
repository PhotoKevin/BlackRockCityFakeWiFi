#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "getopt.h"

#pragma warning(disable:4996)
// Ignore Spelling: const


void processFile (FILE *fout, FILE *fin, const char *varname)
{
   printf ("   varname is now %s\n", varname);

   if (fin != NULL && fout != NULL)
   {
      fputs ("\n\n", fout);
      fprintf (fout, "const char %s[] PROGMEM = \n", varname);
      int prevch = -1;
      while (!feof (fin) && !ferror (fin))
      {
         char input [255];
         if (NULL != fgets (input, sizeof input, fin))
         {
            fputs ("\"", fout);

            for (char* ch = input; *ch != '\0'; ch++)
            {
               if (*ch == '\"')
                  fputs ("\\\"", fout);

               else if (*ch == '\n')
                  fputs ("\\n", fout);

               else if (*ch == '\\')
                  fputs ("\\\\", fout);

               else if (*ch != ' ' || prevch != ' ')
                  fputc (*ch, fout);

               prevch = *ch;

            }

            fputs ("\"\n", fout);
         }
      }

      fputs (";\n", fout);
   }
}

/// <summary>
/// Given a web file name such as index.html, figure out the appropriate 
/// variable name index_html.
/// </summary>
/// <param name="filename">Name of the web file</param>
/// <returns>The variable name</returns>
const char *variableName (const char *filename)
{
   static char varname[FILENAME_MAX];

   printf ("proc %s\n", filename);
   // Throw away any path portion.
   char *pos = strrchr (filename, '\\');
   if (pos != NULL)
      strcpy_s (varname, sizeof varname, pos+1);
   else
      strcpy_s (varname, sizeof varname, filename);

   // Replace all period and minus signs with an underscore.
   do
   {
      pos = strchr (varname, '.');
      if (pos != NULL)
         *pos = '_';
   } while (pos != NULL);

   do
   {
      pos = strchr (varname, '-');
      if (pos != NULL)
         *pos = '_';
   } while (pos != NULL);

   printf ("    -> %s\n", varname);
   return varname;
}

void process (const char *outputfile, const char *filename, const char *includefile)
{
   char newfile[FILENAME_MAX];
   FILE *fout;

   // NULL means each input gets its own output.
   if (outputfile == NULL)
   {
      strcpy_s  (newfile, sizeof newfile, filename);
      strcat_s (newfile, sizeof newfile, ".cpp");
      fout = fopen (newfile, "w");
      fputs ("#include <Arduino.h>\n", fout);
      if (includefile != NULL)
         fprintf (fout, "#include \"%s\"\n", includefile);
   }
   else
   {
      fout = fopen (outputfile, "a");
   }

   FILE *fin = fopen (filename, "r");

   processFile (fout, fin, variableName (filename));

   if (fin == NULL)
      perror (filename);
   else
      fclose (fin);

   if (fout == NULL)
      perror (newfile);
   else
      fclose (fout);
}

void usage (void)
{
   exit (EXIT_FAILURE);
}

void main (int argc, char *argv[])
{
   char *outputfile = NULL;
   char *includefile = NULL;

   static struct option longopts[] = 
   {
      { "include", required_argument, NULL, 'i' },
      { "output", required_argument, NULL, 'o' },
	   { NULL, 	0,			NULL, 		0 }
   };

   int ch;
   while ((ch = getopt_long(argc, argv, "o:", longopts, NULL)) != -1)
   {
   	switch (ch) 
      {
   	case 'o':
         outputfile = optarg;
		   break;

      case 'i':
         includefile = optarg;
         break;

   	default:
	   	usage();
		   /* NOTREACHED */
	   }
   }
   argc -= optind;
   argv += optind;

   if (outputfile != NULL)
   {
      FILE *fout = fopen (outputfile, "w");
      if (fout != NULL)
      {
         fputs ("#include <Arduino.h>\n", fout);
         if (includefile != NULL)
            fprintf (fout, "#include \"%s\"\n", includefile);
         fclose (fout);
      }
   }

   for (int i=0; i<argc; i++)
   {
      printf ("ARG: %s\n", argv[i]);
      process (outputfile, argv[i], includefile);
   }

   exit (EXIT_SUCCESS);
}