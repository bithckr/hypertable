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

#ifndef MASTER_PROTOCOL_H
#define MASTER_PROTOCOL_H

#include "AsyncComm/CommBuf.h"
#include "AsyncComm/Event.h"
#include "AsyncComm/Protocol.h"

#include "Hypertable/Lib/Types.h"


namespace Hypertable {

  class MasterProtocol : public Protocol {

  public:

    static const short COMMAND_CREATE_TABLE    = 0;
    static const short COMMAND_GET_SCHEMA      = 1;
    static const short COMMAND_STATUS          = 2;
    static const short COMMAND_REGISTER_SERVER = 3;
    static const short COMMAND_REPORT_SPLIT    = 4;
    static const short COMMAND_DROP_TABLE      = 5;
    static const short COMMAND_SHUTDOWN        = 6;
    static const short COMMAND_RENAME_TABLE    = 7;
    static const short COMMAND_MAX             = 8;

    static const char *m_command_strings[];

    static CommBuf *create_create_table_request(const char *tablename, const char *schemastr);
    
    static CommBuf *create_rename_table_request(const char *old_tablename, const char *new_tablename);

    static CommBuf *create_get_schema_request(const char *tablename);

    static CommBuf *create_status_request();

    static CommBuf *create_register_server_request(const String &location);

    static CommBuf *create_report_split_request(TableIdentifier *table, RangeSpec &range, const char *transfer_log_dir, uint64_t soft_limit);

    static CommBuf *create_drop_table_request(const char *table_name, bool if_exists);

    static CommBuf *create_shutdown_request();

    virtual const char *command_text(short command);

  };

}

#endif // MASTER_PROTOCOL_H
