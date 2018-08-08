#ifndef STRING_H_INCLUDED
#define STRING_H_INCLUDED

#include "cmp_mixin.h"
#include "misc.h"
#include "rc_mixin.h"

#include <new>
#include <functional>

#ifndef NDEBUG
#include "alloc_monitor.h"
#endif

/*
  An immutable string allocated in a single buffer.
  This object is actually just the header of a dynamically allocated block of memory.
  This object must be allocated using the make_new template specialization.
  For this reason creating objects of this class is disabled using any other method.
  Copying and moving is also disabled.
*/

#ifndef NDEBUG
class String;
extern AllocMonitor<String> string_alloc_monitor;
#endif

class String:
  public RcDirectMixin<String>,
  public CmpMixin<String>
#ifndef NDEBUG
, public MonitoredMixin<String, string_alloc_monitor>
#endif
{
  int len_;
  //the hash_ value should be initialized in every constructor, since it is used
  //by std::hash.
  size_t hash_;
  
  char* mut_str_(){return reinterpret_cast<char*>(this) + sizeof(String);}
  
  String();
  String(const char* str, int l);
  String(const String* l, const String* r);
  String(const String*, const char*, int);
  String(const char*, int, const String*);
  
public:
  int len()const{return len_;}
  const char* str()const{
    return reinterpret_cast<const char*>(this) + sizeof(String);
  }
  
  String(const String&) = delete;
  String(String&&) = delete;
  String& operator=(const String&) = delete;
  String& operator=(String &&) = delete;
  
  int cmp(const String& other)const;
  int cmp(const char* other)const;
  bool operator==(const String& other)const{
    return this == &other;
  }
  bool operator!=(const String& other)const{
    return this != &other;
  }
  
  size_t hash()const;
  
  bool toInt32(int32_t*);
  bool toInt64(int64_t*);
  bool toFloat(float*);
  bool toDouble(double*);
  
  int utf8Len()const;
  int utf82Idx(unsigned)const;
  uint32_t getGlyph(unsigned)const;
  uint32_t utf8Get(unsigned)const;
  
  void operator delete(void*);
  void operator delete[](void*) = delete;
  
  static void* operator new(size_t)   = delete;
  static void* operator new[](size_t) = delete;
  
  friend String* make_new<String>();
  
  friend String* make_new<String, const char*>
    (const char*);
  friend String* make_new<String, const char*, int>
    (const char*, int);
  friend String* make_new<String, const char*, const char*>
    (const char*, const char*);
  friend String* make_new<String, const String*, const String*>
    (const String*, const String*);
  friend String* make_new<String, const char*, int, const String*>
    (const char*, int, const String*);
  friend String* make_new<String, const String*, const char*, int>
    (const String*, const char*, int);

  friend String* make_new<String, bool>(bool);
  friend String* make_new<String, int32_t>(int32_t);
  friend String* make_new<String, int64_t>(int64_t);
  friend String* make_new<String, float>(float);
  friend String* make_new<String, double>(double);
  
  friend String* make_new<String, uint32_t>(uint32_t);
  
  friend String* make_new<String, bool, const String*>(bool, const String*);
  friend String* make_new<String, int32_t, const String*>(int32_t, const String*);
  friend String* make_new<String, int64_t, const String*>(int64_t, const String*);
  friend String* make_new<String, float, const String*>(float, const String*);
  friend String* make_new<String, double, const String*>(double, const String*);
  friend String* make_new<String, const String*, bool>(const String*, bool);
  friend String* make_new<String, const String*, int32_t>(const String*, int32_t);
  friend String* make_new<String, const String*, int64_t>(const String*, int64_t);
  friend String* make_new<String, const String*, float>(const String*, float);
  friend String* make_new<String, const String*, double>(const String*, double);
  
  friend std::hash<String>;
};

template<> String* make_new<String>();

template<> String* make_new<String, const char*>
  (const char*);
template<> String* make_new<String, const char*, int>
  (const char*, int);
template<> String* make_new<String, const char*, const char*>
  (const char*, const char*);
template<> String* make_new<String, const String*, const String*>
  (const String*, const String*);
template<> String* make_new<String, const char*, int, const String*>
  (const char*, int, const String*);
template<> String* make_new<String, const String*, const char*, int>
  (const String*, const char*, int);

template<> String* make_new<String, bool>(bool);
template<> String* make_new<String, int32_t>(int32_t);
template<> String* make_new<String, int64_t>(int64_t);
template<> String* make_new<String, float>(float);
template<> String* make_new<String, double>(double);

template<> String* make_new<String, uint32_t>(uint32_t);

template<> String* make_new<String, bool, const String*>(bool, const String*);
template<> String* make_new<String, int32_t, const String*>(int32_t, const String*);
template<> String* make_new<String, int64_t, const String*>(int64_t, const String*);
template<> String* make_new<String, float, const String*>(float, const String*);
template<> String* make_new<String, double, const String*>(double, const String*);
template<> String* make_new<String, const String*, bool>(const String*, bool);
template<> String* make_new<String, const String*, int32_t>(const String*, int32_t);
template<> String* make_new<String, const String*, int64_t>(const String*, int64_t);
template<> String* make_new<String, const String*, float>(const String*, float);
template<> String* make_new<String, const String*, double>(const String*, double);

namespace std{
  
  template<> struct hash<String>{
    size_t operator()(const String& arg)const{
      return arg.hash_;
    }
  };
  
  template<> struct equal_to<String>{
    bool operator()(const String& lhs, const String& rhs)const{
      return lhs.cmp(rhs) == 0;
    }
  };
}

#endif