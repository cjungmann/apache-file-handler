---
author:
  name: Chuck Jungmann
  email: chuck@cpjj.net
description: 'Using a FASTCGI Script as a File Handler'
keywords: 'apache,fastcgi,SetHandler,AddType,mod_fastcgi'
license: '[CC BY-ND 3.0](http://creativecommons.org/licenses/by-nd/3.0/us/)'
modified: 'Tuesday, June 22nd, 2016'
modified_by:
  name: Chuck Jungmann
published: 'Tuesday, June 22nd, 2016'
title: 'Configuring A Custom File Handler for Apache'
external_resources:
 - '[Apache HTTP Server Version 2.4 Documentation](https://httpd.apache.org/docs/2.4/)'
 - '[Module mod_fastcgi](https://htmlpreview.github.io/?https://github.com/FastCGI-Backups/mod_fastcgi/blob/master/docs/mod_fastcgi.html)'
 - '[Archive of FastCGI.com](https://github.com/FastCGI-Archives/FastCGI.com/blob/master/README.md)'
---
I have written a script interpreter that I want to run when a URL calls for a
document with a specific extension.  The model for this expectation is PHP, where
when a file with a _.php_ extension is called, the PHP interpreter takes over.

This objective turned out to be much harder than I expected.  The problem was not
that it required sophisticated coding or complicated configuration settings but
rather that I couldn't find many relevant examples and I struggled to adapt the
examples I found.

I hope this guide helps others avoid the frustration of endless trial-and-error
combining Apache configuration commands.

## Assumptions

It is assumed that readers of this guide are familiar with CGI programming.
Although an inexperienced programmer could follow the steps in this guide to
successfully create the sample file handler, the exercise is only useful for
programmers who can write their own CGI scripts.

It is also assumed that persons following this guide have a running LAMP
installation and are at least familiar with how to write or copy files in
a write-protected directory, particularly directories under `/etc/apache2`.

## Prerequisites

1. A running LAMP installation.  Complete the
   [Getting Started](/docs/getting-started) guide.

2. Install `build-essential` for C/C++ compiling the FastCGI libraries and
   the sample FastCGI program.
   
       sudo apt-get install build-essential

