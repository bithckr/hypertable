/**
 * Copyright (C) 2008 Luke Lu (Zvents, Inc.)
 * 
 * This file is part of Hypertable.
 * 
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "Common/Compat.h"
#include "RangeServerMetaLogEntries.h"
//#include "MasterMetaLogEntries.h"

#define HT_R_CAST_AND_OUTPUT(_type_, _ep_, _out_) \
  _type_ *sp = (_type_ *)_ep_; _out_ <<'{'<< #_type_ <<": table='" \
      << sp->table.name <<"' range="<< sp->range

namespace Hypertable {

using namespace MetaLogEntryFactory;
using namespace RangeServerTxn;

std::ostream &
operator<<(std::ostream &out, const MetaLogEntry *ep) {
  switch (ep->get_type()) {
    case RS_SPLIT_START: {
      HT_R_CAST_AND_OUTPUT(SplitStart, ep, out) <<" split_off="<< sp->split_off
          <<" state="<< sp->range_state <<'}';
    } break;
    case RS_SPLIT_SHRUNK: {
      HT_R_CAST_AND_OUTPUT(SplitShrunk, ep, out) <<'}';
    } break;
    case RS_SPLIT_DONE: {
      HT_R_CAST_AND_OUTPUT(SplitDone, ep, out) << '}';
    } break;
    case RS_RANGE_LOADED: {
      HT_R_CAST_AND_OUTPUT(RangeLoaded, ep, out)
          <<" state="<< sp->range_state <<'}';
    } break;
    case RS_MOVE_START: {
      HT_R_CAST_AND_OUTPUT(MoveStart, ep, out)
          <<" state="<< sp->range_state <<'}';
    } break;
    case RS_MOVE_PREPARED: {
      HT_R_CAST_AND_OUTPUT(MovePrepared, ep, out) <<'}';
    } break;
    case RS_MOVE_DONE: {
      HT_R_CAST_AND_OUTPUT(MoveDone, ep, out) <<'}';
    } break;
    default: out <<"{UnknownEntry("<< ep->get_type() <<")}";
  }
  return out;
}

} // namespace Hypertable
