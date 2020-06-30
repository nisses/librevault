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

#include "SignedMeta.h"
#include "util/SQLiteWrapper.h"
#include "util/blob.h"
#include "util/log.h"

namespace librevault {

class FolderParams;
class StateCollector;

class Index : public QObject {
  Q_OBJECT
  LOG_SCOPE("Index");
 signals:
  void metaAdded(SignedMeta meta);
  void metaAddedExternal(SignedMeta meta);

 public:
  Index(const FolderParams& params, StateCollector* state_collector, QObject* parent);

  /* Meta manipulators */
  bool haveMeta(const Meta::PathRevision& path_revision) noexcept;
  SignedMeta getMeta(const Meta::PathRevision& path_revision);
  SignedMeta getMetaByPathId(const QByteArray& path_id);
  QList<SignedMeta> getAllMeta();
  QList<SignedMeta> getExistingMeta();
  QList<SignedMeta> getIncompleteMeta();
  void putMeta(const SignedMeta& signed_meta, bool fully_assembled = false);

  bool putAllowed(const Meta::PathRevision& path_revision) noexcept;

  void setAssembled(const QByteArray& path_id);
  bool isAssembledChunk(const QByteArray& ct_hash);
  QPair<quint32, QByteArray> getChunkSizeIv(const QByteArray& ct_hash);

  /* Properties */
  QList<SignedMeta> containingChunk(const QByteArray& ct_hash);

 private:
  const FolderParams& params_;
  StateCollector* state_collector_;

  // Better use SOCI library ( https://github.com/SOCI/soci ). My "reinvented wheel" isn't stable enough.
  std::unique_ptr<SQLiteDB> db_;

  QList<SignedMeta> queryMeta(const std::string& sql,
                              const std::map<QString, SQLValue>& values = std::map<QString, SQLValue>());
  void wipe();

  void notifyState();
};

}  // namespace librevault
