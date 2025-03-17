/*
 * bin2c.c
 *
 * convert a binary file into a C source array.
 *
 * THE "BEER-WARE LICENSE" (Revision 3.1415):
 * sandro AT sigala DOT it wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff.  If we meet some
 * day, and you think this stuff is worth it, you can buy me a beer in
 * return.  Sandro Sigala
 *
 * Subsequently modified by Joel Sherrill <joel.sherrill@oarcorp.com>
 * to add a number of capabilities not in the original.
 * 
 * Modified by Harinder Singh Dhanoa <hsd1807@proton.me>
 * to add the feature to include license file support.
 *
 */

#define _XOPEN_SOURCE 700 // for strnlen
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int useconst = 1;
int usestatic = 0;
int verbose = 0;
int zeroterminated = 0;
int createC = 1;
int createH = 1;
unsigned int align = 0;
char *licensefile = NULL;

static void sanitize_file_name(char *p)
{
  while (*p != '\0') {
    if (!isalnum((unsigned char)*p)) /* cast to avoid negative indexing */
      *p = '_';
    ++p;
  }
}

int myfgetc(FILE *f)
{
  int c = fgetc(f);
  if (c == EOF && zeroterminated) {
    zeroterminated = 0;
    return 0;
  }
  return c;
}

char *read_license_file(const char *filename) {
  FILE *file;
  long length;
  char *buffer;
  size_t readsize;
  
  file = fopen(filename, "r");
  if (!file) {
    perror("error: could not open license file");
    exit(1);
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    perror("error: could not seek to end of license file");
    fclose(file);
    exit(1);
  }

  length = ftell(file);
  if (length == -1) {
    perror("error: could not tell position in license file");
    fclose(file);
    exit(1);
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    perror("error: could not seek to start of license file");
    fclose(file);
    exit(1);
  }

  buffer = malloc(length + 1);
  if (!buffer) {
    perror("error: could not allocate memory for license file");
    fclose(file);
    exit(1);
  }

  readsize = fread(buffer, 1, length, file);
  if (readsize != (size_t) length) {
    perror("error: could not read license file");
    free(buffer);
    fclose(file);
    exit(1);
  }
  buffer[length] = '\0';

  fclose(file);
  return buffer;
}

