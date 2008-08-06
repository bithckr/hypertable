/**
 * Copyright (C) 2007 Doug Judd (Zvents, Inc.)
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
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

#ifndef HYPERTABLE_DFSBROKER_CLIENT_H
#define HYPERTABLE_DFSBROKER_CLIENT_H

#include "Common/Mutex.h"
#include "Common/HashMap.h"
#include "Common/Properties.h"

#include "AsyncComm/Comm.h"
#include "AsyncComm/DispatchHandlerSynchronizer.h"
#include "AsyncComm/ConnectionManager.h"

#include "Hypertable/Lib/Filesystem.h"

#include "ClientBufferedReaderHandler.h"

#include "Protocol.h"


namespace Hypertable { namespace DfsBroker {

    /** Proxy class for DFS broker.  As specified in the general contract for
     * a Filesystem, commands that operate on the same file descriptor
     * are serialized by the underlying filesystem.  In other words, if you issue
     * three asynchronous commands, they will get carried out and their responses
     * will come back in the same order in which they were issued.
     */
    class Client : public Filesystem {
    public:

      virtual ~Client() { return; }

      /** Constructor with explicit values.  Connects to the DFS broker at the
       * address given by the addr argument and uses the timeout argument for
       * the request timeout values.
       *
       * @param conn_manager_ptr smart pointer to connection manager
       * @param addr address of DFS broker to connect to
       * @param timeout timeout value to use in requests
       */
      Client(ConnectionManagerPtr &conn_manager_ptr, struct sockaddr_in &addr, time_t timeout);

      /** Constructor with Properties object.  The following properties are read
       * to determine the location of the broker and the request timeout value:
       * <pre>
       * DfsBroker.port
       * DfsBroker.host
       * DfsBroker.timeout
       * </pre>
       *
       * @param conn_manager_ptr smart pointer to connection manager
       * @param props_ptr smart pointer to properties object
       */
      Client(ConnectionManagerPtr &conn_manager_ptr, PropertiesPtr &props_ptr);

      /** Constructor without connection manager.
       *
       * @param comm pointer to the Comm object
       * @param addr remote address of already connected DfsBroker
       * @param timeout timeout value to use in requests
       */
      Client(Comm *comm, struct sockaddr_in &addr, time_t timeout);

      /** Convenient contructor for dfs testing
       *
       * @param host - dfs hostname
       * @param port - dfs port
       * @param timeout - timeout for requests
       */
      Client(const char *host, int port, time_t timeout);

      /** Waits up to max_wait_secs for a connection to be established with the DFS
       * broker.
       *
       * @param max_wait_secs maximum amount of time to wait
       * @return true if connected, false otherwise
       */
      bool wait_for_connection(long max_wait_secs) {
        if (m_conn_mgr)
          return m_conn_mgr->wait_for_connection(m_addr, max_wait_secs);
        return true;
      }

      virtual void open(const String &name, DispatchHandler *handler);
      virtual int open(const String &name);
      virtual int open_buffered(const String &name, uint32_t buf_size,
                                uint32_t outstanding, uint64_t start_offset=0,
                                uint64_t end_offset=0);

      virtual void create(const String &name, bool overwrite,
                          int32_t bufsz, int32_t replication,
                          int64_t blksz, DispatchHandler *handler);
      virtual int create(const String &name, bool overwrite, int32_t bufsz,
                         int32_t replication, int64_t blksz);

      virtual void close(int32_t fd, DispatchHandler *handler);
      virtual void close(int32_t fd);

      virtual void read(int32_t fd, size_t amount, DispatchHandler *handler);
      virtual size_t read(int32_t fd, void *dst, size_t amount);

      virtual void append(int32_t fd, StaticBuffer &buffer, uint32_t flags,
                          DispatchHandler *handler);
      virtual size_t append(int32_t fd, StaticBuffer &buffer,
                            uint32_t flags = 0);

      virtual void seek(int32_t fd, uint64_t offset, DispatchHandler *handler);
      virtual void seek(int32_t fd, uint64_t offset);

      virtual void remove(const String &name, DispatchHandler *handler);
      virtual void remove(const String &name, bool force = true);

      virtual void length(const String &name, DispatchHandler *handler);
      virtual int64_t length(const String &name);

      virtual void pread(int32_t fd, size_t len, uint64_t offset,
                         DispatchHandler *handler);
      virtual size_t pread(int32_t fd, void *dst, size_t len, uint64_t offset);

      virtual void mkdirs(const String &name, DispatchHandler *handler);
      virtual void mkdirs(const String &name);

      virtual void flush(int32_t fd, DispatchHandler *handler);
      virtual void flush(int32_t fd);

      virtual void rmdir(const String &name, DispatchHandler *handler);
      virtual void rmdir(const String &name, bool force = true);

      virtual void readdir(const String &name, DispatchHandler *handler);
      virtual void readdir(const String &name, std::vector<String> &listing);

      virtual void exists(const String &name, DispatchHandler *handler);
      virtual bool exists(const String &name);

      virtual void rename(const String &src, const String &dst,
                          DispatchHandler *handler);
      virtual void rename(const String &src, const String &dst);

      /** Checks the status of the DFS broker.  Issues a status command and waits
       * for it to return.
       */
      void status();

      /** Shuts down the DFS broker.  Issues a shutdown command to the DFS broker.
       * If the flag is set to Protocol::SHUTDOWN_FLAG_IMMEDIATE, then the
       * broker will call exit(0) directly from the I/O reactor thread.  Otherwise,
       * a shutdown command will get added to the broker's application queue, allowing
       * the shutdown to be handled more gracefully.
       *
       * @param flags controls how broker gets shut down
       * @param handler response handler
       */
      void shutdown(uint16_t flags, DispatchHandler *handler);

      Protocol *get_protocol_object() { return &m_protocol; }

      /** Gets the configured request timeout value.
       *
       * @return timeout value in seconds
       */
      time_t get_timeout() { return m_timeout; }

    private:

      /** Sends a message to the DFS broker.
       *
       * @param cbp message to send
       * @param handler response handler
       */
      void send_message(CommBufPtr &cbp, DispatchHandler *handler);

      typedef hash_map<uint32_t, ClientBufferedReaderHandler *>
          BufferedReaderMap;

      Mutex                 m_mutex;
      Comm                 *m_comm;
      ConnectionManagerPtr  m_conn_mgr;
      struct sockaddr_in    m_addr;
      time_t                m_timeout;
      Protocol              m_protocol;
      BufferedReaderMap     m_buffered_reader_map;
    };

    typedef intrusive_ptr<Client> ClientPtr;

}} // namespace Hypertable::DfsBroker


#endif // HYPERTABLE_DFSBROKER_CLIENT_H

