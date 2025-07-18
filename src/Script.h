#pragma once

#include <string>
#include <vector>

#include <stdint.h>

#include <boost/foreach.hpp>
#include <boost/variant.hpp>

#include "KeyStore.h"
#include "BigNum.h"


typedef std::vector<unsigned char> valtype;

class CTransaction;

std::string Hash160ToAddress(uint160 hash160);
extern bool AddressToHash160(const char* psz, uint160& hash160Ret);
extern bool AddressToHash160(const std::string& str, uint160& hash160Ret);
static const unsigned int MAX_SCRIPT_ELEMENT_SIZE = 1000000;
static const unsigned int MAX_OP_RETURN_RELAY = 40;


enum
{
  SIGHASH_ALL = 1,
  SIGHASH_NONE = 2,
  SIGHASH_SINGLE = 3,
  SIGHASH_ANYONECANPAY = 0x80,
};


enum
{
  SCRIPT_VERIFY_NONE = 0,
  SCRIPT_VERIFY_NOCACHE = (1U << 0),
  SCRIPT_VERIFY_NULLDUMMY = (1U << 1),
# 57 "script.h"
  SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS = (1U << 2),

  SCRIPT_VERIFY_STRICTENC = (1U << 3),

  SCRIPT_VERIFY_ALLOW_EMPTY_SIG = (1U << 4),
  SCRIPT_VERIFY_FIX_HASHTYPE = (1U << 5),



  SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 6),
};






static const unsigned int MANDATORY_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_NULLDUMMY |
    SCRIPT_VERIFY_STRICTENC |
    SCRIPT_VERIFY_ALLOW_EMPTY_SIG |
    SCRIPT_VERIFY_FIX_HASHTYPE |
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;




static const unsigned int STANDARD_SCRIPT_VERIFY_FLAGS = MANDATORY_SCRIPT_VERIFY_FLAGS |
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS;


static const unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS;

enum txnouttype
{
  TX_NONSTANDARD,

  TX_PUBKEY,
  TX_PUBKEYHASH,
  TX_SCRIPTHASH,
  TX_MULTISIG,
  TX_NULL_DATA,
};

class CNoDestination
{
public:
  friend bool operator==(const CNoDestination &a, const CNoDestination &b)
  {
    return true;
  }
  friend bool operator<(const CNoDestination &a, const CNoDestination &b)
  {
    return true;
  }
};







typedef boost::variant<CNoDestination, CKeyID, CScriptID> CTxDestination;

const char* GetTxnOutputType(txnouttype t);


enum opcodetype
{

  OP_0 = 0x00,
  OP_FALSE = OP_0,
  OP_PUSHDATA1 = 0x4c,
  OP_PUSHDATA2 = 0x4d,
  OP_PUSHDATA4 = 0x4e,
  OP_1NEGATE = 0x4f,
  OP_RESERVED = 0x50,
  OP_1 = 0x51,
  OP_TRUE=OP_1,
  OP_2 = 0x52,
  OP_3 = 0x53,
  OP_4 = 0x54,
  OP_5 = 0x55,
  OP_6 = 0x56,
  OP_7 = 0x57,
  OP_8 = 0x58,
  OP_9 = 0x59,
  OP_10 = 0x5a,
  OP_11 = 0x5b,
  OP_12 = 0x5c,
  OP_13 = 0x5d,
  OP_14 = 0x5e,
  OP_15 = 0x5f,
  OP_16 = 0x60,


  OP_NOP = 0x61,
  OP_VER = 0x62,
  OP_IF = 0x63,
  OP_NOTIF = 0x64,
  OP_VERIF = 0x65,
  OP_VERNOTIF = 0x66,
  OP_ELSE = 0x67,
  OP_ENDIF = 0x68,
  OP_VERIFY = 0x69,
  OP_RETURN = 0x6a,


  OP_TOALTSTACK = 0x6b,
  OP_FROMALTSTACK = 0x6c,
  OP_2DROP = 0x6d,
  OP_2DUP = 0x6e,
  OP_3DUP = 0x6f,
  OP_2OVER = 0x70,
  OP_2ROT = 0x71,
  OP_2SWAP = 0x72,
  OP_IFDUP = 0x73,
  OP_DEPTH = 0x74,
  OP_DROP = 0x75,
  OP_DUP = 0x76,
  OP_NIP = 0x77,
  OP_OVER = 0x78,
  OP_PICK = 0x79,
  OP_ROLL = 0x7a,
  OP_ROT = 0x7b,
  OP_SWAP = 0x7c,
  OP_TUCK = 0x7d,


