#include "string.h"

#ifndef NDEBUG
#include "alloc_monitor.h"
#include <cstdio>
#endif

namespace{
  
  inline int writeNumToBuffer_(char* buffer, bool val){
    return sprintf(buffer, "%s", val? "true" : "false");
  }
  inline int writeNumToBuffer_(char* buffer, int32_t val){
    return sprintf(buffer, "%d", val);
  }
  inline int writeNumToBuffer_(char* buffer, int64_t val){
    return sprintf(buffer, "%lld", val);
  }
  inline int writeNumToBuffer_(char* buffer, float val){
    return sprintf(buffer, "%g", (double)val);
  }
  inline int writeNumToBuffer_(char* buffer, double val){
    return sprintf(buffer, "%g", val);
  }
  
  template<class T>
  String* makeStringFromNum_(T val){
    char buffer[32];
    int len = writeNumToBuffer_(buffer, val);
    return make_new<String>(const_cast<const char*>(buffer), len);
  }
  
  template<class T>
  String* addNumToString_(T val, const String* st){
    char buffer[32];
    int len = writeNumToBuffer_(buffer, val);
    return make_new<String>(const_cast<const char*>(buffer), len, st);
  }
  template<class T>
  String* addNumToString_(const String* st, T val){
    char buffer[32];
    int len = writeNumToBuffer_(buffer, val);
    return make_new<String>(st, const_cast<const char*>(buffer), len);
  }
}

String::String(): len_(0){
  this->mut_str_()[0] = '\0';
}

String::String(const char* str, int l)
: len_(l){
  memcpy(this->mut_str_(), str, l);
  this->mut_str_()[l] = '\0';
}

String::String(const String* l, const String* r)
: len_(l->len() + r->len()){
  memcpy(this->mut_str_(), l->str(), l->len());
  memcpy(this->mut_str_() + l->len(), r->str(), r->len());
  this->mut_str_()[this->len_] = '\0';
}

String::String(const String* st, const char* cs, int csl)
: len_(st->len() + csl){
  memcpy(this->mut_str_(), st->str(), st->len());
  memcpy(this->mut_str_() + st->len(), cs, csl);
  this->mut_str_()[this->len_] = '\0';
}

String::String(const char* cs, int csl, const String* st)
: len_(st->len() + csl){
  memcpy(this->mut_str_(), cs, csl);
  memcpy(this->mut_str_() + csl, st->str(), st->len());
  this->mut_str_()[this->len_] = '\0';
}

size_t String::hash()const{
  constexpr size_t _offset = sizeof(size_t) == 8?
    0xcbf29ce484222325 : 0x811c9dc5;
  constexpr size_t _prime = sizeof(size_t) == 8?
    0x100000001b3 : 0x1000193;
  
  const char* data = this->str();
  size_t ret = _offset;
  while(*data != '\0'){
    ret ^= *data;
    ret *= _prime;
    ++data;
  }
  return ret;
}

bool String::toInt32(int32_t* t){
  return sscanf(this->str(), "%d", t) == 1;
}
bool String::toInt64(int64_t* t){
  return sscanf(this->str(), "%lld", t) == 1;
}
bool String::toFloat(float* t){
  return sscanf(this->str(), "%f", t) == 1;
}
bool String::toDouble(double* t){
  return sscanf(this->str(), "%lf", t) == 1;
}

int String::utf8Len()const{
  int len = 0;
  for(const char* p = this->str(); *p != '\0'; ++p){
    if((*p & 0x80) == 0) ++len;
  }
  return len;
}

uint32_t String::utf8Get(unsigned utf8_idx)const{
  auto idx = this->utf82Idx(utf8_idx);
  if(idx < 0) return 0xffffffff;
  else return this->getGlyph(idx);
}

int String::utf82Idx(unsigned i)const{
  int ret = 0;
  const char* p = this->str();
  
  for(; i > 0; --i){
    if(p[ret] == 0x00) return -1;
    while((p[ret] & 0x80) != 0) ++ret;
    ++ret;
  }
  
  return ret;
}

