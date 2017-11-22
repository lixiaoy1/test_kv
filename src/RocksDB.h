#ifndef ROCKSDB_H
#define ROCKSDB_H

#include "KeyValueDB.h"
#include "rocksdb/db.h"
#include "rocksdb/merge_operator.h"

class RocksDB: public KeyValueDB {
public:
  int create_and_open(const string& dir,
                      const string& options) override;
  void close() override;

  void put(const string& key, const string& value) override;
  void del(const string &key) override;
  void singleDel(const string& key) override;
  void merge(const string &key, const string& value) override;
  void rmRange(const string &start, const string &end) override;
  void get(const string &key, string &value) override;

private:
  rocksdb::DB *db;
  int tryInterpret(const string&, const string&, rocksdb::Options&);
  int ParseOptionsFromString(const string&, rocksdb::Options&);
};
#endif
