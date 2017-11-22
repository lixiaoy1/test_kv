#include "KeyValueDB.h"
#include "RocksDB.h"

KeyValueDB* KeyValueDB::create(const string& type)
{
  if (type == "rocksdb") {
    return new RocksDB();
  }
  return NULL;
}
