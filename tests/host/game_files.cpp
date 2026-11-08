#include "../../src/platform/GameFiles.h"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>

namespace fs=std::filesystem;
struct HostStorage final : cpc_files::Storage {
  fs::path root;
  bool failMove=false;
  explicit HostStorage(fs::path path):root(std::move(path)) {}
  fs::path real(const char *path) const {return root/fs::path(path).relative_path();}
  bool scan(const char *path,cpc_files::Visitor visitor,void *context) override {
    if(!fs::is_directory(real(path))) return false;
    std::vector<fs::directory_entry> entries;
    for(const auto &entry:fs::directory_iterator(real(path))) entries.push_back(entry);
    // Exercise arbitrary FAT iteration order, including folders arriving last.
    std::sort(entries.begin(),entries.end(),[](const auto &a,const auto &b) {return a.path()>b.path();});
    for(const auto &entry:entries) visitor(context,entry.path().filename().string().c_str(),entry.is_directory());
    return true;
  }
  bool exists(const char *path) override {return fs::exists(real(path));}
  bool directory(const char *path) override {return fs::is_directory(real(path));}
  bool mkdir(const char *path) override {return fs::create_directory(real(path));}
  bool move(const char *from,const char *to) override {
    if(failMove || exists(to)) return false;
    std::error_code error;fs::rename(real(from),real(to),error);return !error;
  }
  void put(const std::string &path,const char *data="game") {
    const auto target=real(path.c_str());fs::create_directories(target.parent_path());
    std::ofstream(target,std::ios::binary)<<data;
  }
  std::string read(const char *path) {
    std::ifstream input(real(path),std::ios::binary);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
  }
};

int main(int argc,char **argv) {
  assert(argc==2);
  using namespace cpc_files;
  HostStorage storage(fs::path(argv[1])/"card");
  storage.put(std::string(kRoot)+"/zeta.cDt","source tape");
  storage.put(std::string(kRoot)+"/alpha.DsK","source disk");
  storage.put(std::string(kRoot)+"/snapshot.SNA");
  storage.put(std::string(kRoot)+"/._zeta.cdt","AppleDouble");
  storage.put(std::string(kRoot)+"/readme.txt","keep");
  storage.put("/CPC464/OS.ROM","keep ROM");
  assert(kindOf("juego.CdT")==Kind::cdt && kindOf("juego.DsK")==Kind::dsk);
  assert(kindOf("juego.tap")==Kind::none);
  auto root=list(storage,kRoot);assert(root.ok && root.count==3 && !root.more);
  assert(findMedia(storage,Kind::cdt)==std::string(kRoot)+"/zeta.cDt");

  storage.put(std::string(kRoot)+"/CDT/zeta.cDt","existing tape");
  auto organized=organize(storage);
  assert(organized.ready && organized.cdt==0 && organized.dsk==1 && organized.skipped==1 && !organized.failed);
  assert(storage.read("/CPC464/GAMES/zeta.cDt")=="source tape");
  assert(storage.read("/CPC464/GAMES/CDT/zeta.cDt")=="existing tape");
  assert(storage.read("/CPC464/GAMES/DSK/alpha.DsK")=="source disk");
  assert(storage.read("/CPC464/OS.ROM")=="keep ROM");
  assert(storage.read("/CPC464/GAMES/._zeta.cdt")=="AppleDouble");

  for(unsigned i=0;i<130;++i) {
    char name[80];std::snprintf(name,sizeof(name),"%s/tape%03u.cdt",kRoot,i);storage.put(name);
  }
  for(unsigned i=0;i<5;++i) {
    char name[80];std::snprintf(name,sizeof(name),"%s/disk%03u.dsk",kRoot,i);storage.put(name);
  }
  root=list(storage,kRoot);
  assert(root.ok && root.count==64 && root.more);
  assert(root.entries[0].directory && root.entries[0].name=="CDT");
  assert(root.entries[1].directory && root.entries[1].name=="DSK");
  organized=organize(storage);
  assert(organized.ready && organized.cdt==130 && organized.dsk==5 && organized.skipped==1 && !organized.failed);
  for(unsigned i=0;i<130;++i) {
    char name[80];std::snprintf(name,sizeof(name),"%s/CDT/tape%03u.cdt",kRoot,i);assert(storage.read(name)=="game");
  }
  organized=organize(storage);
  assert(organized.ready && !organized.cdt && !organized.dsk && organized.skipped==1 && !organized.failed);
  storage.put(std::string(kRoot)+"/Espaคol.CDT","UTF-8 name");
  organized=organize(storage);
  assert(organized.cdt==1 && !organized.failed);
  assert(storage.read("/CPC464/GAMES/CDT/Espaคol.CDT")=="UTF-8 name");
  storage.put(std::string(kRoot)+"/CDT/Profanation cinta.cdt");
  assert(findMedia(storage,Kind::cdt)==std::string(kRoot)+"/CDT/Profanation cinta.cdt");
  assert(findMedia(storage,Kind::dsk)==std::string(kRoot)+"/DSK/alpha.DsK");

  storage.put(std::string(kRoot)+"/CDT/aventuras/episodio.cdt");
  assert(list(storage,child(child(kRoot,"CDT"),"aventuras")).count==1);
  assert(parent(child(child(kRoot,"CDT"),"aventuras"))==child(kRoot,"CDT"));
  assert(parent(child(kRoot,"CDT"))==kRoot && parent(kRoot)==kRoot);
  assert(parent("/CPC464/GAMES_OTHER/child")==kRoot);
  assert(child(kRoot,"../escape")==kRoot && child(kRoot,"..")==kRoot);
  assert(!list(storage,child(kRoot,"missing")).ok);
  Listing reused;
  assert(list(storage,child(kRoot,"CDT"),reused) && reused.count==64 && reused.more);
  assert(list(storage,child(kRoot,"DSK"),reused) && reused.count==6 && !reused.more);
  assert(!list(storage,child(kRoot,"missing"),reused) && reused.count==0 && !reused.more);

  storage.put(std::string(kRoot)+"/move-fails.cdt","keep on error");storage.failMove=true;
  organized=organize(storage);
  assert(organized.failed==1 && organized.skipped==1);
  assert(storage.read("/CPC464/GAMES/move-fails.cdt")=="keep on error");
  HostStorage broken(fs::path(argv[1])/"broken");
  broken.put(std::string(kRoot)+"/CDT","not a folder");broken.put(std::string(kRoot)+"/safe.cdt","untouched");
  organized=organize(broken);
  assert(!organized.ready && organized.failed==1 && broken.read("/CPC464/GAMES/safe.cdt")=="untouched");
  std::puts("Game folders, nested paths, filtering, 64-entry limit, batch moves, collisions and failure preservation: OK");
}
