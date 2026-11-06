#pragma once
#include <cstdio>
#include <memory>
#include <cstdint>
#define FILE_READ "rb"
class File {
  std::shared_ptr<FILE> f;
public:
  File() = default;
  explicit File(const char *p) : f(fopen(p, "rb"), [](FILE *v) { if (v) fclose(v); }) {}
  explicit operator bool() const { return f.get() != nullptr; }
  size_t read(uint8_t *b, size_t n) { return f ? fread(b,1,n,f.get()) : 0; }
  int read() { return f ? fgetc(f.get()) : -1; }
  bool seek(uint32_t n) { return f && !fseek(f.get(),n,SEEK_SET); }
  uint32_t position() { return f ? ftell(f.get()) : 0; }
  size_t size() { auto p=position(); fseek(f.get(),0,SEEK_END); auto n=position(); seek(p); return n; }
  void close() { f.reset(); }
};
struct HostSD { File open(const char *p, const char * = FILE_READ) { return File(p); } };
inline HostSD SD;
