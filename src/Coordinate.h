#ifndef COORDINATE_H
#define COORDINATE_H 

//XXXX PRAGMA

#include "InterfaceCrypt.h"
#include <string>
#include <vector>
#include <algorithm>

class CoordinateVector : InterfaceCrypt
{
public:
  CoordinateVector()
  {
  }

  CoordinateVector(std::string& s)
  {
    this->r_ = s;
  };

  virtual ~CoordinateVector()
  {
  }

  inline virtual int sig()
  {
    return 0;
  }
  inline virtual void open() {}
  inline virtual void close() {}
  inline virtual std::string alias()
  {
    return "";
  }
  inline virtual std::string ctrl_()
  {
    return this->r_;
  }
  inline virtual void ctrl(std::string& c)
  {
    this->r_ = c;
  }

  inline bool mapNode(std::string l, int& p)
  {
    std::vector<std::string>::iterator iter;
    iter=std::find(this->d1_.begin(),this->d1_.end(),l);
    if(iter != this->d1_.end())
    {
      p = iter - this->d1_.begin();
      return true;
    }

    return false;
  }

  inline bool scale_()
  {
    return this->d0_.size() == 0;
  }
  inline bool scale()
  {
    return this->d1_.size() == 0;
  }
  inline std::string& domainImage()
  {
    return this->d0_.back();
  }
  inline std::string& codomainImage()
  {
    return this->d1_.back();
  }
  inline void domain(std::string s)
  {
    this->d0_.push_back(s);
  }
  inline void codomain(std::string s)
  {
    this->d1_.push_back(s);
  }

  IMPLEMENT_SERIALIZE
  (
    READWRITE(this->d0_);
    READWRITE(this->d1_);
  )

private:
  std::string r_;
  std::string locator_;
  std::vector<std::string> d0_;
  std::vector<std::string> d1_;
};

#endif
