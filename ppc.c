#include <stdio.h>
static void license(void) {
	puts("MIT License");
	puts("");
	puts("Copyright (c) 2025 Tristan Styles");
	puts("");
	puts("Permission is hereby granted, free of charge, to any person obtaining a copy");
	puts("of this software and associated documentation files (the \"Software\"), to deal");
	puts("in the Software without restriction, including without limitation the rights");
	puts("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell");
	puts("copies of the Software, and to permit persons to whom the Software is");
	puts("furnished to do so, subject to the following conditions:");
	puts("");
	puts("The above copyright notice and this permission notice shall be included in all");
	puts("copies or substantial portions of the Software.");
	puts("");
	puts("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
	puts("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
	puts("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
	puts("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
	puts("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
	puts("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
	puts("SOFTWARE.");
}
#ifndef VERSION
#	define VERSION  1.0.0
#endif
//
// Build with https://github.com/stytri/m
//
// ::compile
// :+  $CC $CFLAGS $XFLAGS $SMALL-BINARY
// :+      -o $+^ $"* $"!
//
// ::debug
// :+  $CC $CFLAGS
// :+      -Og -g -DDEBUG_$: -o $^-$+: $"!
// :&  $DBG -tui --args $^-$+: $"*
// :&  $RM $^-$+:
//
// ::-
// :+  $CC $CFLAGS $XFLAGS $SMALL-BINARY
// :+      -o $+: $"* $"!
//
// ::CFLAGS!CFLAGS
// :+      -Wall -Wextra $WINFLAGS $INCLUDE
//
// ::XFLAGS!XFLAGS
// :+      -DNDEBUG=1 -O3 -march=native
//
// ::SMALL-BINARY
// :+      -fmerge-all-constants -ffunction-sections -fdata-sections
// :+      -fno-unwind-tables -fno-asynchronous-unwind-tables
// :+      -Wl,--gc-sections -s
//
// ::windir?WINFLAGS
// :+      -D__USE_MINGW_ANSI_STDIO=1
//
// ::INCLUDE!INCLUDE
// :+      -I ../inc
//
#include <string.h>
static char const *getfilename(char const *cs, int *lenp);
static void usage(char *arg0, FILE *out);
static void version(void) {
#define VERSION__STR(VERSION__STR__version)  #VERSION__STR__version
#define VERSIONTOSTR(VERSIONTOSTR__version)  VERSION__STR(VERSIONTOSTR__version)
	puts("## Version "VERSIONTOSTR(VERSION));
}
static void readme(char *arg0) {
	int         n;
	char const *file = getfilename(arg0, &n);
	printf("# %.*s\n", n, file);
	puts("");
	version();
	puts("");
	puts("Copies the files on the command line to stdout, prepending a C style #line directive to each file.");
	puts("");
	puts("## Command Line");
	puts("");
	puts("```");
	usage(arg0, stdout);
	puts("```");
}

//------------------------------------------------------------------------------

#include <hol/holibc.h>  // https://github.com/stytri/hol
#include <defer.h>       // https://github.com/stytri/defer
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

//------------------------------------------------------------------------------

#ifdef __MINGW64__
int _dowildcard = -1;
#endif

static struct optget options[] = {
	{  0, "Pre Process Cat",   NULL },
	{  0, "usage: %s [OPTION]... [FILE]...",   NULL },
	{  0, "options:",                NULL },
	{  1, "-h, --help",              "display this help and exit" },
	{  2, "    --version",           "display version and exit" },
	{  3, "    --license",           "display license and exit" },
	{  4, "    --readme",            "display readme and exit" },
	{  9, "-o, --output FILE",       "output to FILE" },
};
static size_t const n_options = (sizeof(options) / sizeof(options[0]));

static void usage(char *arg0, FILE *out) {
	optuse(n_options, options, arg0, out);
}

static inline bool qerror(char const *cs) {
	perror(cs);
	return false;
}

int
main(
	int    argc,
	char **argv
) {
	char const *infile = NULL;
	char const *outfile = NULL;

	int argi = 1;
	while((argi < argc) && (*argv[argi] == '-')) {
		char const *args = argv[argi++];
		char const *argp = NULL;
		do {
			int argn   = argc - argi;
			int params = 0;
			switch(optget(n_options - 2, options + 2, &argp, args, argn, &params)) {
			case 1:
				usage(argv[0], stdout);
				return 0;
			case 2:
				version();
				return 0;
			case 3:
				license();
				return 0;
			case 4:
				readme(argv[0]);
				return 0;
			case 9:
				outfile = argv[argi];
				break;
			default:
				errorf("invalid option: %s", args);
				usage(argv[0], stderr);
				return EXIT_FAILURE;
			}
			argi += params;
		} while(argp)
			;
	}
#ifdef _WIN32
	if(!outfile) {
		_setmode(_fileno(stdout), _O_BINARY);
	}
#endif
	bool eofnl = true;
	bool failed = false;
	DEFER(FILE *out = outfile ? fopen(outfile, "wb") : stdout,
		!(failed = !out) || qerror(outfile),
		out != stdout ? fclose(out) : (void)0
	) while(!failed && (argi < argc)) {
		infile = argv[argi++];
		DEFER(FILE *in = fopen(infile, "rb"),
			!(failed = !in) || qerror(infile),
			fclose(in)
		) {
			if(!eofnl) {
				fwrite("\n", sizeof(char), 1, out);
			}
			fwrite("#line 1 \"", sizeof(char), 9, out);
			fwrite(infile, sizeof(char), strlen(infile), out);
			fwrite("\"\n", sizeof(char), 2, out);
			eofnl = false;
			do {
				char buf[BUFSIZ];
				size_t n = fread(buf, sizeof(char), BUFSIZ, in);
				if(n > 0) {
					eofnl = (buf[n - 1] == '\n');
					fwrite(buf, sizeof(char), n, out);
				}
				if((failed = failed || ferror(in))) {
					perror(infile);
				}
				if((failed = failed || ferror(out))) {
					perror(outfile);
				}
			} while(!failed && !feof(in))
				;
		}
	}
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