void process(const char *ifname, const char *ofname, const char *forced_name)
{
  FILE *ifile, *ocfile, *ohfile;
  char buf[PATH_MAX+1];
  char obasename[PATH_MAX+1];
  char ocname[PATH_MAX+5];
  char ohname[PATH_MAX+5];
  size_t len;

  ocfile = NULL;
  ohfile = NULL;

  /* Error check */
  if ( !ifname || !ofname ) {
    fprintf(stderr, "process has NULL filename\n");
    exit(1);
  }

  strncpy( obasename, ofname, PATH_MAX );
  len = strnlen( obasename, PATH_MAX );
  if ( len >= 2 ) {
    if ( obasename[len-2] == '.' ) {
      if ( (obasename[len-1] == 'c') || (obasename[len-1] == 'h') )
        obasename[len-2] = '\0';
    }
  }

  sprintf( ocname, "%s.c", obasename );
  sprintf( ohname, "%s.h", obasename );

  if ( verbose ) {
    fprintf(
      stderr,
      "in file: %s\n"
      "c file: %s\n"
      "h file: %s\n",
      ifname,
      ocname,
      ohname
    );
  }

  /* Open input and output files */
  ifile = fopen(ifname, "rb");
  if (ifile == NULL) {
    fprintf(stderr, "cannot open %s for reading\n", ifname);
    exit(1);
  }

  if ( createC ) {
    ocfile = fopen(ocname, "wb");
    if (ocfile == NULL) {
      fprintf(stderr, "cannot open %s for writing\n", ocname);
      exit(1);
    }
  }

  if ( createH ) {
    ohfile = fopen(ohname, "wb");
    if (ohfile == NULL) {
      fprintf(stderr, "cannot open %s for writing\n", ohname);
      exit(1);
    }
  }
  
  /* Read license header if specified */
  char *license_header = NULL;
  if (licensefile) {
    license_header = read_license_file(licensefile);
  }

  /* find basename */
  char *ifbasename_to_free =
    forced_name != NULL ? strdup(forced_name) : strdup(ifname);
  if ( ifbasename_to_free == NULL ) {
    fprintf(stderr, "cannot allocate memory\n" );
    fclose(ifile);
    if ( createC ) { fclose(ocfile); }
    if ( createH ) { fclose(ohfile); }
    exit(1);
  }

  char *ifbasename;
  ifbasename = basename(ifbasename_to_free);

  /* Ensure length of ifbasename is shorter than length of buf */
  if (strlen(ifbasename) > PATH_MAX+1) {
    fprintf(
      stderr,
      "error: Base name of %s is too long.\n",
      ifbasename
    );
    fclose(ifile);
    if ( createC ) { fclose(ocfile); }
    if ( createH ) { fclose(ohfile); }
    exit(1);
  }

  strcpy(buf, ifbasename);
  sanitize_file_name(buf);

  if ( createC ) {
    /* print C file header */
    if (license_header) {
      fprintf(ocfile, "%s\n", license_header);
      if (licensefile) {
        free(license_header);
      }
    }
    fprintf(
      ocfile,
      "/*\n"
      " *  Declarations for C structure representing binary file %s\n"
      " *\n"
      " *  WARNING: Automatically generated -- do not edit!\n"
      " */\n"
      "\n"
      "#include <sys/types.h>\n"
      "\n",
      ifbasename
    );

    /* print structure */
    fprintf(
      ocfile,
      "%s%sunsigned char %s[] ",
      ((usestatic) ? "static " : ""),
      ((useconst) ? "const " : ""),
      buf
    );
    if (align > 0) {
      fprintf(
        ocfile,
        "__attribute__(( __aligned__(%d) )) ",
        align
      );
    }
    fprintf(
      ocfile,
      "= {\n  "
    );
    int c, col = 1;
    while ((c = myfgetc(ifile)) != EOF) {
      if (col >= 78 - 6) {
        fprintf(ocfile, "\n  ");
        col = 1;
      }
      fprintf(ocfile, "0x%.2x, ", c);
      col += 6;

    }
    fprintf(ocfile, "\n};\n");

    /* print sizeof */
    fprintf(
      ocfile,
      "\n"
      "%s%ssize_t %s_size = sizeof(%s);\n",
      ((usestatic) ? "static " : ""),
      ((useconst) ? "const " : ""),
      buf,
      buf
    );
  } /* createC */

  /*****************************************************************/
  /******                    END OF C FILE                     *****/
  /*****************************************************************/

  if ( createH ) {
    /* print H file header */
    char hbasename[PATH_MAX];
    /* Clean up the file name if it is an abs path */
    strcpy(
      hbasename,
      obasename
    );
    sanitize_file_name(hbasename);
    fprintf(
      ohfile,
      "/*\n"
      " *  Extern declarations for C structure representing binary file %s\n"
      " *\n"
      " *  WARNING: Automatically generated -- do not edit!\n"
      " */\n"
      "\n"
      "#ifndef __%s_h\n"
      "#define __%s_h\n"
      "\n"
      "#include <sys/types.h>\n"
      "\n",
      ifbasename,  /* header */
      hbasename,  /* ifndef */
      hbasename   /* define */
    );

    /* print structure */
    fprintf(
      ohfile,
      "extern %s%sunsigned char %s[]",
      ((usestatic) ? "static " : ""),
      ((useconst) ? "const " : ""),
      buf
    );
    if (align > 0) {
      fprintf(
        ohfile,
        " __attribute__(( __aligned__(%d) ))",
        align
      );
    };
    /* print sizeof */
    fprintf(
      ohfile,
      ";\n"
      "extern %s%ssize_t %s_size;\n",
      ((usestatic) ? "static " : ""),
      ((useconst) ? "const " : ""),
      buf
    );

    fprintf(
      ohfile,
      "\n"
      "#endif\n"
    );
  } /* createH */

  /*****************************************************************/
  /******                    END OF H FILE                     *****/
  /*****************************************************************/

  fclose(ifile);
  if ( createC ) { fclose(ocfile); }
  if ( createH ) { fclose(ohfile); }
  free(ifbasename_to_free);
}

void usage(void)
{
  fprintf(
     stderr,
     "usage: bin2c [-csvzCH] [-N name] [-A alignment] [-l license_file] <input_file> <output_file>\n"
     "  <input_file> is the binary file to convert\n"
     "  <output_file> should not have a .c or .h extension\n"
     "\n"
     "  -c - do NOT use const in declaration\n"
     "  -s - do use static in declaration\n"
     "  -v - verbose\n"
     "  -z - add zero terminator\n"
     "  -H - create c-header only\n"
     "  -C - create c-source file only\n"
     "  -N - force name of data array\n"
     "  -A - add alignment - parameter can be a hexadecimal or decimal number\n"
     "  -l - <license_file> - add the specified file as a license header\n"
    );
  exit(1);
}

int main(int argc, char **argv)
{
  const char *name = NULL;
  while (argc > 3) {
    if (!strcmp(argv[1], "-c")) {
      useconst = 0;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-s")) {
      usestatic = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-v")) {
      verbose = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-z")) {
      zeroterminated = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-C")) {
      createH = 0;
      createC = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-H")) {
      createC = 0;
      createH = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-N")) {
      --argc;
      ++argv;
      if (argc <= 1) {
        fprintf(stderr, "error: -N needs a name\n");
        usage();
      }
      name = argv[1];
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-A")) {
      --argc;
      ++argv;
      if (argc <= 1) {
        fprintf(stderr, "error: -A needs an alignment\n");
        usage();
      }
      align = strtoul(argv[1], NULL, 0);
      if (align == 0) {
        fprintf(stderr, "error: Couldn't convert argument of -A\n");
        usage();
      }
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-l")) {
      --argc;
      ++argv;
      if (argc <= 1) {
        fprintf(stderr, "error: -l needs a license file\n");
        usage();
      }
      licensefile = argv[1];
      --argc;
      ++argv;
    } else {
      usage();
    }
  }
  if (argc != 3) {
    usage();
  }

  /* process( input_file, output_basename ) */
  process(argv[1], argv[2], name);
  return 0;
}