uint32_t String::getGlyph(unsigned idx)const{
  uint32_t ret = 0;
  const char* p = this->str() + idx;
  for(int i = 0; i < 4; ++i){
    ret <<= 8;
    ret |= p[i];
    if((ret & 0x80) == 0) break;
  }
  return ret;
}

template<>
String* make_new<String>(){
  void* mem = ::operator new(1 + sizeof(String));
  return ::new(mem) String();
}

template<>
String* make_new<String, const char*>(const char* str){
  int len = strlen(str);
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(str, len);
}

template<>
String* make_new<String, const char*, int>(const char* str, int len){
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(str, len);
}

template<>
String* make_new<String, const char*, const char*>(const char* begin, const char* end){
  int len = end - begin;
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(begin, len);
}

template<>
String* make_new<String, const String*, const String*>(const String* l, const String* r){
  int len = l->len() + r->len();
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(l, r);
}

template<>
String* make_new<String, const char*, int, const String*>
(const char* cs, int csl, const String* st){
  int len = csl + st->len();
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(cs, csl, st);
}

template<>
String* make_new<String, const String*, const char*, int>
(const String* st, const char* cs, int csl){
  int len = csl + st->len();
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(st, cs, csl);
}

//numeral conversions
template<> String* make_new<String, bool>(bool val){
  return makeStringFromNum_(val);
}
template<> String* make_new<String, int32_t>(int32_t val){
  return makeStringFromNum_(val);
}
template<> String* make_new<String, int64_t>(int64_t val){
  return makeStringFromNum_(val);
}
template<> String* make_new<String, float>(float val){
  return makeStringFromNum_(val);
}
template<> String* make_new<String, double>(double val){
  return makeStringFromNum_(val);
}

//single glyph
template<> String* make_new<String, uint32_t>(uint32_t val){
  int len = 4;
  while(val & 0xff000000 == 0){
    val <<= 8;
    --len;
  }
  char buffer[4];
  for(int i = 3; i >= 0; --i){
    buffer[i] = (val >> (8 * i)) & 0xff;
  }
  
  void* mem = ::operator new(len + 1 + sizeof(String));
  return ::new(mem) String(buffer, len);
}

//numeral additions
template<>
String* make_new<String, bool, const String*>(bool val, const String* st){
  return addNumToString_(val, st);
}
template<>
String* make_new<String, int32_t, const String*>(int32_t val, const String* st){
  return addNumToString_(val, st);
}
template<>
String* make_new<String, int64_t, const String*>(int64_t val, const String* st){
  return addNumToString_(val, st);
}
template<>
String* make_new<String, float, const String*>(float val, const String* st){
  return addNumToString_(val, st);
}
template<>
String* make_new<String, double, const String*>(double val, const String* st){
  return addNumToString_(val, st);
}
template<>
String* make_new<String, const String*, bool>(const String* st, bool val){
  return addNumToString_(st, val);
}
template<>
String* make_new<String, const String*, int32_t>(const String* st, int32_t val){
  return addNumToString_(st, val);
}
template<>
String* make_new<String, const String*, int64_t>(const String* st, int64_t val){
  return addNumToString_(st, val);
}
template<>
String* make_new<String, const String*, float>(const String* st, float val){
  return addNumToString_(st, val);
}
template<>
String* make_new<String, const String*, double>(const String* st, double val){
  return addNumToString_(st, val);
}

#ifndef NDEBUG
AllocMonitor<String> string_alloc_monitor([](AllocMsg msg, const String* str){
  switch(msg){
  //case AllocMsg::Allocation:
  //  fprintf(stderr, "allocaed string at %p\n", str);
  //  break;
  //case AllocMsg::Deallocation:
  //  fprintf(stderr, "deallocated string at %p\n", str);
  //  break;
  case AllocMsg::DoubleAllocation:
    fprintf(stderr, "double allocated string at %p\n", str);
    break;
  case AllocMsg::InvalidFree:
    fprintf(stderr, "invalid free of string at %p\n", str);
    break;
  }
});
#endif