3. Download, compile, and install FastCGI libraries.

   Note: When writing this guide, the otherwise lost contents of fastcgi.com have
   been archived at [FastCGI Archives](https://github.com/FastCGI-Archives/FastCGI.com),
   and the `wget` URL below reflects this relocation.

   ~~~
   cd /usr/local/src
   sudo wget https://github.com/FastCGI-Archives/FastCGI.com/blob/master/original_snapshot/fcgi-2.4.1-SNAP-0910052249.tar.gz
   sudo tar -xvz fcgi-2.4.1*
   cd fcgi-2.4.1*
   sudo ./configure
   sudo make
   sudo make install
   ~~~

4. Install `mod_fastcgi`

       sudo apt-get install libapache2_mod_fastcgi

5. Enable Apache mods _actions_ and _fastcgi_

       sudo a2enmod actions fastcgi

The remaining steps for creating a file handler are the topic of this guide.  The
remaining steps are
- [Create a skeleton FastCGI program](#CreateSkeleton)
- [Establish the FastCGI program as a file handler](#EstablishFastCGIProgram)
- [Confirm file handler operation](#TestHandlerOperation)
- [Flesh-out the skeleton program](#FleshOutSkeleton) with code to access
  the target file.
- [Failed Experiments with simplefcgi.conf](#failed_experiments_with_simplefcgi_conf)

A final section, [Discussion of Simplefcgi.conf]{#DiscussSimplefcgiConf} is my
attempt to make sense of the configuration file that establishes the FastCGI program
as the file handler.


## Create a Skeleton FastCGI Program   {#CreateSkeleton}

Programmers who already have a FastCGI script to use, feel free to use that
program instead of this example.  Skip ahead to
[the next step](#EstablishFastCGIProgram) and replace paths in the configuration
file with the path to your FastCGI script.  You may also find the
[final coding section](#FleshOutSkeleton) useful for modifying your FastCGI
script to work as a file handler.

For those still following here, the first FastCGI program will only print out
a result, not reading the contents of the target file that caused the program
to be called.  


1. Create the following source file of a very basic FastCGI application:

   {:.file} simplefcgi.c
   
   ~~~c
   #include <sys/types.h>  // For return value type of getpid()
   #include <unistd.h>     // for getpid()

   // This replaces <stdio.h> for FastCGI output.
   #include <fcgi_stdio.h>

   /** Output a complete header, including the double newline marking its end. */
   void print_headers(void)
   {
      fputs("Status: 200 OK\n", stdout);
      fputs("Content-Type: text/html\n", stdout);
      fputc('\n', stdout);
   }

   int main(int argc, char **argv)
   {
      int count = 0;
      int procid = getpid();

      // The next line makes it a FastCGI application, assuming an
      // entire response occurs within the loop.
      while (FCGI_Accept()==0)
      {
         print_headers();
         fputs("<html><head><title>Apache File Handler Demo</title><head>\n", stdout);
         fputs("<body>\n<h1>Apache File Handler Demo</h1>\n<p>", stdout);

         printf("Number of responses for process %ld: %d.\n", pid_t, ++count);

         fputs("</p>\n</body>\n</html>\n", stdout);
      }

      return 0;
   }
   ~~~

2. Compile and install the program.

   This can be done on the command line like this:
   
   ~~~
   gcc simplefcgi.c -L /usr/local/lib -lfcgi -o simple.fcgi
   sudo cp -R simple.fcgi /usr/local/lib/cgi-bin
   ~~~

   but once the FastCGI server is running, Apache will have to be stopped before
   the program can be replaced.  It will be easier with a makefile.  Note that
   `sudo` is not included in the lines of the makefile because `sudo` will have
   been used to call `sudo make install`.

   {:file} makefile
   ~~~
   all: simple.fcgi

   simple.fcgi : simplefcgi.c
      gcc simplefcgi.c -L /user/local/lib -lfcgi -o simple.fcgi

   install:
      /etc/init.d/apache2 stop
      cp -R simple.fcgi /usr/local/lib/cgi-bin
      /etc/init.d/apache2 start
   ~~~

   Using the makefile, type `make` to compile the code, then `sudo make install`
   to copy the new file between stopping and starting Apache.

## Establish the FastCGI Program as a File Handler {#EstablishFastCGIProgram}

Up until here, we have been preparing the pieces with which the new
configuration can be demonstrated.

1. Create the following file.

   {:.file} /etc/apache2/conf-available/simplefcgi.conf
   ~~~apache
   <IfModule mod_fastcgi.c>
      <IfModule mod_actions.c>
         FastCgiServer /usr/local/lib/cgi-bin/simple.fcgi -processes 1
         Alias /simple-cgi-bin /usr/local/lib/cgi-bin
         Action application/simpledoc /simple-cgi-bin/simple.fcgi
         <Directory /usr/local/lib/cgi-bin>
            Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch
            Require all granted
         </Directory>

         <FilesMatch ".*\.simple$">
            SetHandler application/simpledoc
         </FilesMatch>
      </IfModule>
   </IfModule>
   ~~~

2. Enable the configuration.

       sudo a2enconf simplefcgi

3. Restart Apache.

       sudo /etc/init.d/apache2 restart

## Confirm File Handler Operation {#TestHandlerOperation}

In this guide, the example detects and handles files with _.simple_ extension.

The .simple file can be temporarily added to the root directory of any working
virtual host on the system and tested by browsing to the file.  Alternatively,
the following steps provide a guide to creating a new virtual host for testing
our file handler.

1. Create a directory to host the site

       sudo mkdir /var/www/simpledemo

2. Create an empty file with the _.simple_ extension

       sudo touch /var/www/simpledemo/index.simple

3. Create a Virtual Host configuration file.  Note that there are no accomodations
   for running CGI or .simple files in this virtual host.

   {:file} /etc/apache2/sites-available/simpsamp.conf
   ~~~apache
   <VirtualHost *:80>
      DocumentRoot /var/www/simpledemo
      DirectoryIndex index.simple
      ServerName simpsamp
      ServerAlias /
   </VirtualHost>
   ~~~

3. Enable the new site

       sudo a2ensite simpsamp

4. Restart Apache

       sudo /etc/init.d/apache2

5. Access Target File with Browser       

   With the preceeding steps completed, pointing a browser to the page will
   confirm that it's working.  Assuming that there is a single FastCGI server process
   running, refreshing the page should cause the `Number of responses for process ...`
   line to increment.

## Flesh-out the Skeleton Program {#FleshOutSkeleton}

You may have noticed that the sample C script does not act on the contents of the
target file.  This section completes the development of the .simple file handler
by reading target file and tailoring the response according to its contents.

### Add Code to Read and Interpret The Target File

This is the ultimate goal of this entire exercise: to treat the target file
as a script.  The file handler program has to find and open the target file
for that to happen.

1. Get the File Path.  The full filesystem path to the target file should be in
   environment variable `PATH_TRANSLATED`.  This value should be used to open
   the file.  Add the following excerpt above function `main()` in the sample
   source file.

   (If `PATH_TRANSLATED` is missing or contains an unsuitable value, see the
   [Display All Environment Variables](#DisplayEnvironment) section for some
   help.)

2. Open the file and read its contents.

3. Generate a custom response according to the target file's contents.

Here is an example:

{:.file-excerpt} simplefcgi.c
~~~c
void process_target(void)
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
~~~

Use the example by calling `process_target` in the while loop of `main` in the
example source file:.

### Display All Environment Variables {#DisplayEnvironment}

There are two purposes for this section.  First, while on my system,
`PATH_TRANSLATED` contains the information I need to open the target file,
it is not a standard CGI variable and thus it may be missing or contain
something other than the full system path to the target file.  In that case,
it is helpful to have the FastCgi program list all of the environment variables
to see if another can help you find the file.

The second reason for displaying all the variables is to simply scan through
the data available to your FastCGI program.  I used something like this for
my first CGI program to see how a query string is formatted, for example.

#### The print_environment Function

Add the following function before `main()` in your source file, then add
a call to this `print_environment()` function in the `while` loop of your
`main()` function.  Don't forget to remove the call in `main()` before
deploying this in a web application.

{.file-excerpt} simplefcgi.c
~~~
/**
 * Write a unordered list of environment variables.
 *
 * This function is not safe for a public site because it reveals private
 * information about your host computer.  Use it for reference in development,
 * then remove any calls to the function before deploying your web application.
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
~~~


## Discussion of simplefcgi.conf  {#DiscussSimplefcgiConf}

A proper understanding of Apache configuration files and directives clearly
eludes me.  I was successful in establishing a file handler largely through
trial-and-error, somewhat informed by guides to running PHP as a FastCGI application.
A final section about [failed experiments](#FailedExperiments) will talk about
my misconceptions.

Note that these directives occur in the global space, not within a virtual host
block.  When the configuration works, a virtual host will not need to specifically
enable the handler to get the desired behavior.  It will work like PHP in that any
file with the defined extension will be processed with the designated FastCGI script.

### Conditionally Applying Directives
The lines
~~~apache
<IfModule mod_fastcgi.c>
   <IfModule mod_actions.c>
~~~
prevent the file handler from being installed if the mods are not enabled.  If the
handler isn't installed, the end-user will see the contents of the file with the
named extension (_.simple_ in the present case).  While it might be easier to notice
the error that would be generated when loading a configuration if a necessary mod was
not enabled, the error message would be at the expense of the all other services
failing if Apache restarted for some other reason.

### Managing FastCGI Instantiation
~~~apache
FastCgiServer /usr/local/lib/cgi-bin/simple.fcgi -processes 1
~~~
This seems to be optional (see [Failed Experiments](#FailedExperiments)), this
directive allows you customize several attributes of your FastCGI script.

### Associating a Type with a Script
~~~apache
Alias /simple-cgi-bin /usr/local/lib/cgi-bin
Action application/simpledoc /simple-cgi-bin/simple.fcgi
~~~
Together, these two directives associate a type _application/simpledoc_ with a
script file _simple.fcgi_.  The `Alias` directive hides the true location of
the script file with a URL alias that is used by the `Action` directive to
find the script.

### Enabling CGI Execution in the Script's Host Directory
~~~apache
<Directory /usr/local/lib/cgi-bin>
   Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch
   Require all granted
</Directory>
~~~
This section enables CGI execution for files found in the named filesystem
directory path.

The directives to this point associate a type, _application/simpledoc_ with a
script file, _/usr/local/lib/cgi-bin/simple.fcgi_.  The final step is to associate
a file extension with the type.

### Associating the File Extension with the Type
~~~apache
<FilesMatch ".*\.simple$">
   SetHandler application/simpledoc
</FilesMatch>
~~~
This `FilesMatch` directive uses a regular expression to match files that end
with _.simple_ .  Note the _$_ at the end matches an end-of-string, so a file
like _bogus.simple.jpeg_ will not be matched.  Nothing else is required beyond
the global `SetHandler` directive for matching files.


### Failed Experiments with simplefcgi.conf 

#### Using a CGI script as a handler
I wanted to see if a CGI script could work as a handler.  Except for the
`FastCgiServer` directive, nothing in the configuration seemed exclusive to
running in FastCGI mode.  I wanted to test my conclusions about Apache configuration
directives, and I also thought it would be more convenient running as CGI so I
wouldn't have to stop Apache before and restart it after installing a new version
of the script.

Despite many efforts, I could never get a CGI script to work as a file handler.

- I tried removing the `FastCgiServer` directive.  Surprisingly, this has __no effect__:
  the script continued to run as a FastCGI server, indicated by multiple page replots
  increasing the value in the _Number of responses for process_ line.  It seems
  sufficient to start FastCGI that there is an enabled FastCGI mod and call for the
  FastCGI program.  In that case, the `FastCgiServer` directive is for pre-starting
  the FastCGI program, and to dictate the number of instances that will be running,
  among other characteristics.

- Then I tried to force CGI mode by making a copy of `simple.fcgi` named `simple.cgi`
  so as to prevent the `fcgi` file extension from triggering FastCGI mode, then
  I changed the `Action` directive to point to `simple.cgi` instead of `simple.fcgi`.
  In this case, the browser reported
  `The requested URL /simple-cgi-bin/simple.cgi/demo.simple was not found on this server.`

- Leaving the settings the same, the next step was to modify _simplefcgi.c_ by removing
  the `<fcgi_stdio.h>` include and the `while (FCGI_Accept())` statement to prevent any
  FastCGI processing.  This made no difference.

As a result of these efforts, I don't believe that a CGI script can be used as a
file handler.  Academically, I would like to be proven wrong.  Technically, this
is not a big problem as the usage of CGI scripts is discouraged, anyway.  However,
I was disappointed and now feel less confident in my understanding of Apache
configurations.

#### Omitted the Alias Directive

Just to see if the `Action` directive could use a filesystem path, I attempted to
use the real path to `simple.fcgi`.  It failed.  The second parameter of the `Action`
directive requires a relative URL.

#### Using /cgi-bin as the Alias

This is trivial, but changing the `Alias` and `Action` directives to `cgi-bin` from
`simple-cgi-bin` resulted in the server being unable to find `simple.fcgi`.  I guess
that `cgi-bin` is reserved for /usr/lib/cgi-bin.


## Some Linode Resources That May Be Helpful
- [Apache Configuration Structure](https://www.linode.com/docs/websites/apache-tips-and-tricks/apache-configuration-structure)
- [Install PHP-FPM and Apache on Debian 8 (Jessie)](https://www.linode.com/docs/websites/apache/install-php-fpm-and-apache-on-debian-8)



