#pragma once
#include <array>
#include <cstddef>
#include <string>

namespace cpc_files {
constexpr const char *kRoot = "/CPC464/GAMES";
constexpr std::size_t kCapacity = 64;
constexpr std::size_t kPageRows = 6;
std::size_t navigate(std::size_t selected,std::size_t count,int rows,int pages);
enum class Kind { none, cdt, dsk, sna };
struct Entry { std::string name; bool directory=false; };
struct Listing {
  std::array<Entry,kCapacity> entries;
  std::size_t count=0;
  bool ok=false, more=false;
};
using Visitor = void (*)(void *,const char *,bool);
struct Storage {
  virtual ~Storage() = default;
  virtual bool scan(const char *,Visitor,void *) = 0;
  virtual bool exists(const char *) = 0;
  virtual bool directory(const char *) = 0;
  virtual bool mkdir(const char *) = 0;
  virtual bool move(const char *,const char *) = 0;
};
Kind kindOf(const std::string &);
std::string baseName(const std::string &);
std::string child(const std::string &,const std::string &);
std::string parent(const std::string &);
Listing list(Storage &,const std::string &);
bool list(Storage &,const std::string &,Listing &);
std::string findMedia(Storage &,Kind);
struct OrganizeResult {
  unsigned cdt=0,dsk=0,skipped=0,failed=0;
  bool ready=false;
};
OrganizeResult organize(Storage &);
}
