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
#include "Common/DynamicBuffer.h"
#include "Common/FileUtils.h"
#include "Common/Logger.h"
#include "Common/System.h"
#include "Common/Usage.h"

#include "Hypertable/Lib/CompressorFactory.h"
#include "Hypertable/Lib/BlockCompressionHeaderCommitLog.h"
#include "Hypertable/Lib/Types.h"

using namespace Hypertable;

namespace {
  const char *usage[] = {
    "usage: compressor_test <type>",
    "",
    "Validates a block compressor.  The type of compressor to validate",
    "is specified by the <type> argument which can be one of:",
    "",
    "none",
    "zlib",
    "lzo",
    "quicklz",
    "",
    0
  };
}

const char MAGIC[12] = { '-','-','-','-','-','-','-','-','-','-','-','-' };

int main(int argc, char **argv) {
  off_t len;
  DynamicBuffer input(0);
  DynamicBuffer output1(0);
  DynamicBuffer output2(0);
  BlockCompressionCodec *compressor;

  BlockCompressionHeaderCommitLog header(MAGIC, 0);

  if (argc == 1 || !strcmp(argv[1], "--help"))
    Usage::dump_and_exit(usage);

  System::initialize(System::locate_install_dir(argv[0]));

  compressor = CompressorFactory::create_block_codec(argv[1]);

  if (!compressor)
    return 1;

  if ((input.base = (uint8_t *)FileUtils::file_to_buffer("./good-schema-1.xml", &len)) == 0) {
    HT_ERROR("Problem loading './good-schema-1.xml'");
    return 1;
  }
  input.ptr = input.base + len;

  try {
    compressor->deflate(input, output1, header);
    compressor->inflate(output1, output2, header);
  }
  catch (Exception &e) {
    HT_ERROR_OUT << e << HT_END;
    return 1;
  }

  if (input.fill() != output2.fill()) {
    HT_ERRORF("Input length (%ld) does not match output length (%ld) after lzo codec", input.fill(), output2.fill());
    return 1;
  }

  if (memcmp(input.base, output2.base, input.fill())) {
    HT_ERROR("Input does not match output after lzo codec");
    return 1;
  }

  // this should not compress ...

  memcpy(input.base, "foo", 3);
  input.ptr = input.base + 3;

  try {
    compressor->deflate(input, output1, header);
    compressor->inflate(output1, output2, header);
  }
  catch (Exception &e) {
    HT_ERROR_OUT << e << HT_END;
    return 1;
  }

  if (input.fill() != output2.fill()) {
    HT_ERRORF("Input length (%ld) does not match output length (%ld) after lzo codec", input.fill(), output2.fill());
    return 1;
  }

  if (memcmp(input.base, output2.base, input.fill())) {
    HT_ERROR("Input does not match output after lzo codec");
    return 1;
  }

  return 0;
}
