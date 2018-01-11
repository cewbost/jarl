#ifndef CMP_TRAIT_H_INCLUDED
#define CMP_TRAIT_H_INCLUDED

template<class T>
struct CmpTrait{
  
  constexpr bool operator==(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) == 0;
  }
  constexpr bool operator!=(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) != 0;
  }
  constexpr bool operator>(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) > 0;
  }
  constexpr bool operator<(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) < 0;
  }
  constexpr bool operator>=(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) >= 0;
  }
  constexpr bool operator<=(const T& other)const{
    return static_cast<const T*>(this)->cmp(other) <= 0;
  }
};

#endif