/* Copyright (C) 2014-2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SRC_SYNCFS_FSBLOCKSTORAGE_H_
#define SRC_SYNCFS_FSBLOCKSTORAGE_H_

#include "MetaStorage.h"
#include "Indexer.h"
#include "EncFSBlockStorage.h"
#include "OpenFSBlockStorage.h"

#include "../../contrib/cryptowrappers/cryptowrappers.h"
#include "../types.h"

namespace librevault {
namespace syncfs {

class FSBlockStorage {
	// Shared DB
	std::shared_ptr<SQLiteDB> directory_db;

	// Storages
	friend class MetaStorage; std::shared_ptr<MetaStorage> meta_storage;
	friend class Indexer; std::shared_ptr<Indexer> indexer;
	friend class EncFSBlockStorage; std::shared_ptr<EncFSBlockStorage> encfs_block_storage;
	friend class OpenFSBlockStorage; std::shared_ptr<OpenFSBlockStorage> openfs_block_storage;

	// Paths
	fs::path system_path;
	fs::path system_dirname;
	fs::path open_path;

	// Encryption
	crypto::Key aes_key;
public:
	enum Errors {
		NoSuchMeta,
		NoSuchBlock,
		NotEnoughBlocks
	};

	void init_db();
public:
	FSBlockStorage(const fs::path& dirpath);
	FSBlockStorage(const fs::path& dirpath, const crypto::Key& aes_key);
	virtual ~FSBlockStorage();

	// Index manipulators
	void create_index();
	void update_index();

	void create_index_file(const fs::path& relpath);
	void update_index_file(const fs::path& relpath);
	void delete_index_file(const fs::path& relpath);

	// Block manipulators
	blob get_block(const crypto::StrongHash& block_hash);
	blob get_encblock(const crypto::StrongHash& block_hash);

	void put_block(const crypto::StrongHash& block_hash, const blob& data);
	void put_encblock(const crypto::StrongHash& block_hash, const blob& data);

	// Getters
	void set_aes_key(const crypto::Key& aes_key) {this->aes_key = aes_key;}
	const crypto::Key& get_aes_key() const {return aes_key;}
};

} /* namespace syncfs */
} /* namespace librevault */

#endif /* SRC_SYNCFS_FSBLOCKSTORAGE_H_ */