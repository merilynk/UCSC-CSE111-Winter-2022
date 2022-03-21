// $Id: xless.h,v 1.4 2021-12-20 12:56:53-08 - - $

#ifndef XLESS_H
#define XLESS_H

//
// We assume that the type type_t has an operator< function.
//

template <typename Type>
struct xless {
   bool operator() (const Type& left, const Type& right) const {
      return left < right;
   }
};

#endif

