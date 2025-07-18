#pragma once
#include "validation/interface/Validator.h"

class Base : public Validator
{
  public:
    Base() = default;
    ~Base() = default;

    virtual bool validate() override;

  private:
};
