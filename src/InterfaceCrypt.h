#pragma once


class InterfaceCrypt
{
public:
  virtual int sig() = 0;
  virtual void open() = 0;
  virtual void close() = 0;
  virtual std::string alias() = 0;
  virtual std::string ctrl_() = 0;
  virtual void ctrl(std::string& c) = 0;
};