  OP_CAT = 0x7e,
  OP_SUBSTR = 0x7f,
  OP_LEFT = 0x80,
  OP_RIGHT = 0x81,
  OP_SIZE = 0x82,


  OP_INVERT = 0x83,
  OP_AND = 0x84,
  OP_OR = 0x85,
  OP_XOR = 0x86,
  OP_EQUAL = 0x87,
  OP_EQUALVERIFY = 0x88,
  OP_RESERVED1 = 0x89,
  OP_RESERVED2 = 0x8a,


  OP_1ADD = 0x8b,
  OP_1SUB = 0x8c,
  OP_2MUL = 0x8d,
  OP_2DIV = 0x8e,
  OP_NEGATE = 0x8f,
  OP_ABS = 0x90,
  OP_NOT = 0x91,
  OP_0NOTEQUAL = 0x92,

  OP_ADD = 0x93,
  OP_SUB = 0x94,
  OP_MUL = 0x95,
  OP_DIV = 0x96,
  OP_MOD = 0x97,
  OP_LSHIFT = 0x98,
  OP_RSHIFT = 0x99,

  OP_BOOLAND = 0x9a,
  OP_BOOLOR = 0x9b,
  OP_NUMEQUAL = 0x9c,
  OP_NUMEQUALVERIFY = 0x9d,
  OP_NUMNOTEQUAL = 0x9e,
  OP_LESSTHAN = 0x9f,
  OP_GREATERTHAN = 0xa0,
  OP_LESSTHANOREQUAL = 0xa1,
  OP_GREATERTHANOREQUAL = 0xa2,
  OP_MIN = 0xa3,
  OP_MAX = 0xa4,

  OP_WITHIN = 0xa5,


  OP_RIPEMD160 = 0xa6,
  OP_SHA1 = 0xa7,
  OP_SHA256 = 0xa8,
  OP_HASH160 = 0xa9,
  OP_HASH256 = 0xaa,
  OP_CODESEPARATOR = 0xab,
  OP_CHECKSIG = 0xac,
  OP_CHECKSIGVERIFY = 0xad,
  OP_CHECKMULTISIG = 0xae,
  OP_CHECKMULTISIGVERIFY = 0xaf,


  OP_NOP1 = 0xb0,
  OP_CHECKLOCKTIMEVERIFY = 0xb1,
  OP_NOP2 = OP_CHECKLOCKTIMEVERIFY,
  OP_NOP3 = 0xb2,
  OP_NOP4 = 0xb3,
  OP_NOP5 = 0xb4,
  OP_NOP6 = 0xb5,
  OP_NOP7 = 0xb6,
  OP_NOP8 = 0xb7,
  OP_NOP9 = 0xb8,
  OP_NOP10 = 0xb9,


  OP_SMALLDATA = 0xf9,
  OP_SMALLINTEGER = 0xfa,
  OP_PUBKEYS = 0xfb,
  OP_PUBKEYHASH = 0xfd,
  OP_PUBKEY = 0xfe,

  OP_INVALIDOPCODE = 0xff,
};

const char* GetOpName(opcodetype opcode);

class scriptnum_error : public std::runtime_error
{
public:
  explicit scriptnum_error(const std::string& str) : std::runtime_error(str) {}
};

class CScriptNum
{
# 287 "script.h"
public:

  explicit CScriptNum(const int64_t& n)
  {
    m_value = n;
  }

  static const size_t nDefaultMaxNumSize = 4;
  explicit CScriptNum(const std::vector<unsigned char>& vch, bool fRequireMinimal,
                      const size_t nMaxNumSize = nDefaultMaxNumSize)
  {
    if (vch.size() > nMaxNumSize)
    {
      throw scriptnum_error("script number overflow");
    }
    if (fRequireMinimal && vch.size() > 0)
    {






      if ((vch.back() & 0x7f) == 0)
      {





        if (vch.size() <= 1 || (vch[vch.size() - 2] & 0x80) == 0)
        {
          throw scriptnum_error("non-minimally encoded script number");
        }
      }
    }
    m_value = set_vch(vch);
  }

