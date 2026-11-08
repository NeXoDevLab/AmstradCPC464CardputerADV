#include "GameFiles.h"
#include <algorithm>
#include <utility>

namespace cpc_files {
namespace {
char lower(char c) {return c>='A' && c<='Z' ? c+'a'-'A' : c;}
bool visible(const std::string &name) {
  return !name.empty() && name[0]!='.' && name.find('/')==std::string::npos;
}
// Sorted, bounded storage: folders cannot disappear behind the file limit.
void insert(Listing &out,Entry entry) {
  auto before=[](const Entry &a,const Entry &b) {
    return a.directory!=b.directory ? a.directory : a.name<b.name;
  };
  std::size_t at=0;
  while(at<out.count && !before(entry,out.entries[at])) ++at;
  if(out.count==kCapacity) out.more=true;
  if(at==kCapacity) return;
  const auto end=std::min(out.count,kCapacity-1);
  for(auto i=end;i>at;--i) out.entries[i]=std::move(out.entries[i-1]);
  out.entries[at]=std::move(entry);
  if(out.count<kCapacity) ++out.count;
}
bool ensureDirectory(Storage &storage,const std::string &path) {
  return storage.exists(path.c_str()) ? storage.directory(path.c_str()) : storage.mkdir(path.c_str());
}
}

std::string baseName(const std::string &path) {
  const auto at=path.find_last_of('/');
  return at==std::string::npos ? path : path.substr(at+1);
}
Kind kindOf(const std::string &name) {
  const auto dot=name.find_last_of('.');
  if(dot==std::string::npos || name.size()-dot!=4) return Kind::none;
  std::string ext=name.substr(dot+1);
  for(auto &c:ext) c=lower(c);
  return ext=="cdt" ? Kind::cdt : ext=="dsk" ? Kind::dsk : ext=="sna" ? Kind::sna : Kind::none;
}
std::string child(const std::string &folder,const std::string &name) {
  if(!visible(name)) return folder;
  return folder+"/"+name;
}
std::string parent(const std::string &folder) {
  const std::string root=kRoot;
  if(folder.compare(0,root.size()+1,root+"/")!=0) return root;
  const auto slash=folder.find_last_of('/');
  return slash<=root.size() ? root : folder.substr(0,slash);
}
Listing list(Storage &storage,const std::string &folder) {
  Listing out;
  list(storage,folder,out);
  return out;
}
bool list(Storage &storage,const std::string &folder,Listing &out) {
  for(auto &entry:out.entries) {std::string{}.swap(entry.name);entry.directory=false;}
  out.count=0;out.more=false;out.ok=false;
  out.ok=storage.scan(folder.c_str(),[](void *context,const char *raw,bool directory) {
    const auto name=baseName(raw);
    if(visible(name) && (directory || kindOf(name)!=Kind::none))
      insert(*static_cast<Listing *>(context),{name,directory});
  },&out);
  return out.ok;
}
std::string findMedia(Storage &storage,Kind kind) {
  struct Context {Kind kind;std::string folder,selected;bool preferred=false;};
  Context context{kind,kRoot,"",false};
  auto visit=[](void *raw,const char *entry,bool directory) {
    auto &c=*static_cast<Context *>(raw);
    const auto name=baseName(entry);
    if(directory || !visible(name) || kindOf(name)!=c.kind) return;
    const bool preferred=name.compare(0,11,"Profanation")==0;
    const auto path=child(c.folder,name);
    if(c.selected.empty() || (preferred && !c.preferred) || (preferred==c.preferred && path<c.selected)) {
      c.selected=path;c.preferred=preferred;
    }
  };
  storage.scan(kRoot,visit,&context);
  context.folder=std::string(kRoot)+(kind==Kind::cdt ? "/CDT" : kind==Kind::dsk ? "/DSK" : "/SNA");
  storage.scan(context.folder.c_str(),visit,&context);
  return context.selected;
}
OrganizeResult organize(Storage &storage) {
  OrganizeResult result;
  if(!ensureDirectory(storage,kRoot) || !ensureDirectory(storage,std::string(kRoot)+"/CDT") ||
     !ensureDirectory(storage,std::string(kRoot)+"/DSK")) {++result.failed;return result;}
  result.ready=true;
  // Collect and close each directory before renaming. A sorted cursor also
  // handles more than 64 files and collisions without repeating skipped files.
  std::string after;
  for(;;) {
    struct Batch {Listing files;const std::string *after;};
    Batch batch{{},&after};
    if(!storage.scan(kRoot,[](void *raw,const char *entry,bool directory) {
      auto &b=*static_cast<Batch *>(raw);
      const auto name=baseName(entry);const auto kind=kindOf(name);
      if(!directory && visible(name) && name>*b.after && (kind==Kind::cdt || kind==Kind::dsk))
        insert(b.files,{name,false});
    },&batch)) {++result.failed;return result;}
    if(!batch.files.count) return result;
    for(std::size_t i=0;i<batch.files.count;++i) {
      const auto &name=batch.files.entries[i].name;after=name;
      const auto source=child(kRoot,name);
      const auto target=std::string(kRoot)+(kindOf(name)==Kind::cdt ? "/CDT/" : "/DSK/")+name;
      if(storage.exists(target.c_str())) {++result.skipped;continue;}
      if(!storage.move(source.c_str(),target.c_str())) {++result.failed;continue;}
      if(kindOf(name)==Kind::cdt) ++result.cdt;else ++result.dsk;
    }
  }
}
}
