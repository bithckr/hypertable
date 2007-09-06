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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef HYPERTABLE_COMMANDLOADRANGE_H
#define HYPERTABLE_COMMANDLOADRANGE_H

#include "Common/InteractiveCommand.h"

#include "Hypertable/Lib/MasterClient.h"
#include "Hypertable/Lib/RangeServerClient.h"
#include "Hyperspace/HyperspaceClient.h"

namespace hypertable {

  class CommandLoadRange : public InteractiveCommand {
  public:
    CommandLoadRange(MasterClient *masterClient, RangeServerClient *rsClient, HyperspaceClient *hyperspace, struct sockaddr_in &addr) : mMaster(masterClient), mRangeServer(rsClient), mHyperspace(hyperspace), mAddr(addr) { return; }
    virtual const char *CommandText() { return "load range"; }
    virtual const char **Usage() { return msUsage; }
    virtual int run();

  private:
    static const char *msUsage[];

    MasterClient      *mMaster;
    RangeServerClient *mRangeServer;
    HyperspaceClient  *mHyperspace;
    struct sockaddr_in mAddr;
  };

}

#endif // HYPERTABLE_COMMANDLOADRANGE_H