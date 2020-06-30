/* Copyright (C) 2016 Alexander Shishenko <alex@shishenko.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#pragma once
#include <QObject>
#include <set>

#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class RemoteFolder;
class ChunkStorage;

class Uploader : public QObject {
  Q_OBJECT
  LOG_SCOPE("Uploader");

 public:
  Uploader(ChunkStorage* chunk_storage, QObject* parent);

  void broadcast_chunk(QList<RemoteFolder*> remotes, const QByteArray& ct_hash);

  /* Message handlers */
  void handle_interested(RemoteFolder* remote);
  void handle_not_interested(RemoteFolder* remote);

  void handle_block_request(RemoteFolder* remote, const QByteArray& ct_hash, uint32_t offset, uint32_t size) noexcept;

 private:
  ChunkStorage* chunk_storage_;

  QByteArray get_block(const QByteArray& ct_hash, uint32_t offset, uint32_t size);
};

}  // namespace librevault
