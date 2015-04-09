/* 
Copyright (C) 2005, 2011 R. Bernstein rocky@gnu.org
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

/** \file dbg_stack.h
 *
 *  \brief debugger command stack routines.
 */

#ifndef DBG_STACK_H
#define DBG_STACK_H

#include "trace.h"

extern int i_stack_pos;

/** Pointer to current target call stack at the place we are currently
   focused on.
 */
extern target_stack_node_t *p_stack;
extern floc_stack_node_t   *p_floc_stack;

extern debug_return_t dbg_adjust_frame(int i_amount, int b_absolute);

#endif /* DBG_STACK_H */
