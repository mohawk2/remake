/* 
Copyright (C) 2004 Free Software Foundation, Inc.
This file is part of GNU Make.

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

/* debugger command interface. */

#include "print.h"
#include "dbg_cmd.h"
#include "debug.h"
#include "expand.h"

#ifdef HAVE_READLINE
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

/* Common debugger command function prototype */
typedef debug_return_t (*dbg_cmd_t) (char *);

static floc_t *p_target_loc = NULL;
static char   *psz_target_name = NULL;
static int i_stack_pos = 0;

/* A structure which contains information on the commands this program
   can understand. */

typedef struct {
  dbg_cmd_t func;               /* Function to call to do the job. */
  const char *doc;		/* Documentation for this function.  */
  const char *use;		/* short command usage.  */
} short_cmd_t;

typedef struct {
  const char *long_name;	/* long name of command. */
  const char short_name;	/* Index into short_cmd array. */
} long_cmd_t;

static debug_return_t com_show_var(char *psz_arg, int expand);
static debug_return_t com_set_var (char *psz_arg, int expand);

static debug_return_t com_break           (char *psz_arg);
static debug_return_t com_continue        (char *psz_arg);
static debug_return_t com_delete          (char *psz_arg);
static debug_return_t com_frame           (char *psz_arg);
static debug_return_t com_help             (char *psz_arg);
#ifdef HISTORY_STUFF
static debug_return_t com_history          (char *psz_arg);
#endif
static debug_return_t com_print            (char *psz_arg);
static debug_return_t com_quit             (char *psz_arg);
static debug_return_t com_restart          (char *psz_arg);
static debug_return_t com_set_var_expand   (char *psz_arg);
static debug_return_t com_set_var_noexpand (char *psz_arg);
static debug_return_t com_shell            (char *psz_arg);
static debug_return_t com_show_stack       (char *psz_arg);
static debug_return_t com_show_var_expand  (char *psz_arg);
static debug_return_t com_skip             (char *psz_arg);
static debug_return_t com_frame_down       (char *psz_arg);
static debug_return_t com_frame_up         (char *psz_arg);
static debug_return_t com_frame_down       (char *psz_arg);
static debug_return_t com_step             (char *psz_arg);
static debug_return_t com_info             (char *psz_arg);
static debug_return_t com_trace            (char *psz_arg);

long_cmd_t commands[] = {
  { "break",    'b' },
  { "continue", 'c' },
  { "delete",   'd' },
  { "down",     'D' },
  { "examine" , 'x' },
  { "exit"    , 'q' },
  { "frame"   , 'f' },
  { "help"    , 'h' },
  { "info"    , 'i' },
  { "next"    , 'n' },
  { "print"   , 'p' },
  { "quit"    , 'q' },
  { "restart" , 'R' },
  { "set"     , '=' },
  { "setq"    , '"' },
  { "shell"   , '!' },
  { "show"    , 'i' },
  { "skip"    , 'k' },
  { "step"    , 's' },
  { "trace"   , 't' },
  { "where"   , 'T' },
  { "up"      , 'u' },
  { (char *)NULL, ' '}
};

short_cmd_t short_command[256] = { { NULL, '\0' }, };

/* This is set to 1 when we are done with the read loop */
static int done=0 ;

/* Pointer to top of current target call stack */
static target_stack_node_t *p_stack_top;

/* Pointer to current target call stack at the place we are currently
   focused on.
 */
static target_stack_node_t *p_stack;

static void 
on_off_toggle(const char *psz_arg, int *var) 
{
  if (strcmp (psz_arg, "on") == 0)
    *var = 1;
  else if (strcmp (psz_arg, "off") == 0)
    *var = 0;
  else if (strcmp (psz_arg, "toggle") == 0)
    *var = !*var;
  else 
    printf("expecting \"on\", \"off\", or \"toggle\"; got %s \n",
	   psz_arg);
}

static char *
var_to_on_off(int var) 
{
  return var ? "on " : "off";
}

