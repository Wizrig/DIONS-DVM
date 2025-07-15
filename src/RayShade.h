#pragma once

#include "InterfaceCrypt.h"
#include <string>
#include <vector>


struct __pq__
{
  std::vector<unsigned char> __fq1;
  std::vector<unsigned char> __fq9;
  std::vector<unsigned char> __fq0;
  std::vector<unsigned char> __fq2;
  std::vector<unsigned char> __fq5;
};

struct __inv__
{
  std::vector<unsigned char> __inv7;
  std::vector<unsigned char> __inv1;
};

class RayShade : InterfaceCrypt
{
public:
  static const int RAY_VTX = 0xa1;
  static const int RAY_SET = 0xc2;

  RayShade()
  {
  }

  RayShade(std::string& s)
  {
    this->tgt_ = s;
  };

  virtual ~RayShade()
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
    return this->tgt_;
  }
  inline virtual void ctrl(std::string& c)
  {
    this->tgt_ = c;
  }
  inline virtual void ctrlExternalDtx(int i, uint160 o)
  {
    this->l7_ = i;
    this->o_ = o;
  }
  inline virtual bool ctrlExternalAngle()
  {
    return this->l7_ == RAY_VTX;
  }
  inline virtual bool ctrlExternalDtx()
  {
    return this->l7_ == RAY_SET;
  }
  inline virtual int ctrlIndex()
  {
    return this->l7_ ;
  }
  inline virtual uint160 ctrlPath()
  {
    return this->o_ ;
  }

  inline virtual void ctrlDtx(std::string& o)
  {
    this->vtx_ = o;
  }
  inline virtual void streamID(std::vector<unsigned char> o)
  {
    this->stream_id = o;
  }
  inline virtual std::vector<unsigned char> streamID()
  {
    return this->stream_id;
  }

  IMPLEMENT_SERIALIZE
  (
    READWRITE(this->tgt_);
    READWRITE(this->o_);
    READWRITE(this->l7_);
    READWRITE(this->vtx_);
    READWRITE(this->stream_id);
  )

private:
  void init()
  {
    stream_id.resize(0x20);
  }

  int l7_;
  std::string tgt_;
  uint160 o_;
  std::string vtx_;
  std::vector<unsigned char> stream_id;
};

