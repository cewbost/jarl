#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <type_traits>
#include <functional>

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