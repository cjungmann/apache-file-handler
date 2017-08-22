// -*- compile-command: "gcc simplefcgi.c -L /usr/local/lib -lfcgi -o simple.fcgi" -*-

#include <sys/types.h>
#include <sys/stat.h>   // for fstat()
#include <unistd.h>     // for getpid(), fstat()
#include <stdlib.h>     // for getenv

#include <fcgi_stdio.h>

/** In this simple program, we'll assume all requests are successful. */
void print_headers(void)
{
   fputs("Status: 200 OK\n", stdout);
   fputs("Content-Type: text/html\n", stdout);
   putc('\n', stdout);
}

/**
 * Not safe for public site, this is useful to see exactly what
 * information is available in the environment variables.
 */
void print_environment(void)
{
   extern char **environ;
   char **env = environ;

   fputs("<h2>CGI Environment listing</h2>\n<ul>\n", stdout);

   while (*env)
   {
      fputs("<li>", stdout);
      fputs(*env, stdout);
      fputs("</li>\n", stdout);

      ++env;
   }

   fputs("</ul>\n", stdout);
}

/** Open file that initiated the call to this script with minimal error-checking.*/
void simple_process_file(void)
{
   char buffer[512];                       // buffer for fgets
   char *info = getenv("PATH_INFO");       // URL path to file
   char *path = getenv("PATH_TRANSLATED"); // Filesystem path to file

   printf("<h2>Processing file \"%s\".</h2>\n", info);
   FILE *f = fopen(path, "r");
   if (f)
   {
      char *found = fgets(buffer, sizeof(buffer), f);
      if (found)
         printf("<p>\"%s\"</p>\n", found);
      else
         fputs("<p>fgets failed</p>\n", stdout);

      fclose(f);
   }
}

/** Open file that initiated the call to this script with better error-checking.*/
void process_file(void)
{
   char buffer[512];                       // buffer for fgets
   char *info = getenv("PATH_INFO");       // URL path to file
   char *path = getenv("PATH_TRANSLATED"); // Filesystem path to file

   fputs("<h2>Processing File</h2>\n", stdout);

   fputs("<p>In process_file: ", stdout);

   if (info && path)
   {
      FILE *f = fopen(path, "r");
      if (f)
      {
         fputs("opened file \"", stdout);
         fputs(info, stdout);
         fputc('"', stdout);
         
         char *found = fgets(buffer, sizeof(buffer), f);
         if (found)
         {
            fputs(" contents=\"", stdout);
            fputs(found, stdout);
            fputc('"', stdout);
         }
         else
            fputs(" empty file", stdout);

         fclose(f);
      }
      else
         printf("<h2>Failed to open \"%s\"</h2>\n", info);
   }
   else
      fputs("unknown file", stdout);

   fputs("</p>\n", stdout);
}

void print_page(int *count, int id_process)
{
   print_headers();
   fputs("<html><head><title>Apache File Handler Test</title><head>\n", stdout);
   fputs("<body>\n<h1>Apache File Handler Test</h1>\n", stdout);

   printf("<p>Number of responses for process %d: %d.</p>\n", id_process, ++(*count));

   process_file();

   print_environment();

   fputs("</body>\n</html>\n", stdout);
}

int main(int argc, char **argv)
{
   int count = 0;
   int procid = getpid();

   while (FCGI_Accept()==0)
   {
      print_headers();
      fputs("<html><head><title>Apache File Handler Demo</title><head>\n", stdout);
      fputs("<body>\n<h1>Apache File Handler Demo</h1>\n", stdout);

      printf("<p>Number of responses for process %d: %d.</p>\n", procid, ++count);

      process_file();

      print_environment();

      fputs("</body>\n</html>\n", stdout);
   }

   return 0;
}