  inline bool operator==(const int64_t& rhs) const
  {
    return m_value == rhs;
  }
  inline bool operator!=(const int64_t& rhs) const
  {
    return m_value != rhs;
  }
  inline bool operator<=(const int64_t& rhs) const
  {
    return m_value <= rhs;
  }
  inline bool operator< (const int64_t& rhs) const
  {
    return m_value < rhs;
  }
  inline bool operator>=(const int64_t& rhs) const
  {
    return m_value >= rhs;
  }
  inline bool operator> (const int64_t& rhs) const
  {
    return m_value > rhs;
  }

  inline bool operator==(const CScriptNum& rhs) const
  {
    return operator==(rhs.m_value);
  }
  inline bool operator!=(const CScriptNum& rhs) const
  {
    return operator!=(rhs.m_value);
  }
  inline bool operator<=(const CScriptNum& rhs) const
  {
    return operator<=(rhs.m_value);
  }
  inline bool operator< (const CScriptNum& rhs) const
  {
    return operator< (rhs.m_value);
  }
  inline bool operator>=(const CScriptNum& rhs) const
  {
    return operator>=(rhs.m_value);
  }
  inline bool operator> (const CScriptNum& rhs) const
  {
    return operator> (rhs.m_value);
  }

  inline CScriptNum operator+( const int64_t& rhs) const
  {
    return CScriptNum(m_value + rhs);
  }
  inline CScriptNum operator-( const int64_t& rhs) const
  {
    return CScriptNum(m_value - rhs);
  }
  inline CScriptNum operator+( const CScriptNum& rhs) const
  {
    return operator+(rhs.m_value);
  }
  inline CScriptNum operator-( const CScriptNum& rhs) const
  {
    return operator-(rhs.m_value);
  }

  inline CScriptNum& operator+=( const CScriptNum& rhs)
  {
    return operator+=(rhs.m_value);
  }
  inline CScriptNum& operator-=( const CScriptNum& rhs)
  {
    return operator-=(rhs.m_value);
  }

  inline CScriptNum operator&( const int64_t& rhs) const
  {
    return CScriptNum(m_value & rhs);
  }
  inline CScriptNum operator&( const CScriptNum& rhs) const
  {
    return operator&(rhs.m_value);
  }

  inline CScriptNum& operator&=( const CScriptNum& rhs)
  {
    return operator&=(rhs.m_value);
  }

  inline CScriptNum operator-() const
  {
    assert(m_value != std::numeric_limits<int64_t>::min());
    return CScriptNum(-m_value);
  }

  inline CScriptNum& operator=( const int64_t& rhs)
  {
    m_value = rhs;
    return *this;
  }

  inline CScriptNum& operator+=( const int64_t& rhs)
  {
    assert(rhs == 0 || (rhs > 0 && m_value <= std::numeric_limits<int64_t>::max() - rhs) ||
           (rhs < 0 && m_value >= std::numeric_limits<int64_t>::min() - rhs));
    m_value += rhs;
    return *this;
  }

  inline CScriptNum& operator-=( const int64_t& rhs)
  {
    assert(rhs == 0 || (rhs > 0 && m_value >= std::numeric_limits<int64_t>::min() + rhs) ||
           (rhs < 0 && m_value <= std::numeric_limits<int64_t>::max() + rhs));
    m_value -= rhs;
    return *this;
  }

  inline CScriptNum& operator&=( const int64_t& rhs)
  {
    m_value &= rhs;
    return *this;
  }

  int getint() const
  {
    if (m_value > std::numeric_limits<int>::max())
    {
      return std::numeric_limits<int>::max();
    }
    else if (m_value < std::numeric_limits<int>::min())
    {
      return std::numeric_limits<int>::min();
    }
    return m_value;
  }

  std::vector<unsigned char> getvch() const
  {
    return serialize(m_value);
  }