static int
get_int(const char *psz_arg, int *result) 
{
  int i;
  char *endptr;
  
  if (!psz_arg || 0==strlen(psz_arg)) return 0;

  i = strtol(psz_arg, &endptr, 10);
  if (*endptr != '\0') {
    printf("expecting %s to be an integer\n", psz_arg);
    return 0;
  }
  *result = i;
  return 1;
}

static void 
cmd_initialize(void) 
{
  short_command['b'].func = &com_break;
  short_command['b'].use  = _("break *target*");
  short_command['b'].doc  = _("Set a breakpoint at a target.");

  short_command['c'].func = &com_continue;
  short_command['c'].use  = _("continue");
  short_command['c'].doc  = 
    _("Continue executing debugged Makefile until another breakpoint.");

  short_command['d'].func = &com_delete;
  short_command['d'].use  = _("delete *target*");
  short_command['d'].doc  = _("Delete target breakpoint.");

  short_command['D'].func = &com_frame_down;
  short_command['D'].use  = _("down [amount]");
  short_command['D'].doc  = 
    _("Select and print the target this one caused to be examined.\n" \
      "\tAn argument says how many targets down to go.");

  short_command['f'].func = &com_frame;
  short_command['f'].use  = _("frame *n*");
  short_command['f'].doc  = 
    _("Move target frame to *n*; In contrast to \"up\" tor \"down\",\n" \
      "\tthis sets to an absolute postion. O is the top.");

  short_command['h'].func = &com_help;
  short_command['h'].use  = _("help [command]");
  short_command['h'].doc = 
    _("Display list of commands (i.e. this help text.)\n"		\
      "\twith an command name, give only the help for that command.");

#ifdef HISTORY_STUFF
  short_command['H'].func = &com_history;
  short_command['H'].use  = _("history [num]");
  short_command['H'].doc = 
    _("Display the history of commands\n"		\
      "\twith an argument number, list that many history entries.");
#endif

  short_command['i'].func = &com_info;
  short_command['i'].use = _("info [thing]");
  short_command['i'].doc = 
    _("Show the state of thing.\n" \
      "\tIf no 'thing' is specified, show everything there is to show.\n");

  short_command['k'].func = &com_skip;
  short_command['k'].use = _("skip");
  short_command['k'].doc = 
    _("Skip execution of next command or action.\n" );

  short_command['?'].func = &com_help;
  short_command['?'].use = short_command['h'].use;
  short_command['?'].doc = short_command['h'].doc;
  
  short_command['n'].func = &com_step;
  short_command['n'].use = _("next [amount]");
  short_command['n'].doc = _("alias for step.");

  short_command['p'].func = &com_print;
  short_command['p'].use = _("print {*variable*|*target*}");
  short_command['p'].doc = 
    _("Show a variable definition or information about a target.\n" \
      "\tIf a variable name is given, the value is shown with the\n" \
      "\tvariable-reference names. Don't include $ before a variable \n" \
      "\tname. See also \"examine\".\n" \
      "\tIf a target name is given, information about target is printed." \
      );

  short_command['q'].func = &com_quit;
  short_command['q'].use = _("quit [exit-status]");
  short_command['q'].doc = 
    _("Exit make. If a numeric argument is given, it will be the exit\n"\
      "\tstatus this program reports back. Otherwise exit with status 0." \
      );

  short_command['Q'].func = &com_quit;
  short_command['Q'].use  = _("exit");
  short_command['Q'].doc  = _("alias for quit.");
  
  short_command['R'].func = &com_restart;
  short_command['R'].doc = _("Restart program.");
  short_command['R'].use = _("restart");

  short_command['s'].func = &com_step;
  short_command['s'].use = _("step [amount]");
  short_command['s'].doc = 
    _("Step execution until another stopping point is reached.\n" \
      "\tArgument N means do this N times (or until there's another\n " \
      "\treason to stop.");

  short_command['t'].func = &com_trace;
  short_command['t'].use  = _("trace [on|off|toggle]");
  short_command['t'].doc  = 
    _("toggle or set tracing on or off. With no argument "	\
      "the value is toggled.\n");
  
  short_command['T'].func = &com_show_stack;
  short_command['T'].doc  = _("Show target stack.");
  short_command['T'].use  = _("where");

  short_command['u'].func = &com_frame_up;
  short_command['u'].use  = _("up [amount]");
  short_command['u'].doc  = 
    _("Select and print target that caused this one to be examined.\n" \
      "\tAn argument says how many targets up to go.");

  short_command['x'].func = &com_show_var_expand;
  short_command['x'].use =  _("examine *string*");
  short_command['x'].doc  = 
    _("Show string with internal variables references expanded. See also \n" \
      "\t\"print\".");

  short_command['!'].func = &com_shell;
  short_command['!'].use =  _("shell *string*");
  short_command['!'].doc  = 
    _("Execute the rest of the line as a shell.");

  short_command['='].func = &com_set_var_expand;
  short_command['='].use =  _("set *variable* *value*");
  short_command['='].doc  = 
    _("Set MAKE variable to value. Variable definitions\n"
      "\tinside VALUE are expanded before assignment occurs.");

  short_command['"'].func = &com_set_var_noexpand;
  short_command['"'].use =  _("setq *variable* *value*");
  short_command['"'].doc  = 
    _("Set MAKE variable to value. Variable definitions\n"
      "\tinside VALUE are not expanded before assignment occurs.");

}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
static short_cmd_t *
find_command (const char *name)
{
  unsigned int i;

  for (i = 0; commands[i].long_name; i++)
    if (strcmp (name, commands[i].long_name) == 0)
      return (&short_command[(uint8_t) commands[i].short_name]);

  return ((short_cmd_t *)NULL);
}

