// Decode kv operations generated by BlueStore
#include <fstream>
#include <ostream>
#include <iostream>
#include <stdlib.h>
#include <cstdint>
#include <string.h>
#include <map>
#include "KeyValueDB.h"
#include <time.h>
#include <utime.h>

using std::string;

template<typename S>
static string pretty_binary_string(const S& in)
{
  char buf[10];
  string out;
  out.reserve(in.length() * 3);
  enum { NONE, HEX, STRING } mode = NONE;
  unsigned from = 0, i;
  for (i=0; i < in.length(); ++i) {
    if ((in[i] < 32 || (unsigned char)in[i] > 126) ||
        (mode == HEX && in.length() - i >= 4 &&
         ((in[i] < 32 || (unsigned char)in[i] > 126) ||
          (in[i+1] < 32 || (unsigned char)in[i+1] > 126) ||
          (in[i+2] < 32 || (unsigned char)in[i+2] > 126) ||
          (in[i+3] < 32 || (unsigned char)in[i+3] > 126)))) {
      if (mode == STRING) {
        out.append(in.c_str() + from, i - from);
        out.push_back('\'');
      }
      if (mode != HEX) {
        out.append("0x");
        mode = HEX;
      }
      if (in.length() - i >= 4) {
        // print a whole u32 at once
        snprintf(buf, sizeof(buf), "%08x",
                 (uint32_t)(((unsigned char)in[i] << 24) |
                            ((unsigned char)in[i+1] << 16) |
                            ((unsigned char)in[i+2] << 8) |
                            ((unsigned char)in[i+3] << 0)));
        i += 3;
      } else {
        snprintf(buf, sizeof(buf), "%02x", (int)(unsigned char)in[i]);
      }
      out.append(buf);
    } else {
      if (mode != STRING) {
        out.push_back('\'');
        mode = STRING;
        from = i;
      }
    }
  }

  if (mode == STRING) {
    out.append(in.c_str() + from, i - from);
    out.push_back('\'');
  }
  return out;
}

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

uint32_t swab(uint32_t val)
{
  return (( val >> 24) |
          ((val >> 8)  & 0xff00) |
          ((val << 8)  & 0xff0000) |
          ((val << 24)));
}


int get_count = 0;
int range_count = 0;
void handle_kv_operation(const char type,
                         const string& key,
                         const string& key2,
                         uint32_t value_len,
                         KeyValueDB* db)
{
  if (type == 4) {
    assert(!key.empty());
    assert(!key2.empty());
  }
/*
  if(key[0] != 'P') {
    return;
  }
  if(type == 0 && get_count < 100) {
    string value;
    db->get(key, value);
    std::cout << pretty_binary_string(key) << "\n";
    get_count++;
  }
  if (type == 4 && range_count < 10) {
    std::cout << key.length() << pretty_binary_string(key) << " ====>" << key2.length() << pretty_binary_string(key2) << "\n";
    range_count++;
  }
  return;
*/

  switch (type) {
    case 0:
    {
      if (value_len != 0)
      {
        string value(value_len, 'x');
        db->put(key, value);
      }
      else {
        string value;
        db->put(key, value);
      }
      break;
    }
    case 1:
      db->del(key);
      break;
    case 2:
      db->singleDel(key);
      break;
    case 3:
    {
      assert(value_len != 0);
//      string value(value_len, 'x');
//      db->merge(key, value);
      break;
    }
    case 4:
      assert(key2[0] == 'P');
      assert(key[0] == 'P');
      db->rmRange(key, key2);
      break;
    default:
      std::cout << "unknown type " << key << "\n";
      assert("Unknown type");
  }
}

int decode_kv_logs(const std::string &filepath, KeyValueDB *db)
{
  char *buffer;
  long size;

  std::ifstream myfile (filepath.c_str(), std::ios::in|std::ios::binary);
  if (!myfile.good()) {
    std::cout << filepath << " not good\n";
    return 1;
  }
  myfile.seekg(0, std::ios::end);
  size = myfile.tellg(); 
  myfile.seekg(0, std::ios::beg);
  buffer = new char[size];
  myfile.read(buffer, size);
  myfile.close();

  char *p = buffer;
  while (p < (size + buffer) )
  {
    // Handle one operation
    char type;
    uint32_t key_len, value_len=1;
    std::string key;
    std::string end;

    type = *(p++);
    memcpy(&key_len, p, sizeof(uint32_t));
    key_len = swab(key_len);
    p = p + sizeof(uint32_t);
    if (key_len == 0) {
        std::cout << "Error: key is null\n";
        return 1;
    }
    // get key
    key.append(p, key_len);
    p = p + key_len;
    // value length
    if (type == 4)
    {
      uint32_t end_len;
      memcpy(&end_len, p, sizeof(uint32_t));
      end_len = swab(end_len);
      p = p + sizeof(uint32_t);
      end.append(p, end_len);
      p = p + end_len;
    }
    else {
      memcpy(&value_len, p, sizeof(uint32_t));
      value_len = swab(value_len);
      p = p + sizeof(uint32_t);
    }

    handle_kv_operation(type, key, end, value_len, db);
  }
  free(buffer);
  return 0;
}

int main(int argc, char *argv[])
{
  KeyValueDB *db = KeyValueDB::create("rocksdb");
  string options = "compression=kNoCompression,max_write_buffer_number=4,min_write_buffer_number_to_merge=1,recycle_log_file_num=4,write_buffer_size=268435456,writable_file_max_buffer_size=0,compaction_readahead_size=2097152,stats_dump_period_sec=30";
//flush_style=kFlushStyleDedup";//stats_dump_period_sec=300";//level0_file_num_compaction_trigger=8,flush_style=kFlushStyleDedup";
  db->create_and_open("/var/lib/ceph/mnt/osd-device-0-data/kv", options);
  std::cout << "check db is null\n";
  assert(db != NULL);

  int file_num = 0;
  if (argc != 2) {
    std::cout <<"exec iomod\n";
    return 1;
  }
  std::string filepath;
  std::string iomode = argv[1];
  if (iomode.compare("4k") == 0) {
    filepath = "/opt/fio_test/osd/4k_logs_range_30mins/logger";
  }
  else if (iomode.compare("16k") == 0) {
    filepath = "/opt/fio_test/osd/16k_logs/logger";
  }
  else {
    filepath = "/opt/fio_test/osd/logs/logger";
  }

  struct timespec start_tp;
  clock_gettime(CLOCK_REALTIME, &start_tp);
  //utime_t n(tp);

  while (1) {
     string filename(filepath);
     filename.append(std::to_string(file_num));
     std::cout << "file started " << filename << "\n";
     if (decode_kv_logs(filename, db)) {
       break;
     }
     file_num++;
  }
  struct timespec end_tp;
  clock_gettime(CLOCK_REALTIME, &end_tp);
  struct timespec duration;
  timespec_diff(&start_tp, &end_tp, &duration);
  std::cout << "Duration second " << duration.tv_sec << " nsec " << duration.tv_nsec << "\n"; 
  return 0;
}
