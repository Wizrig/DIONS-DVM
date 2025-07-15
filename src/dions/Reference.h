#pragma once 

#include "Constants.h"
#include "Dions.h"


class Reference
{
public:
  Reference() : INIT_REF("0")
  {
  };
  Reference(vchType k)
  {
    INIT_REF="0";
    this->m_ = stringFromVch(k);
  }
  bool operator()()
  {
    return this->m_ == INIT_REF;
  }

private:
  std::string INIT_REF ;
  std::string m_;
};

