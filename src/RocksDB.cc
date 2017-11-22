#include "RocksDB.h"
#include <cassert>
#include <map>
#include <list>
#include <ctype.h>
#include "rocksdb/convenience.h"
#include <iostream>

using std::list;
using std::map;
using std::string;

static bool get_next_token(const string &s, size_t& pos, const char *delims, string& token)
{
  int start = s.find_first_not_of(delims, pos);
  int end;

  if (start < 0){
    pos = s.size();
    return false;
  }

  end = s.find_first_of(delims, start);
  if (end >= 0)
    pos = end + 1;
  else {
    pos = end = s.size();
  }

  token = s.substr(start, end - start);
  return true;
}

static string trim(const string& str) {
  size_t start = 0;
  size_t end = str.size() - 1;
  while (isspace(str[start]) != 0 && start <= end) {
    ++start;
  }
  while (isspace(str[end]) != 0 && start <= end) {
    --end;
  }
  if (start <= end) {
    return str.substr(start, end - start + 1);
  }
  return string();
}


static void get_str_list(const string& str, const char *delims, list<string>& str_list)
{
  size_t pos = 0;
  string token;

  str_list.clear();

  while (pos < str.size()) {
    if (get_next_token(str, pos, delims, token)) {
      if (token.size() > 0) {
        str_list.push_back(token);
      }
    }
  }
}

static int get_str_map(
    const string &str,
    map<string,string> *str_map,
    const char *delims)
{
  list<string> pairs;
  get_str_list(str, delims, pairs);
  for (list<string>::iterator i = pairs.begin(); i != pairs.end(); ++i) {
    size_t equal = i->find('=');
    if (equal == string::npos)
      (*str_map)[*i] = string();
    else {
      const string key = trim(i->substr(0, equal));
      equal++;
      const string value = trim(i->substr(equal));
      (*str_map)[key] = value;
    }
  }
  return 0;
}

int RocksDB::ParseOptionsFromString(const string &opt_str, rocksdb::Options &opt)
{
  map<string, string> str_map;
  int r = get_str_map(opt_str, &str_map, ",\n;");
  if (r < 0)
    return r;
  map<string, string>::iterator it;
  for(it = str_map.begin(); it != str_map.end(); ++it) {
    string this_opt = it->first + "=" + it->second;
    rocksdb::Status status = rocksdb::GetOptionsFromString(opt, this_opt , &opt); 
    if (!status.ok()) {
      //unrecognized by rocksdb, try to interpret by ourselves.
      std::cout << status.ToString() << "\n";
      return -EINVAL;
    }
  }
  return 0;
}

int RocksDB::create_and_open(const string& dir,
                              const string& options_str)
{
  rocksdb::Options options;

  if (options_str.length()) {
    int r = ParseOptionsFromString(options_str, options);
    assert(r == 0);
  }

  options.create_if_missing = true;
  //options.merge_operator = rocksdb::MergeOperators::CreateFromStringId("put");
  rocksdb::Status status = rocksdb::DB::Open(options, dir, &db);
  assert(status.ok());
  return 0;
}

void RocksDB::close()
{
  delete db;
}

void RocksDB::put(const string& key, const string& value)
{
  rocksdb::WriteOptions woptions;
//  woptions.sync = true;
//  woptions.disableWAL = true;
  rocksdb::Status status = db->Put(woptions,key, value);
  assert(status.ok());
}

void RocksDB::del(const string &key)
{
  rocksdb::WriteOptions woptions;
//  woptions.sync = true;
//  woptions.disableWAL = true;
  rocksdb::Status status = db->Delete(woptions, key);
  assert(status.ok());
}

void RocksDB::singleDel(const string& key)
{
  rocksdb::WriteOptions woptions;
//  woptions.sync = true;
//  woptions.disableWAL = true;
  rocksdb::Status status = db->SingleDelete(woptions, key);
  assert(status.ok());
}

void RocksDB::merge(const string &key, const string& value)
{
  rocksdb::WriteOptions woptions;
  rocksdb::Status status = db->Merge(woptions,key, value);
  assert(status.ok());
}

void RocksDB::rmRange(const string &start, const string &end)
{
/*
  rocksdb::WriteOptions woptions;
  rocksdb::Status status = db->DeleteRange(woptions, start, end);
  assert(status.ok());
*/
  rocksdb::CompactRangeOptions coptions;
  rocksdb::Slice start_key(start);
  rocksdb::Slice end_key(end);
  rocksdb::Status status = db->CompactRange(coptions, &start_key, &end_key);
  assert(status.ok());
}

void RocksDB::get(const string &key, string &value)
{
  rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
  assert(status.ok());
}
