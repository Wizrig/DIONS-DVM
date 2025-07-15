#pragma once

#include "boost/random/mersenne_twister.hpp"
#include "boost/math/constants/constants.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include "boost/multiprecision/cpp_dec_float.hpp"
#include "boost/random/independent_bits.hpp"

#include <vector>
#include <map>
#include<tuple>





typedef boost::random::independent_bits_engine<boost::random::mt19937, 256, boost::multiprecision::cpp_int> GEN__;
typedef struct displ
{
  int sect_;
  GEN__ strm_;
  boost::multiprecision::cpp_int offset_;
  boost::multiprecision::cpp_int gen_mat_test_;
  boost::multiprecision::cpp_dec_float_50 scale_;
  boost::multiprecision::cpp_dec_float_50 range_;
  std::vector<int> coord_;
} view;



typedef struct ex_mix
{
  int pos_;
  GEN__ g_;
  std::vector<displ> descTable_;
} mix;


typedef struct mtx
{
  ex_mix disc_;
  ex_mix codom_;
  ex_mix codom_mtx_;
} mtx_co;

typedef struct FI1__
{
  ex_mix r;
  ex_mix th;
  ex_mix ph;
  ex_mix t;
} fi1;



typedef struct FI2__
{
  ex_mix desc_;
  ex_mix path_;
  ex_mix desc1_;
  std::map<FI1__, displ> transMap_;
  std::map<FI1__, displ> internMap_;
  std::map<FI2__, displ> extTransMap_;
  std::map<FI2__, displ> torTransMap_;
} fi2;

typedef struct R1_mtx_rotate
{
  FI2__ alpha_;
  FI2__ beta_;
  FI2__ gamma_;

  FI1__ res_;
  FI2__ reference_;
  FI2__ basis_;
} rot;


typedef struct TransitionElement
{
  std::vector<double> ent_indicator_;
  FI2__ key_center_;
  std::vector<R1_mtx_rotate> morph_l_;
  std::vector<FI1__> reference_;
  std::vector<FI2__> cbase_;
  std::vector<FI2__> key_l_;
} transelt;

typedef struct Spectra
{
  FI1__ basis_;
  FI2__ mix_;
  FI2__ dim_;
} spec;

std::vector<double> f_dist(std::vector<unsigned char>& in);
double s_entropy(std::vector<double> v);

void trans(std::vector<unsigned char>, unsigned char (*f)(unsigned char));
void transHom(std::vector<unsigned char>, unsigned char (*f)(unsigned char), unsigned char (*g)(unsigned char));
void transHomExt(std::vector<unsigned char>, unsigned char (*f)(unsigned char), unsigned char (*g)(unsigned char));
class SpecExec
{
public:
  SpecExec() {}
  ~SpecExec() {}

  virtual double entropy(std::vector<double> v);
  virtual double sect(std::vector<double> v);
  virtual double sect_outer(std::vector<double> v);
  virtual double trans_ext(std::vector<double> v);

};

class TLV
{
public:
  TLV() {};
  ~TLV() {};
  TransitionElement trans;
  TransitionElement co_dom_;
  TransitionElement trans_gnd_;
  TransitionElement trans_atom_;
  TransitionElement trans_ion_;
  TransitionElement trans_ex_;
  SpecExec se;
  SpecExec list;
  SpecExec trans_base;
  SpecExec trans_gcd_ext;
private:
  FI1__ mix;
};

double ic(const std::string& );

void trans(std::vector<unsigned char>& data, unsigned char (*f)(unsigned char));
double s_entropy(std::vector<double>&) ;
void switchIO(unsigned char (*p)(unsigned char, unsigned char), unsigned char);
void transHom(std::vector<unsigned char>&, unsigned char (*f)(unsigned char), unsigned char (*g)(unsigned char));
void multiChan(unsigned char* (*p)(unsigned char, unsigned char), unsigned char);
void hPerm(int s, int n, void (*p)(int), void (*inv)(int, int), void (*center)(int));
double sw(double weight, int i, int j, int (*inv)(int, int));
void rms(const std::string&, std::string& );
std::vector<double> f_dist(std::vector<unsigned char>&);
void transHomExt(std::vector<unsigned char>&, unsigned char (*f)(unsigned char), unsigned char (*g)(unsigned char));
int outer_sect(int (*s)(int), int (*t)(int), int, int);



