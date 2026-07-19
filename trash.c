/* Moves files and directories into the Recycle Bin of the current user

Copyright (c) 2026, LH_Mouse
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

#define _GNU_SOURCE  1
#define WIN32_LEAN_AND_MEAN  1
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/cygwin.h>
#include <windows.h>
#include <shellapi.h>

int
main(int argc, char** argv)
  {
    setlocale(LC_ALL, "");
    tzset();

    // Parse command-line options.
    bool opt_force = false;
    bool opt_help = false;
    bool opt_interactive = false;
    bool opt_verbose = false;
    bool opt_version = false;

    static const struct option s_long_options[] =
      {
        { .name = "directory", .has_arg = no_argument, .val = 'd' },
        { .name = "force", .has_arg = no_argument, .val = 'f' },
        { .name = "help", .has_arg = no_argument, .val = 'h' },
        { .name = "interactive", .has_arg = no_argument, .val = 'i' },
        { .name = "recursive", .has_arg = no_argument, .val = 'r' },
        { .name = "verbose", .has_arg = no_argument, .val = 'v' },
        { .name = "version", .has_arg = no_argument, .val = 'V' },
        { 0 }
      };

    int c;
    while((c = getopt_long(argc, argv, "dfhirRv", s_long_options, NULL)) != -1)
      switch(c)
        {
        case 'd':
          continue;

        case 'f':
          opt_force = true;
          continue;

        case 'h':
          opt_help = true;
          continue;

        case 'i':
          opt_interactive = true;
          continue;

        case 'r':
        case 'R':
          continue;

        case 'v':
          opt_verbose = true;
          continue;

        case 'V':
          opt_version = true;
          continue;

        default:
  do_report_invalid_options:
          fprintf(stderr, "Try `%s --help` for usage.\n", argv[0]);
          quick_exit(1);
        }

    if(opt_help) {
      printf(
//        1         2         3         4         5         6         7      |
// 34567890123456789012345678901234567890123456789012345678901234567890123456|
"Usage: %s [OPTIONS]... [--] PATHS...\n"
"\n"
"Moves PATHS into the Recycle Bin of the current user.\n"
"\n"
"Options:\n"
"  -d, --directory       (ignored)\n"
"  -f, --force           ignore nonexistent paths\n"
"  -h, --help            show help message then exit\n"
"  -i, --interactive     prompt before removing every path\n"
"  -r, -R, --recursive   (ignored)\n"
"  -v, --verbose         print status\n"
"  -V, --version         show version information then exit\n"
// 34567890123456789012345678901234567890123456789012345678901234567890123456|
//        1         2         3         4         5         6         7      |
        , argv[0]);
      quick_exit(0);
    }

    if(opt_version) {
      printf("Unversioned\n");
      quick_exit(0);
    }

    // The remaining arguments are paths to trash.
    if(optind >= argc) {
      fprintf(stderr, "%s: missing operand\n", argv[0]);
      goto do_report_invalid_options;
    }

    wchar_t* wpath = NULL;
    int has_errors = 0;

    for(int k = optind;  k < argc;  ++k) {
      if(opt_interactive) {
        // Ask the user for confirmation.
        char resp[256];
        do
          printf("Trash '%s'? (y/N) ", argv[k]);
        while(fgets(resp, sizeof(resp), stdin) && (resp[0] != '\n')
              && (resp[0] != 'y') && (resp[0] != 'Y')
              && (resp[0] != 'n') && (resp[0] != 'N'));

        if((resp[0] != 'y') && (resp[0] != 'Y'))
          continue;
      }

      if(access(argv[k], W_OK) != 0) {
        // Report an error.
        if(!(opt_force && (errno == ENOENT)))
          fprintf(stderr, "Cannot trash '%s': %m\n", argv[k]);
        continue;
      }

      // Convert the MSYS2 path to an absolute Win32 wide string, with two null
      // terminators as required by `SHFileOperationW()`.
      ssize_t wpath_cb = cygwin_conv_path(CCP_POSIX_TO_WIN_W, argv[k], NULL, 0);
      if(wpath_cb < 0) {
        has_errors |= 1;
        fprintf(stderr, "Invalid path '%s': %m\n", argv[k]);
        continue;
      }

      wpath = realloc(wpath, (size_t) wpath_cb + sizeof(wchar_t));
      if(!wpath)
        abort();
      *(wpath + wpath_cb / sizeof(wchar_t)) = 0;
      if(cygwin_conv_path(CCP_POSIX_TO_WIN_W, argv[k], wpath, (size_t) wpath_cb) != 0) {
        has_errors |= 1;
        fprintf(stderr, "Invalid path '%s': %m\n", argv[k]);
        continue;
      }

      // Remove it using SHELL32.
      if(opt_verbose)
        printf("Trashing '%ls'...\n", wpath);

      SHFILEOPSTRUCTW fileop = {0};
      fileop.wFunc = FO_DELETE;
      fileop.pFrom = wpath;
      fileop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
      has_errors |= SHFileOperationW(&fileop);
      has_errors |= fileop.fAnyOperationsAborted;
    }

    // Exit.
    free(wpath);
    quick_exit(has_errors ? 2 : 0);
  }
