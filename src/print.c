/* Output or logging functions for GNU Make.  

Copyright (C) 2004 Free Software Foundation, Inc.  This file is part
of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "make.h"
#include "expand.h"
#include "print.h"
#include "debug.h"
#include "dbg_cmd.h"

#if HAVE_VPRINTF || HAVE_DOPRNT
# define HAVE_STDVARARGS 1
# if __STDC__
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif
# if HAVE_VPRINTF
#  define VA_PRINTF(fp, lastarg, args) vfprintf((fp), (lastarg), (args))
# else
#  define VA_PRINTF(fp, lastarg, args) _doprnt((lastarg), (args), (fp))
# endif
# define VA_END(args) va_end(args)
#else
/* # undef HAVE_STDVARARGS */
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
# define VA_START(args, lastarg)
# define VA_PRINTF(fp, lastarg, args) fprintf((fp), (lastarg), va_alist)
# define VA_END(args)
#endif


/* Print a message on stdout.  */

void
#if __STDC__ && HAVE_STDVARARGS
message (int prefix, const char *fmt, ...)
#else
message (int prefix, const char *fmt, va_alist)
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (fmt != 0)
    {
      if (prefix)
	{
	  if (makelevel == 0)
	    printf ("%s: ", program);
	  else
	    printf ("%s[%u]: ", program, makelevel);
	}
      VA_START (args, fmt);
      VA_PRINTF (stdout, fmt, args);
      VA_END (args);
      putchar ('\n');
    }

  fflush (stdout);
}

/* Print an error message.  */

