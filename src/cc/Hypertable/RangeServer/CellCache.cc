/** -*- c++ -*-
 * Copyright (C) 2008 Doug Judd (Zvents, Inc.)
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
#include <cassert>
#include <iostream>

#include "Common/Logger.h"

#include "Hypertable/Lib/Key.h"

#include "CellCache.h"
#include "CellCacheScanner.h"
#include "Global.h"

using namespace Hypertable;
using namespace std;


const uint32_t CellCache::ALLOC_BIT_MASK  = 0x80000000;
const uint32_t CellCache::OFFSET_BIT_MASK = 0x7FFFFFFF;


//#define STAT


CellCache::~CellCache() {
  uint64_t mem_freed = 0;
  uint32_t offset;
  uint8_t *ptr;
#ifdef STAT
  uint32_t skipped = 0;
#endif

  for (CellMap::iterator iter = m_cell_map.begin(); iter != m_cell_map.end(); iter++) {
    if (((*iter).second & ALLOC_BIT_MASK) == 0) {
      ptr = (uint8_t *)(*iter).first.ptr;
      offset = (*iter).second & OFFSET_BIT_MASK;
      mem_freed += offset + ByteString(ptr + offset).length();
      delete [] ptr;
    }
#ifdef STAT
    else
      skipped++;
#endif
  }

#ifdef STAT
  cout << flush;
  cout << "STAT[~CellCache]\tmemory freed\t" << mem_freed << endl;
  cout << "STAT[~CellCache]\tentries skipped\t" << skipped << endl;
  cout << "STAT[~CellCache]\tentries total\t" << m_cell_map.size() << endl;
#endif

  Global::memory_tracker.remove_memory(mem_freed);
  Global::memory_tracker.remove_items(m_cell_map.size());

}



/**
 */
int CellCache::add(const ByteString key, const ByteString value, int64_t real_timestamp) {
  ByteString new_key;
  uint8_t *ptr;
  size_t key_len = key.length();
  size_t total_len = key_len + value.length();

  (void)real_timestamp;

  new_key.ptr = ptr = new uint8_t [total_len];

  memcpy(ptr, key.ptr, key_len);
  ptr += key_len;

  value.write(ptr);

  if (! m_cell_map.insert(CellMap::value_type(new_key, key_len)).second) {
    m_collisions++;
    HT_WARNF("Collision detected key insert (row = %s)", new_key.str());
    delete [] new_key.ptr;
  }
  else {
    m_memory_used += total_len;
    if (key.ptr[key_len - 9] <= FLAG_DELETE_CELL)
      m_deletes++;
  }

  return 0;
}



const char *CellCache::get_split_row() {
  assert(!"CellCache::get_split_row not implemented!");
  return 0;
}



void CellCache::get_split_rows(std::vector<std::string> &split_rows) {
  boost::mutex::scoped_lock lock(m_mutex);
  if (m_cell_map.size() > 2) {
    CellMap::const_iterator iter = m_cell_map.begin();
    size_t i=0, mid = m_cell_map.size() / 2;
    for (i=0; i<mid; i++)
      iter++;
    split_rows.push_back((*iter).first.str());
  }
}



void CellCache::get_rows(std::vector<std::string> &rows) {
  boost::mutex::scoped_lock lock(m_mutex);
  const char *row, *last_row = "";
  for (CellMap::const_iterator iter = m_cell_map.begin(); iter != m_cell_map.end(); iter++) {
    row = (*iter).first.str();
    if (strcmp(row, last_row)) {
      rows.push_back(row);
      last_row = row;
    }
  }
}



CellListScanner *CellCache::create_scanner(ScanContextPtr &scan_ctx) {
  CellCachePtr cellcache(this);
  return new CellCacheScanner(cellcache, scan_ctx);
}



/**
 * This must be called with the cell cache locked
 */
CellCache *CellCache::slice_copy(int64_t timestamp) {
  Key key;
#ifdef STAT
  uint64_t dropped = 0;
#endif

  CellCachePtr child_ptr = new CellCache();

  for (CellMap::iterator iter = m_cell_map.begin(); iter != m_cell_map.end(); iter++) {

    if (!key.load((*iter).first)) {
      HT_ERROR("Problem deserializing key/value pair");
      continue;
    }

    if (key.timestamp > timestamp) {
      child_ptr->m_cell_map.insert(CellMap::value_type((*iter).first, (*iter).second));
      (*iter).second |= ALLOC_BIT_MASK;  // mark this entry in the "old" map so it doesn't get deleted
    }
#ifdef STAT
    else
      dropped++;
#endif
  }

#ifdef STAT
  cout << flush;
  cout << "STAT[slice_copy]\tdropped\t" << dropped << endl;
#endif

  Global::memory_tracker.add_items(child_ptr->m_cell_map.size());

  m_children.push_back(child_ptr);

  return child_ptr.get();
}


