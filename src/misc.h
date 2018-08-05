#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <type_traits>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstdarg>

/*
  use this instead of new, to allow for overloading where the parameters are passed
  to the allocation function.
*/
template<class T, class... P>
T* make_new(P... p)
{return new T(p...);}

template<class T>
struct ptr_hash{
  typedef std::hash<typename std::remove_pointer<T>::type> hash;
  size_t operator()(const T& arg)const{
    return hash::operator()(*arg);
  }
};

template<class T>
struct ptr_equal_to{
  typedef std::equal_to<typename std::remove_pointer<T>::type> equal_to;
  constexpr bool operator()(const T& lhs, const T& rhs)const{
    return equal_to::operator()(*lhs, *rhs);
  }
};

inline char* dynSprintf(const char* format, ...){
  va_list args;
  va_start(args, format);
  int len = vsnprintf(nullptr, 0, format, args);
  
  char* buffer = new char[len + 1];
  va_end(args);
  va_start(args, format);
  vsprintf(buffer, format, args);
  
  va_end(args);
  return buffer;
}

inline size_t defaultHash(const char* from, const char* to){
  constexpr size_t _offset = sizeof(size_t) == 8?
    0xcbf29ce484222325 : 0x811c9dc5;
  constexpr size_t _prime = sizeof(size_t) == 8?
    0x100000001b3 : 0x1000193;
  
  size_t ret = _offset;
  for(; from < to; ++from){
    ret ^= *from;
    ret *= _prime;
  }
  return ret;
}

#ifndef NDEBUG
inline std::string unlexString(const char* str){
  std::string ret = "\"";
  ret += str;
  ret += "\"";
  
  std::string::size_type pos = 0;
  for(;;){
    pos = ret.find('\n', pos);
    if(pos == std::string::npos) break;
    ret.replace(pos, 1, "\\n");
  }
  pos = 0;
  for(;;){
    pos = ret.find('\t', pos);
    if(pos == std::string::npos) break;
    ret.replace(pos, 1, "\\t");
  }
  
  return ret;
}
#endif

#endif