  static std::vector<unsigned char> serialize(const int64_t& value)
  {
    if(value == 0)
    {
      return std::vector<unsigned char>();
    }

    std::vector<unsigned char> result;
    const bool neg = value < 0;
    uint64_t absvalue = neg ? -value : value;

    while(absvalue)
    {
      result.push_back(absvalue & 0xff);
      absvalue >>= 8;
    }
# 495 "script.h"
    if (result.back() & 0x80)
    {
      result.push_back(neg ? 0x80 : 0);
    }
    else if (neg)
    {
      result.back() |= 0x80;
    }

    return result;
  }

private:
  static int64_t set_vch(const std::vector<unsigned char>& vch)
  {
    if (vch.empty())
    {
      return 0;
    }

    int64_t result = 0;
    for (size_t i = 0; i != vch.size(); ++i)
    {
      result |= static_cast<int64_t>(vch[i]) << 8*i;
    }



    if (vch.back() & 0x80)
    {
      return -((int64_t)(result & ~(0x80ULL << (8 * (vch.size() - 1)))));
    }

    return result;
  }

  int64_t m_value;
};

inline std::string ValueString(const std::vector<unsigned char>& vch)
{
  if (vch.size() <= 4)
  {
    return strprintf("%d", CBigNum(vch).getint());
  }
  else
  {
    return HexStr(vch);
  }
}

inline std::string StackString(const std::vector<std::vector<unsigned char> >& vStack)
{
  std::string str;
  BOOST_FOREACH(const std::vector<unsigned char>& vch, vStack)
  {
    if (!str.empty())
    {
      str += " ";
    }
    str += ValueString(vch);
  }
  return str;
}
# 568 "script.h"
class CScript : public std::vector<unsigned char>
{
protected:
  CScript& push_int64(int64_t n)
  {
    if (n == -1 || (n >= 1 && n <= 16))
    {
      push_back(n + (OP_1 - 1));
    }
    else
    {
      CBigNum bn(n);
      *this << bn.getvch();
    }
    return *this;
  }

  CScript& push_uint64(uint64_t n)
  {
    if (n >= 1 && n <= 16)
    {
      push_back(n + (OP_1 - 1));
    }
    else
    {
      CBigNum bn(n);
      *this << bn.getvch();
    }
    return *this;
  }

public:
  CScript() { }
  CScript(const CScript& b) : std::vector<unsigned char>(b.begin(), b.end()) { }
  CScript(const_iterator pbegin, const_iterator pend) : std::vector<unsigned char>(pbegin, pend) { }
#ifndef _MSC_VER
  CScript(const unsigned char* pbegin, const unsigned char* pend) : std::vector<unsigned char>(pbegin, pend) { }
#endif

  CScript& operator=(const CScript& b)
  {
	  std::vector<unsigned char>::operator=(b);
    return *this;
  } 

  CScript& operator+=(const CScript& b)
  {
    insert(end(), b.begin(), b.end());
    return *this;
  }

  friend CScript operator+(const CScript& a, const CScript& b)
  {
    CScript ret = a;
    ret += b;
    return ret;
  }



  explicit CScript(signed char b)
  {
    operator<<(b);
  }
  explicit CScript(short b)
  {
    operator<<(b);
  }
  explicit CScript(int b)
  {
    operator<<(b);
  }
  explicit CScript(long b)
  {
    operator<<(b);
  }
  explicit CScript(long long b)
  {
    operator<<(b);
  }
  explicit CScript(unsigned char b)
  {
    operator<<(b);
  }
  explicit CScript(unsigned int b)
  {
    operator<<(b);
  }
  explicit CScript(unsigned short b)
  {
    operator<<(b);
  }
  explicit CScript(unsigned long b)
  {
    operator<<(b);
  }
  explicit CScript(unsigned long long b)
  {
    operator<<(b);
  }

  explicit CScript(opcodetype b)
  {
    operator<<(b);
  }
  explicit CScript(const uint256& b)
  {
    operator<<(b);
  }
  explicit CScript(const CBigNum& b)
  {
    operator<<(b);
  }
  explicit CScript(const std::vector<unsigned char>& b)
  {
    operator<<(b);
  }



  CScript& operator<<(signed char b)
  {
    return push_int64(b);
  }
  CScript& operator<<(short b)
  {
    return push_int64(b);
  }
  CScript& operator<<(int b)
  {
    return push_int64(b);
  }
  CScript& operator<<(long b)
  {
    return push_int64(b);
  }
  CScript& operator<<(long long b)
  {
    return push_int64(b);
  }
  CScript& operator<<(unsigned char b)
  {
    return push_uint64(b);
  }
  CScript& operator<<(unsigned int b)
  {
    return push_uint64(b);
  }
  CScript& operator<<(unsigned short b)
  {
    return push_uint64(b);
  }
  CScript& operator<<(unsigned long b)
  {
    return push_uint64(b);
  }
  CScript& operator<<(unsigned long long b)
  {
    return push_uint64(b);
  }