/* Execute a command line. */
static debug_return_t
execute_line (char *line)
{
  int i;
  short_cmd_t *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  if (1 == strlen(word)) {
    if ( NULL != short_command[(uint8_t) word[0]].func ) 
      command = &short_command[(uint8_t) word[0]];
    else
      command = NULL;
  } else {
    command = find_command (word);
  }
  if (!command)
    {
      fprintf (stderr, "No such debugger command: %s.\n", word);
      return debug_read;
    }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
static char *
stripwhite (char *string)
{
  char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;
    
  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* Give some help info. */
static debug_return_t com_help (char *psz_arg)
{
  unsigned int i;

  if (!psz_arg || 0==strlen(psz_arg)) {
    printf ("Available commands are: \n");
    for (i = 0; commands[i].long_name; i++) {
      uint8_t s=commands[i].short_name;
      printf("  %-25s (%c):\n", 
	     short_command[s].use, commands[i].short_name);
      printf("\t%s\n\n", short_command[s].doc);
    }
  } else {
    short_cmd_t *p_command;

    if (1 == strlen(psz_arg)) {
      if ( NULL != short_command[(uint8_t)psz_arg[0]].func ) 
	p_command = &short_command[(uint8_t)psz_arg[0]];
      else
	p_command = NULL;
    } else {
      p_command = find_command (psz_arg);
    }
    if (p_command) {
      printf("  %s:\n\t%s\n", p_command->use, p_command->doc);
    } else {
      printf("Invalid command %s. Try help for a list of commands\n", 
	     psz_arg);
    }
  }
  
  return 0;
}

#if HISTORY_STUFF
/* Give some help info. */
static debug_return_t com_history (char *psz_arg)
{
  unsigned int i;

  if (!psz_arg || 0==strlen(psz_arg)) {
    ;
  }
  
}
#endif

/* Show target call stack info. */
static debug_return_t com_restart (char *psz_arg)
{
  printf("Changing directory to %s and restarting...\n", 
	 directory_before_chdir);
  chdir (directory_before_chdir);
  execvp (global_argv[0], global_argv);
  /* NOT USED: */
  return debug_read;
}

/* Show target call stack info. */
static debug_return_t com_show_stack (char *psz_arg)
{
  print_target_stack (p_stack_top, i_stack_pos);
  return debug_read;
}

/* Terminate execution. */
static debug_return_t com_quit (char *psz_arg)
{
  if (!psz_arg || 0==strlen(psz_arg)) {
    exit(0);
  } else {
    int rc;
    if (get_int(psz_arg, &rc)) 
      exit(rc);
  }
  /* NOT USED: */
  return debug_read;
}

/* Set a breakpoint. */
static debug_return_t com_break (char *psz_target)
{
  file_t *p_target;

  if (!psz_target || 0==strlen(psz_target))
    return debug_read;
  
  p_target = lookup_file (psz_target);
  if (!p_target) {
    printf("Can't find target %s; breakpoint not set.\n", psz_target);
    return debug_read;
  }

  if (p_target->tracing) {
    printf("Breakpoint already set at target %s; nothing done.\n", psz_target);
  } else {
    p_target->tracing = 1;
    printf("Breakpoint on target %s set.\n", psz_target);
  }

  return debug_read;
}

/* Delete a breakpoint. */
static debug_return_t com_delete (char *psz_target)
{
  file_t *p_target;

  if (!psz_target || 0==strlen(psz_target))
    return debug_read;
  
  p_target = lookup_file (psz_target);
  if (!p_target) {
    printf("Can't find target %s; breakpoint not cleared.\n", psz_target);
    return debug_read;
  }

  if (p_target->tracing) {
    p_target->tracing = 0;
    printf("Breakpoint on target %s cleared\n", psz_target);
  } else {
    printf("No breakpoint at target %s; nothing cleared.\n", psz_target);
  }

  return debug_read;
}

/* Continue running program. */
static debug_return_t com_continue (char *psz_arg)
{
  debugger_stepping = 0;
  return continue_execution;
}

/* Give some help info. */
static debug_return_t com_info (char *psz_arg)
{
  if (!psz_arg || 0==strlen(psz_arg)) {
    com_info("target");
    com_info("trace");
  } else {
    if (0 == strcmp (psz_arg, "target")) {
      if (p_stack_top && p_stack_top->p_target && p_stack_top->p_target->name)
	printf("target: %s\n", p_stack_top->p_target->name);
      else 
	printf("target unknown\n");
    } else if (0 == strcmp (psz_arg, "trace")) {
      printf("trace: %s\n", var_to_on_off(tracing));
    } else {
      printf("Don't know how to show %s\n", psz_arg);
    }
  }
  
  return debug_read;
}

/* Skip over next comand or action. */
static debug_return_t com_skip (char *psz_arg)
{
  return skip_execution;
}

/* Step next command. */
static debug_return_t com_step (char *psz_arg)
{

  if (!psz_arg || 0==strlen(psz_arg)) {
    debugger_stepping = 1;
    return continue_execution;
  } 
  if (get_int(psz_arg, &debugger_stepping)) 
    return continue_execution;
  else 
    return continue_execution;
}

/* Continue running program. */
static debug_return_t com_trace (char *psz_arg)
{
  if (!psz_arg || 0==strlen(psz_arg))
    on_off_toggle("toggle", &tracing) ;
  else
    on_off_toggle(psz_arg, &tracing) ;
  com_info("trace");
  return debug_read;
}

/* Show a variable definition. */
static debug_return_t com_print (char *psz_object) 
{
  if (!psz_object || 0==strlen(psz_object))
    return 0;
  else {
    file_t *p_target = lookup_file (psz_object);
    if (p_target) {
      print_target ((const void *) p_target);
    } else {
      com_show_var(psz_object, 0);
    }
  }
  return debug_read;
}

/* Set a variable. Set "expand' to 1 if you want variable 
   definitions inside the value getting passed in to be expanded
   before assigment. */
static debug_return_t com_set_var (char *psz_args, int expand) 
{
  if (!psz_args || 0==strlen(psz_args)) {
    printf("need to supply a variable name\n");
  } else {
    variable_t *p_v;
    char *psz_varname;
    unsigned int u_len=0;

    /* Isolate the variable. */
    while (*psz_args && whitespace (*psz_args))
      *psz_args++;

    psz_varname = psz_args;
    
    while (*psz_args && !whitespace (*psz_args)) {
      *psz_args++;
      u_len++;
    }

    if (*psz_args) *psz_args++ = '\0';

    while (*psz_args && whitespace (*psz_args))
      *psz_args++;

    p_v = lookup_variable (psz_varname, u_len);

    if (p_v) {
      variable_origin_t e_origin = p_v->origin;
      char *psz_value =  expand ? variable_expand(psz_args) : psz_args;
      
      define_variable_in_set(p_v->name, u_len, psz_value,
			     o_env_override, 0, NULL,
			     &(p_v->fileinfo));
      p_v->origin = e_origin;
      
      printf("Variable %s now has value '%s'\n", psz_varname,
	     psz_value);
    } else {
      printf("Can't find variable %s\n", psz_varname);
    }
  }
  return debug_read;
}

/* Set a variable definition with all variable references in the value
   part of psz_string expanded. */
static debug_return_t com_set_var_expand (char *psz_string) 
{
  com_set_var(psz_string, 1);
  return debug_read;
}

/* Set a variable definition without variable references but don't 
   expand variable references in the value part of psz_string. */
static debug_return_t com_set_var_noexpand (char *psz_string) 
{
  com_set_var(psz_string, 0);
  return debug_read;
}

/* Run a shell command. */
static debug_return_t com_shell (char *psz_varname) 
{
  system(psz_varname);
  return debug_read;
}


/* Show a variable definition. Set "expand" to 1 if you want variable
   definitions inside the displayed value expanded.
*/
static debug_return_t com_show_var (char *psz_varname, int expand) 
{
  if (!psz_varname || 0==strlen(psz_varname)) {
    printf("need to supply a variable name\n");
  } else {
    variable_t *p_v = 
      lookup_variable (psz_varname, strlen (psz_varname));
    if (p_v) {
      if (expand) 
	print_variable_expand(p_v);
      else
	print_variable(p_v);
      printf("\n");
    } else {
      if (expand)
	printf("%s\n", variable_expand(psz_varname));
      else
	printf("Can't find variable %s\n", psz_varname);
    }
  }
  return debug_read;
}

/* Show a variable definition;'t expand any variable references
   in the displayed value. */
static debug_return_t com_show_var_expand (char *psz_string) 
{
  com_show_var(psz_string, 1);
  return debug_read;
}

/* Move reported target frame postition up one. */
static debug_return_t com_frame_down (char *psz_amount) 
{
  unsigned int i=0;
  int i_amount = 1;

  if (!psz_amount || 0==strlen(psz_amount)) {
    i_amount = 1;
  } else {
    if (!get_int(psz_amount, &i_amount))
      return debug_read;
  }

  if (i_stack_pos - i_amount < 0) {
    printf("Move down by %d would be below bottom-most frame position.\n",
	   i_amount);
    return debug_read;
  }
  
  i_stack_pos -= i_amount;
  
  for ( p_stack=p_stack_top; p_stack ; p_stack = p_stack->p_parent ) {
    if (i_stack_pos == i)
      break;
    i++;
  }

  p_target_loc    = &(p_stack->p_target->floc);
  psz_target_name = p_stack->p_target->name;
  
  return debug_read;
}

/* Move reported target frame postition up one. */
static debug_return_t com_frame (char *psz_frame) 
{
  int i, i_frame;

  if (!psz_frame || 0==strlen(psz_frame)) {
    return debug_read;
  } else {
    if (!get_int(psz_frame, &i_frame))
      return debug_read;
  }

  i = i_frame + 1;
  
  for ( p_stack=p_stack_top; p_stack ; p_stack = p_stack->p_parent ) {
    i--;
    if (0 == i)
      break;
  }

  if (0 != i) {
    printf("Can't set frame to position %d; %d is the highest position.",
	   i_frame, i_frame - i);
    return debug_read;
  }

  i_stack_pos     = i_frame;
  p_target_loc    = &(p_stack->p_target->floc);
  psz_target_name = p_stack->p_target->name;
  
  return debug_read;
}

/* Move reported target frame postition up one. */
static debug_return_t com_frame_up (char *psz_amount) 
{
  unsigned int i_amount=1;
  unsigned int i = 0;
  target_stack_node_t *p=p_stack;

  if (!psz_amount || 0==strlen(psz_amount)) {
    i_amount = 1;
  } else {
    if (!get_int(psz_amount, &i_amount))
      return debug_read;
  }

  for ( ; p ; p = p->p_parent, i++ ) {
    if (i_amount == i) break;
  }

  if (p) {
    i_stack_pos    += i_amount;
    p_stack         = p;
    p_target_loc    = &(p->p_target->floc);
    psz_target_name = p->p_target->name;
  } else {
    printf("Can't move up %d - would be beyond top-most frame position.\n",
	   i_amount);
  }
  
  return debug_read;
}

#endif /* HAVE_READLINE */

#define PROMPT_LENGTH 200
#define NEST_DEPTH    ((PROMPT_LENGTH / 2) - 40)

debug_return_t
enter_debugger (target_stack_node_t *p, file_t *p_target, int err)
{
#ifdef HAVE_READLINE
  char *line, *s;
  static int i_init = 0;
  char open_depth[NEST_DEPTH];
  char close_depth[NEST_DEPTH];
  unsigned int i;
  debug_return_t debug_return = continue_execution;

  if ( debugger_stepping > 1 ) {
    /* Don't stop unless we are here from a breakpoint. But
       do decrement the step count. */
    debugger_stepping--;
    if (!p_target->tracing) return continue_execution;
  } else if (!debugger_on_error && !debugger_stepping 
	     && !p_target->tracing) return continue_execution;
  
  if (0 == i_init) {
    rl_initialize ();
    cmd_initialize();
    i_init = 1;
  }

  /* Set initial frame position reporting area: 0 is bottom. */
  p_target_loc    = NULL;
  psz_target_name = "";
  i_stack_pos      = 0;

  p_stack = p_stack_top = p;

  if (p && p->p_target) {
    p_target_loc    = &(p->p_target->floc);  
    psz_target_name = p->p_target->name;
  } else if (p_target) {
    p_target_loc    = &(p_target->floc);
    psz_target_name = p_target->name;
  }

  for (i=0; i<=makelevel && i < NEST_DEPTH; i++) {
    open_depth[i]  = '<';
    close_depth[i] = '>';
  }
  open_depth[i] = close_depth[i] = '\0';

  if (err) {
    if (-1 == err) {
      printf("\n***Entering debugger because we encountered an error.\n");
    } else {
      printf("\n***Entering debugger because we encountered a fatal error.\n");
      printf("***Exiting the debugger will exit make with exit code %d.\n", 
	     err);
    }
  }

  /* Loop reading and executing lines until the user quits. */
  for ( ; done == 0; )
    {
      char prompt[PROMPT_LENGTH];
      
      if (p_target_loc) {
	printf ("\n");
	print_floc_prefix(p_target_loc);
	printf (": %s\n", psz_target_name);
      }
      
      snprintf(prompt, PROMPT_LENGTH, "makedb%s%d%s ", 
	       open_depth, where_history(), close_depth);
      
      line = readline (prompt);

      if (!line) {
	printf("\n");
        break;
      }

      if (0==strlen(line)) {
	com_step("");
      }
      
      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (line);

      if (*s)
        {
          add_history (s);
          if ( (debug_return=execute_line(s)) != debug_read ) break;
        }

      free (line);
    }

  return debug_return;
#else 
  ;
#endif /* HAVE_READLINE */
}