void
#if __STDC__ && HAVE_STDVARARGS
error (const struct floc *flocp, const char *fmt, ...)
#else
error (flocp, fmt, va_alist)
     const struct floc *flocp;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (flocp && flocp->filenm)
    fprintf (stderr, "%s:%lu: ", flocp->filenm, flocp->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: ", program);
  else
    fprintf (stderr, "%s[%u]: ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  putc ('\n', stderr);
  fflush (stderr);
  if (debugger_on_error & DEBUGGER_ON_ERROR) 
    enter_debugger(NULL, NULL, -1);
}

void
#if __STDC__ && HAVE_STDVARARGS
err (target_stack_node_t *p_call, const char *fmt, ...)
#else
err (p_call, fmt, va_alist)
     target_stack_node_t *p_call;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif
  floc_t *p_floc   = NULL;
  file_t *p_target = NULL;
   
  log_working_directory (1);

  if (p_call && p_call->p_target) {
    p_target = p_call->p_target;
    p_floc   = &(p_target->floc);
  }
  
  if (p_floc && p_floc->filenm)
    fprintf (stderr, "%s:%lu: ", p_floc->filenm, p_floc->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: ", program);
  else
    fprintf (stderr, "%s[%u]: ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  putc ('\n', stderr);
  if (extended_errors) print_target_stack(p_call, -1);
  fflush (stderr);
  if (debugger_on_error & DEBUGGER_ON_ERROR) 
    enter_debugger(p_call, p_target, -1);
}

/* Print an error message and exit.  */

void
#if __STDC__ && HAVE_STDVARARGS
fatal (const struct floc *flocp, const char *fmt, ...)
#else
fatal (flocp, fmt, va_alist)
     const struct floc *flocp;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (flocp && flocp->filenm)
    fprintf (stderr, "%s:%lu: *** ", flocp->filenm, flocp->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: *** ", program);
  else
    fprintf (stderr, "%s[%u]: *** ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  fputs (_(".  Stop.\n"), stderr);

  if ( (debugger_on_error & DEBUGGER_ON_FATAL) || debugger_enabled )
    enter_debugger(NULL, NULL, 2);
  die (2);
}

/* Print an error message and exit.  */

void
#if __STDC__ && HAVE_STDVARARGS
fatal_err(target_stack_node_t *p_call, const char *fmt, ...)
#else
fatal_err (flocp, fmt, va_alist)
     target_stack_node_t *p_call;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif
  floc_t *p_floc   = NULL;
  file_t *p_target = NULL;

  log_working_directory (1);

  if (p_call && p_call->p_target) {
    p_target = p_call->p_target;
    p_floc   = &(p_target->floc);
  }
  
  if (p_floc && p_floc->filenm)
    fprintf (stderr, "%s:%lu: *** ", p_floc->filenm, p_floc->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: *** ", program);
  else
    fprintf (stderr, "%s[%u]: *** ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  fputs (_(".  Stop.\n"), stderr);
  if (extended_errors) print_target_stack(p_call, -1);
  if ( (debugger_on_error & DEBUGGER_ON_FATAL) || debugger_enabled )
    enter_debugger(p_call, p_target, 2);
  die (2);
}

#ifndef HAVE_STRERROR

#undef	strerror

char *
strerror (int errnum)
{
  extern int errno, sys_nerr;
#ifndef __DECC
  extern char *sys_errlist[];
#endif
  static char buf[] = "Unknown error 12345678901234567890";

  if (errno < sys_nerr)
    return sys_errlist[errnum];

  sprintf (buf, _("Unknown error %d"), errnum);
  return buf;
}
#endif

/*! Print an error message from errno.  */
void
perror_with_name (const char *str, const char *name)
{
  error (NILF, _("%s%s: %s"), str, name, strerror (errno));
  if (debugger_on_error & DEBUGGER_ON_ERROR) 
    enter_debugger(NULL, NULL, -1);
}

/*! Print an error message from errno and exit.  */

void
pfatal_with_name (const char *name)
{
  fatal (NILF, _("%s: %s"), name, strerror (errno));
  /* NOTREACHED */
}

/*! Under -d, write a message describing the current IDs.  */

void
log_access (char *flavor)
{
  if (! ISDB (DB_JOBS))
    return;

  /* All the other debugging messages go to stdout,
     but we write this one to stderr because it might be
     run in a child fork whose stdout is piped.  */

  fprintf (stderr, _("%s: user %lu (real %lu), group %lu (real %lu)\n"),
	   flavor, (unsigned long) geteuid (), (unsigned long) getuid (),
           (unsigned long) getegid (), (unsigned long) getgid ());
  fflush (stderr);
}


/*! Write a message indicating that we've just entered or
   left (according to ENTERING) the current directory.  */
void
log_working_directory (int entering)
{
  static int entered = 0;

  /* Print nothing without the flag.  Don't print the entering message
     again if we already have.  Don't print the leaving message if we
     haven't printed the entering message.  */
  if (! print_directory_flag || entering == entered)
    return;

  entered = entering;

  if (print_data_base_flag)
    fputs ("# ", stdout);

  /* Use entire sentences to give the translators a fighting chance.  */

  if (makelevel == 0)
    if (starting_directory == 0)
      if (entering)
        printf (_("%s: Entering an unknown directory\n"), program);
      else
        printf (_("%s: Leaving an unknown directory\n"), program);
    else
      if (entering)
        printf (_("%s: Entering directory `%s'\n"),
                program, starting_directory);
      else
        printf (_("%s: Leaving directory `%s'\n"),
                program, starting_directory);
  else
    if (starting_directory == 0)
      if (entering)
        printf (_("%s[%u]: Entering an unknown directory\n"),
                program, makelevel);
      else
        printf (_("%s[%u]: Leaving an unknown directory\n"),
                program, makelevel);
    else
      if (entering)
        printf (_("%s[%u]: Entering directory `%s'\n"),
                program, makelevel, starting_directory);
      else
        printf (_("%s[%u]: Leaving directory `%s'\n"),
                program, makelevel, starting_directory);
}

/*! Display a variable and its value. */
void 
print_variable (variable_t *p_v)
{
  if (p_v) {
    if (NULL != p_v->fileinfo.filenm) {
      printf("%s:%lu %s = %s\n", 
	     p_v->fileinfo.filenm, p_v->fileinfo.lineno,
	     p_v->name, p_v->value);
    } else {
      printf("(null):0 %s = %s\n", 
	     p_v->name, p_v->value);
    }
  }
}

/*! Display a variable and its value with all substitutions included. */
void 
print_variable_expand (variable_t *p_v)
{
  printf("%s:%lu %s = %s\n", 
	 p_v->fileinfo.filenm, p_v->fileinfo.lineno,
	 p_v->name, variable_expand(p_v->value));
}

/*! Show a command before executing it. */
extern void 
print_target_prefix (const char *p_name) 
{
  printf(" %s", p_name);
  if (makelevel != 0) {
    printf ("[%u]", makelevel);
  }
}

/*! Show a command before executing it. */
extern void 
print_file_target_prefix (const file_t *p_target) 
{
  print_floc_prefix(&(p_target->floc));
  print_target_prefix(p_target->name);
}

/*! Show a command before executing it. */
extern void 
print_floc_prefix (const floc_t *p_floc) 
{
  if (!basename_filenames) 
    printf("%s/", starting_directory);
  printf("%s:%lu", p_floc->filenm, p_floc->lineno);
}

/*! Show a command before executing it. */
extern debug_return_t
print_child_cmd (child_t *p_child, target_stack_node_t *p)
{
  debug_return_t rc = continue_execution;

  if (!p_child) return continue_execution;

  if (tracing) {
    print_floc_prefix(&(p->p_target->floc));
    print_target_prefix(p_child->file->name);
    printf("\n");
  }

  if (debugger_stepping || p_child->file->tracing) {
    rc=enter_debugger(p, p_child->file, 0);
  }

  return rc;
}

/*! Display common prefix message output file target. */
extern void 
print_target_stack (target_stack_node_t *p, int pos)
{
  unsigned int i=0;
  printf("\n");
  for ( ; p ; p = p->p_parent ) {
    const floc_t *p_floc = &(p->p_target->floc);
    if (p_floc->filenm) {
      if (pos != -1) {
	printf("%s", (i == pos) ? "=>" : "  ");
      }
      printf ("#%u  %s at ", i, p->p_target->name);
      print_floc_prefix(p_floc);
      printf ("\n");
    }
    i++;
  }
}