  CScript& operator<<(opcodetype opcode)
  {
    if (opcode < 0 || opcode > 0xff)
    {
      throw std::runtime_error("CScript::operator<<() : invalid opcode");
    }
    insert(end(), (unsigned char)opcode);
    return *this;
  }

  CScript& operator<<(const uint160& b)
  {
    insert(end(), sizeof(b));
    insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
    return *this;
  }

  CScript& operator<<(const uint256& b)
  {
    insert(end(), sizeof(b));
    insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
    return *this;
  }

  CScript& operator<<(const CPubKey& key)
  {
    std::vector<unsigned char> vchKey = key.Raw();
    return (*this) << vchKey;
  }

  CScript& operator<<(const CBigNum& b)
  {
    *this << b.getvch();
    return *this;
  }

  CScript& operator<<(const std::vector<unsigned char>& b)
  {
    if (b.size() < OP_PUSHDATA1)
    {
      insert(end(), (unsigned char)b.size());
    }
    else if (b.size() <= 0xff)
    {
      insert(end(), OP_PUSHDATA1);
      insert(end(), (unsigned char)b.size());
    }
    else if (b.size() <= 0xffff)
    {
      insert(end(), OP_PUSHDATA2);
      unsigned short nSize = b.size();
      insert(end(), (unsigned char*)&nSize, (unsigned char*)&nSize + sizeof(nSize));
    }
    else
    {
      insert(end(), OP_PUSHDATA4);
      unsigned int nSize = b.size();
      insert(end(), (unsigned char*)&nSize, (unsigned char*)&nSize + sizeof(nSize));
    }
    insert(end(), b.begin(), b.end());
    return *this;
  }

  CScript& operator<<(const CScript& b)
  {


    assert(!"Warning: Pushing a CScript onto a CScript with << is probably not intended, use + to concatenate!");
    return *this;
  }


  bool GetOp(iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>& vchRet)
  {

    const_iterator pc2 = pc;
    bool fRet = GetOp2(pc2, opcodeRet, &vchRet);
    pc = begin() + (pc2 - begin());
    return fRet;
  }

  bool GetOp(iterator& pc, opcodetype& opcodeRet)
  {
    const_iterator pc2 = pc;
    bool fRet = GetOp2(pc2, opcodeRet, NULL);
    pc = begin() + (pc2 - begin());
    return fRet;
  }

  bool GetOp(const_iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>& vchRet) const
  {
    return GetOp2(pc, opcodeRet, &vchRet);
  }

  bool GetOp(const_iterator& pc, opcodetype& opcodeRet) const
  {
    return GetOp2(pc, opcodeRet, NULL);
  }

  bool GetOp2(const_iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>* pvchRet) const
  {
    opcodeRet = OP_INVALIDOPCODE;
    if (pvchRet)
    {
      pvchRet->clear();
    }
    if (pc >= end())
    {
      return false;
    }


    if (end() - pc < 1)
    {
      return false;
    }
    unsigned int opcode = *pc++;


    if (opcode <= OP_PUSHDATA4)
    {
      unsigned int nSize;
      if (opcode < OP_PUSHDATA1)
      {
        nSize = opcode;
      }
      else if (opcode == OP_PUSHDATA1)
      {
        if (end() - pc < 1)
        {
          return false;
        }
        nSize = *pc++;
      }
      else if (opcode == OP_PUSHDATA2)
      {
        if (end() - pc < 2)
        {
          return false;
        }
        nSize = 0;
        memcpy(&nSize, &pc[0], 2);
        pc += 2;
      }
      else if (opcode == OP_PUSHDATA4)
      {
        if (end() - pc < 4)
        {
          return false;
        }
        memcpy(&nSize, &pc[0], 4);
        pc += 4;
      }
      if (end() - pc < 0 || (unsigned int)(end() - pc) < nSize)
      {
        return false;
      }
      if (pvchRet)
      {
        pvchRet->assign(pc, pc + nSize);
      }
      pc += nSize;
    }

    opcodeRet = (opcodetype)opcode;
    return true;
  }


