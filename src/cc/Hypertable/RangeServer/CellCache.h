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

#ifndef HYPERTABLE_CELLCACHE_H
#define HYPERTABLE_CELLCACHE_H

#include <map>

#include "Common/Mutex.h"

#include "CellListScanner.h"
#include "CellList.h"

namespace Hypertable {

  /**
   * Represents  a sorted list of key/value pairs in memory.
   * All updates get written to the CellCache and later get "compacted"
   * into a CellStore on disk.
   */
  class CellCache : public CellList {

  public:
    CellCache() : CellList(), m_memory_used(0), m_deletes(0), m_collisions(0) { return; }
    virtual ~CellCache();

    /**
     * Adds a key/value pair to the CellCache.  This method assumes that
     * the CellCache has been locked by a call to #lock.  Copies of
     * the key and value are created and inserted into the underlying cell map
     *
     * @param key key to be inserted
     * @param value value to inserted
     * @param real_timestamp real commit log timestamp
     * @return zero
     */
    virtual int add(const ByteString key, const ByteString value, int64_t real_timestamp);

    virtual const char *get_split_row();

    virtual void get_split_rows(std::vector<std::string> &split_rows);

    virtual void get_rows(std::vector<std::string> &rows);

    /**
     * Creates a CellCacheScanner object that contains an shared pointer (intrusive_ptr)
     * to this CellCache.
     */
    virtual CellListScanner *create_scanner(ScanContextPtr &scan_ctx);

    void lock()   { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    size_t size() { return m_cell_map.size(); }

    /**
     * Makes a copy of this CellCache, but only includes the key/value
     * pairs that have a timestamp greater than the timestamp argument.
     * This method is called after a compaction to drop the key/value
     * pairs that were compacted to disk.
     *
     * @param timestamp cutoff timestamp
     * @return The new "sliced" copy of the cell cache
     */
    CellCache *slice_copy(int64_t timestamp);

    /**
     * Purges all deleted pairs along with the corresponding delete entries.
     *
     * @return The new "purged" copy of the cell cache
     */
    CellCache *purge_deletes();

    /**
     * Returns the amount of memory used by the CellCache.  This is the summation
     * of the lengths of all the keys and values in the map.
     */
    uint64_t memory_used() {
      ScopedLock lock(m_mutex);
      return m_memory_used + (m_cell_map.size() * sizeof(CellMap::value_type));
    }

    uint32_t get_collision_count() { return m_collisions; }

    uint32_t get_delete_count() { return m_deletes; }

    friend class CellCacheScanner;

  protected:
    typedef std::map<const ByteString, uint32_t, LtByteString> CellMap;

    static const uint32_t ALLOC_BIT_MASK;
    static const uint32_t OFFSET_BIT_MASK;

    Mutex              m_mutex;
    std::vector<CellListPtr> m_children;
    CellMap            m_cell_map;
    uint64_t           m_memory_used;
    uint32_t           m_deletes;
    uint32_t           m_collisions;
  };

  typedef boost::intrusive_ptr<CellCache> CellCachePtr;

}

#endif // HYPERTABLE_CELLCACHE_H

