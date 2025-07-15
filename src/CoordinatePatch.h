#ifndef COORDINATE_H
#define COORDINATE_H 
//XXXX PRAGMA

#include "InterfaceCrypt.h"
#include "State.h"
#include <string>
#include <vector>

class CoordinatePatch : InterfaceCrypt
{
public:
  CoordinatePatch()
  {
  }

  CoordinatePatch(std::string& s)
  {
    this->r_ = s;
  };

  virtual ~CoordinatePatch()
  {
  }

  inline virtual int sig()
  {
    return 0;
  }
  inline virtual bool burstRelay(BurstBuffer& d)
  {
    return true;
  }
  inline virtual bool transientRelay(BurstBuffer& d)
  {
    return true;
  }
  inline virtual bool internRelay(BurstBuffer& d)
  {
    return true;
  }
  inline virtual void burstTx(BurstBuffer& d) { }
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


private:
  std::string r_;
  std::string locator_;
  std::vector<unsigned int> connectionVector_;
};

#endif
