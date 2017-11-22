#ifndef KEY_VALUE_DB_H
#define KEY_VALUE_DB_H

#include <string>
#include <cassert>

using std::string;

class KeyValueDB {
public:
  static KeyValueDB *create(const string& type);

  virtual int create_and_open(const string& dir,
                              const string& options) = 0;
  virtual void close() {};

  virtual void put(const string& key, const string& value) = 0;
  virtual void del(const string &key) = 0;
  virtual void singleDel(const string& key) {
    del(key);
  }
  virtual void rmRange(const string &start, const string &end) = 0;
  virtual void merge(const string &key, const string& value) {
    assert("Not implemented!");
  }
  virtual void get(const string &key, string &value) = 0;
};
#endif
