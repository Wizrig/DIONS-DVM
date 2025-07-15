#pragma once

#include "NodeEx.h"
#include "InterfaceCrypt.h"
#include "Coordinate.h"
#include "CoordinatePatch.h"
#include "RayShade.h"
#include <string>

class Relay : InterfaceCrypt
{
public:
  Relay()
  {
  }

  Relay(std::string& s)
  {
    this->r_ = s;
  };

  virtual ~Relay()
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

  IMPLEMENT_SERIALIZE
  (
    READWRITE(this->r_);
    READWRITE(this->locator_);
  )

private:
  std::string r_;
  std::string locator_;
};

