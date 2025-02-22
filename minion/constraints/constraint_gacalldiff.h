/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */







#ifndef CONSTRAINT_GACALLDIFF_H
#define CONSTRAINT_GACALLDIFF_H

#include "alldiff_common.h"

template <typename VarArray>
AbstractConstraint* BuildCT_GACALLDIFF(const VarArray& varArray, ConstraintBlob&) {
  return new GacAlldiffConstraint<VarArray>(varArray);
}

/* JSON
{ "type": "constraint",
  "name": "gacalldiff",
  "internal_name": "CT_GACALLDIFF",
  "args": [ "read_list" ]
}
*/
#endif
