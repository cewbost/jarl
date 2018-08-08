#ifndef CMP_MIXIN_H_INCLUDED
#define CMP_MIXIN_H_INCLUDED

template<class T>
struct CmpMixin{
  
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
  
private:
  CmpMixin(){}
  friend T;
};

#endif