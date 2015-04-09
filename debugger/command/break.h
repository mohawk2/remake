/* 
Set a breakpoint at a target.  With a target name, set a break before
running commands of that target.  Without argument, list all breaks.
*/
/* 
Copyright (C) 2004, 2005, 2007, 2008, 2009, 2011 R. Bernstein 
<rocky@gnu.org>
This file is part of GNU Make (remake variant).

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
static debug_return_t 
dbg_cmd_break (char *psz_args)
{
  if (!psz_args || !*psz_args) {
    list_breakpoints();
    return debug_readloop;
  } else {
    char *psz_target = get_word(&psz_args);
    char *psz_break_type; 
    file_t *p_target;
    unsigned int i_brkpt_mask = BRK_NONE;

    /** FIXME: DRY with code in continue.h **/
    if (p_stack && p_stack->p_target) {
      unsigned int u_lineno=0;
      f2l_entry_t entry_type;
      if (get_uint(psz_target, &u_lineno, false)) {
          p_target = target_for_file_and_line(p_stack->p_target->floc.filenm,
                                              u_lineno, &entry_type);
          if (F2L_TARGET == entry_type) {
            if (!p_target) {
              dbg_errmsg("Can't find target or pattern on line %s.\n" 
                         "Use 'info lines' to get a list of breakpoint lines.", 
                         psz_target);
              return debug_cmd_error;
            }
          } else {
            dbg_errmsg("No support of breakpoints on target patterns yet.");
            return debug_cmd_error;
          }
      } else
        p_target = 
          lookup_file(variable_expand_set(psz_target, 
                                            p_stack->p_target->variables));
    } else {
      p_target = lookup_file(psz_target);
    }

    if (!p_target) {
	dbg_errmsg("Can't find target %s; breakpoint not set.", psz_target);
	return debug_cmd_error;
    }

    /* FIXME: Combine with code in continue. */
    if (!(psz_args && *psz_args))
      i_brkpt_mask = BRK_ALL;
    else {
      while ((psz_break_type = get_word(&psz_args))) {
        if (!(psz_break_type && *psz_break_type)) break;
        i_brkpt_mask |= get_brkpt_option(psz_break_type) ;
      }
    }
    add_breakpoint(p_target, i_brkpt_mask);
  }

  return debug_readloop;
};

static void
dbg_cmd_break_init(unsigned int c) 
{
  short_command[c].func = &dbg_cmd_break;
  short_command[c].use  = _("break [TARGET|LINENUM] [all|run|prereq|end]*");
  short_command[c].doc  = _("Set a breakpoint at a target.\n"
"With a target name or a line number, set a break before running commands\n"
"of that target or line number.  Without argument, list all breakpoints.\n"
"There are 3 place where one may want to stop at and that name can\n"
"be given as a last option. The stopping points are:\n"
" - before prerequisite checking (prereq)\n"
" - after prerequisite checking but before running commands (run)\n"
" - after target is complete (end)\n"
"\n"
"To see a list of targets run \"info targets\"\n"
"See also \"continue\".\n");
}

/* 
 * Local variables:
 * eval: (c-set-style "gnu")
 * indent-tabs-mode: nil
 * End:
 */