  static int DecodeOP_N(opcodetype opcode)
  {
    if (opcode == OP_0)
    {
      return 0;
    }
    assert(opcode >= OP_1 && opcode <= OP_16);
    return (int)opcode - (int)(OP_1 - 1);
  }
  static opcodetype EncodeOP_N(int n)
  {
    assert(n >= 0 && n <= 16);
    if (n == 0)
    {
      return OP_0;
    }
    return (opcodetype)(OP_1+n-1);
  }

  int FindAndDelete(const CScript& b)
  {
    int nFound = 0;
    if (b.empty())
    {
      return nFound;
    }
    iterator pc = begin();
    opcodetype opcode;
    do
    {
      while (end() - pc >= (long)b.size() && memcmp(&pc[0], &b[0], b.size()) == 0)
      {
        erase(pc, pc + b.size());
        ++nFound;
      }
    }
    while (GetOp(pc, opcode));
    return nFound;
  }
  int Find(opcodetype op) const
  {
    int nFound = 0;
    opcodetype opcode;
    for (const_iterator pc = begin(); pc != end() && GetOp(pc, opcode);)
      if (opcode == op)
      {
        ++nFound;
      }
    return nFound;
  }






  unsigned int GetSigOpCount(bool fAccurate) const;



  unsigned int GetSigOpCount(const CScript& scriptSig) const;

  uint160 GetBitcoinAddressHash160() const;
  void SetBitcoinAddress(const std::vector<unsigned char>& vchPubKey)
  {
    SetBitcoinAddress(Hash160(vchPubKey));
  }

  void SetBitcoinAddress(const uint160& hash160)
  {
    this->clear();
    *this << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;
  }
  bool SetBitcoinAddress(const std::string& strAddress)
  {
    this->clear();
    uint160 hash160;
    if (!AddressToHash160(strAddress, hash160))
    {
      return false;
    }
    SetBitcoinAddress(hash160);
    return true;
  }

  std::string GetBitcoinAddress() const
  {
    uint160 hash160 = GetBitcoinAddressHash160();
    if (hash160 == 0)
    {
      return "";
    }
    return Hash160ToAddress(hash160);
  }

  bool IsPayToScriptHash() const;


  bool IsPushOnly() const
  {
    const_iterator pc = begin();
    while (pc < end())
    {
      opcodetype opcode;
      if (!GetOp(pc, opcode))
      {
        return false;
      }
      if (opcode > OP_16)
      {
        return false;
      }
    }
    return true;
  }


  bool HasCanonicalPushes() const;

  void SetDestination(const CTxDestination& address);
  void SetMultisig(int nRequired, const std::vector<CKey>& keys);

  std::string ToString(bool fShort=false) const
  {
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    const_iterator pc = begin();
    while (pc < end())
    {
      if (!str.empty())
      {
        str += " ";
      }
      if (!GetOp(pc, opcode, vch))
      {
        str += "[error]";
        return str;
      }
      if (0 <= opcode && opcode <= OP_PUSHDATA4)
      {
        str += ValueString(vch);
      }
      else
      {
        str += GetOpName(opcode);
      }
    }
    return str;
  }

  CScriptID GetID() const
  {
    return CScriptID(Hash160(*this));
  }
};





bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, int flags, int nHashType);
bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet);
int ScriptSigArgsExpected(txnouttype t, const std::vector<std::vector<unsigned char> >& vSolutions);
bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType);
bool IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);
bool IsMine(const CKeyStore& keystore, const CTxDestination &dest);
void ExtractAffectedKeys(const CKeyStore &keystore, const CScript& scriptPubKey, std::vector<CKeyID> &vKeys);
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);
bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet);
bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool SignSignature(const CKeyStore& keystore, const CTransaction& txFrom, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, int flags,
                  int nHashType);
bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, int flags, int nHashType);



CScript CombineSignatures(CScript scriptPubKey, const CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);
bool ExtractPubKey(const CScript& scriptPubKey, const CKeyStore* pkeystore, std::vector<unsigned char>& vchPubKeyRet);
bool ExtractHash160(const CScript& scriptPubKey, uint160& hash160Ret);