/**
 *
 */
CellCache *CellCache::purge_deletes() {
  Key key_comps;
  size_t len;
  bool          delete_present = false;
  DynamicBuffer deleted_row(0);
  int64_t       deleted_row_timestamp = 0;
  DynamicBuffer deleted_column_family(0);
  int64_t       deleted_column_family_timestamp = 0;
  DynamicBuffer deleted_cell(0);
  int64_t       deleted_cell_timestamp = 0;
  CellMap::iterator iter;

  HT_INFO("Purging deletes from CellCache");

  CellCachePtr child_ptr = new CellCache();

  iter = m_cell_map.begin();

  while (iter != m_cell_map.end()) {

    if (!key_comps.load((*iter).first)) {
      HT_ERROR("Problem deserializing key/value pair");
      iter++;
      continue;
    }

    if (key_comps.flag == FLAG_INSERT) {

      if (delete_present) {
        if (deleted_cell.fill() > 0) {
          len = (key_comps.column_qualifier - key_comps.row) + strlen(key_comps.column_qualifier) + 1;
          if (deleted_cell.fill() == len && !memcmp(deleted_cell.base, key_comps.row, len)) {
            if (key_comps.timestamp > deleted_cell_timestamp) {
              child_ptr->m_cell_map.insert(CellMap::value_type((*iter).first, (*iter).second));
              (*iter).second |= ALLOC_BIT_MASK;  // mark this entry in the "old" map so it doesn't get deleted
            }
            iter++;
            continue;
          }
          deleted_cell.clear();
        }
        if (deleted_column_family.fill() > 0) {
          len = key_comps.column_qualifier - key_comps.row;
          if (deleted_column_family.fill() == len && !memcmp(deleted_column_family.base, key_comps.row, len)) {
            if (key_comps.timestamp > deleted_column_family_timestamp) {
              child_ptr->m_cell_map.insert(CellMap::value_type((*iter).first, (*iter).second));
              (*iter).second |= ALLOC_BIT_MASK;  // mark this entry in the "old" map so it doesn't get deleted
            }
            iter++;
            continue;
          }
          deleted_column_family.clear();
        }
        if (deleted_row.fill() > 0) {
          len = strlen(key_comps.row) + 1;
          if (deleted_row.fill() == len && !memcmp(deleted_row.base, key_comps.row, len)) {
            if (key_comps.timestamp > deleted_row_timestamp) {
              child_ptr->m_cell_map.insert(CellMap::value_type((*iter).first, (*iter).second));
              (*iter).second |= ALLOC_BIT_MASK;  // mark this entry in the "old" map so it doesn't get deleted
            }
            iter++;
            continue;
          }
          deleted_row.clear();
        }
        delete_present = false;
      }
      child_ptr->m_cell_map.insert(CellMap::value_type((*iter).first, (*iter).second));
      (*iter).second |= ALLOC_BIT_MASK;  // mark this entry in the "old" map so it doesn't get deleted
      iter++;
    }
    else {
      if (key_comps.flag == FLAG_DELETE_ROW) {
        len = strlen(key_comps.row) + 1;
        if (delete_present && deleted_row.fill() == len && !memcmp(deleted_row.base, key_comps.row, len)) {
          if (deleted_row_timestamp < key_comps.timestamp)
            deleted_row_timestamp = key_comps.timestamp;
        }
        else {
          deleted_row.set(key_comps.row, len);
          deleted_row_timestamp = key_comps.timestamp;
          delete_present = true;
        }
      }
      else if (key_comps.flag == FLAG_DELETE_COLUMN_FAMILY) {
        len = key_comps.column_qualifier - key_comps.row;
        if (delete_present && deleted_column_family.fill() == len && !memcmp(deleted_column_family.base, key_comps.row, len)) {
          if (deleted_column_family_timestamp < key_comps.timestamp)
            deleted_column_family_timestamp = key_comps.timestamp;
        }
        else {
          deleted_column_family.set(key_comps.row, len);
          deleted_column_family_timestamp = key_comps.timestamp;
          delete_present = true;
        }
      }
      else if (key_comps.flag == FLAG_DELETE_CELL) {
        len = (key_comps.column_qualifier - key_comps.row) + strlen(key_comps.column_qualifier) + 1;
        if (delete_present && deleted_cell.fill() == len && !memcmp(deleted_cell.base, key_comps.row, len)) {
          if (deleted_cell_timestamp < key_comps.timestamp)
            deleted_cell_timestamp = key_comps.timestamp;
        }
        else {
          deleted_cell.set(key_comps.row, len);
          deleted_cell_timestamp = key_comps.timestamp;
          delete_present = true;
        }
      }
      iter++;
    }
  }

  Global::memory_tracker.add_items(child_ptr->m_cell_map.size());

  m_children.push_back(child_ptr);

  return child_ptr.get();
}
