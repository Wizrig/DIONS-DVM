
#include "db.h"
#include "Net.h"
#include "Miner.h"
#include "wallet/Wallet.h"
#include "core/strlcpy.h"
#include "AddrMan.h"
#include "UIInterface.h"
#include <memory>
#ifdef WIN32
#include <string.h>
#endif
using namespace std;
using namespace boost;
static const int MAX_OUTBOUND_CONNECTIONS = 16;
extern __wx__* pwalletMainId;
void StartShutdown();
void ThreadMessageHandler2(void* parg);
void ThreadSocketHandler2(void* parg);
void ThreadOpenConnections2(void* parg);
void ThreadOpenAddedConnections2(void* parg);
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, const char *strDest = NULL, bool fOneShot = false);
extern string strDNSSeedNode;
struct LocalServiceInfo
{
  int nScore;
  int nPort;
};
bool fDiscover = true;
bool fUseUPnP = false;
uint64_t nLocalServices = NODE_NETWORK;
static CCriticalSection cs_mapLocalHost;
static map<CNetAddr, LocalServiceInfo> mapLocalHost;
static bool vfReachable[NET_MAX] = {};
static bool vfLimited[NET_MAX] = {};
static CNodeRef pnodeLocalHost;
CAddress addrSeenByPeer(CService("0.0.0.0", 0), nLocalServices);
uint64_t nLocalHostNonce = 0;
boost::array<int, THREAD_MAX> vnThreadsRunning;
static std::vector<SOCKET> vhListenSocket;
CAddrMan addrman;
vector<CNodeRef> vNodes;
CCriticalSection cs_vNodes;
map<CInv, CDataStream> mapRelay;
deque<pair<int64_t, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
map<CInv, int64_t> mapAlreadyAskedFor;
static deque<string> vOneShots;
CCriticalSection cs_vOneShots;
set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;
static CSemaphore *semOutbound = NULL;
void AddOneShot(string strDest)
{
  LOCK(cs_vOneShots);
  vOneShots.push_back(strDest);
}
unsigned short GetListenPort()
{
  return (unsigned short)(GetArg("-port", GetDefaultPort()));
}
void CNode::PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd)
{
  if (pindexBegin == pindexLastGetBlocksBegin && hashEnd == hashLastGetBlocksEnd)
  {
    return;
  }

  pindexLastGetBlocksBegin = pindexBegin;
  hashLastGetBlocksEnd = hashEnd;
  PushMessage("getblocks", CBlockLocator(pindexBegin), hashEnd);
}
bool GetLocal(CService& addr, const CNetAddr *paddrPeer)
{
  if (fNoListen)
  {
    return false;
  }

  int nBestScore = -1;
  int nBestReachability = -1;
  {
    LOCK(cs_mapLocalHost);

    for (map<CNetAddr, LocalServiceInfo>::iterator it = mapLocalHost.begin(); it != mapLocalHost.end(); it++)
    {
      int nScore = (*it).second.nScore;
      int nReachability = (*it).first.GetReachabilityFrom(paddrPeer);

      if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore))
      {
        addr = CService((*it).first, (*it).second.nPort);
        nBestReachability = nReachability;
        nBestScore = nScore;
      }
    }
  }
  return nBestScore >= 0;
}
CAddress GetLocalAddress(const CNetAddr *paddrPeer)
{
  CAddress ret(CService("0.0.0.0",0),0);
  CService addr;

  if (GetLocal(addr, paddrPeer))
  {
    ret = CAddress(addr);
    ret.nServices = nLocalServices;
    ret.nTime = GetAdjustedTime();
  }

  return ret;
}
bool RecvLine(SOCKET hSocket, string& strLine)
{
  strLine = "";

  while (true)
  {
    char c;
    int nBytes = recv(hSocket, &c, 1, 0);

    if (nBytes > 0)
    {
      if (c == '\n')
      {
        continue;
      }

      if (c == '\r')
      {
        return true;
      }

      strLine += c;

      if (strLine.size() >= 9000)
      {
        return true;
      }
    }
    else if (nBytes <= 0)
    {
      if (fShutdown)
      {
        return false;
      }

      if (nBytes < 0)
      {
        int nErr = WSAGetLastError();

        if (nErr == WSAEMSGSIZE)
        {
          continue;
        }

        if (nErr == WSAEWOULDBLOCK || nErr == WSAEINTR || nErr == WSAEINPROGRESS)
        {
          MilliSleep(10);
          continue;
        }
      }

      if (!strLine.empty())
      {
        return true;
      }

      if (nBytes == 0)
      {
        printf("socket closed\n");
        return false;
      }
      else
      {
        int nErr = WSAGetLastError();
        printf("recv failed: %d\n", nErr);
        return false;
      }
    }
  }
}
void static AdvertizeLocal()
{
  LOCK(cs_vNodes);
  BOOST_FOREACH(CNodeRef pnode, vNodes)
  {
    if (pnode->fSuccessfullyConnected)
    {
      CAddress addrLocal = GetLocalAddress(&pnode->addr);

      if (addrLocal.IsRoutable() && (CService)addrLocal != (CService)pnode->addrLocal)
      {
        pnode->PushAddress(addrLocal);
        pnode->addrLocal = addrLocal;
      }
    }
  }
}
void SetReachable(enum Network net, bool fFlag)
{
  LOCK(cs_mapLocalHost);
  vfReachable[net] = fFlag;

  if (net == NET_IPV6 && fFlag)
  {
    vfReachable[NET_IPV4] = true;
  }
}
bool AddLocal(const CService& addr, int nScore)
{
  if (!addr.IsRoutable())
  {
    return false;
  }

  if (!fDiscover && nScore < LOCAL_MANUAL)
  {
    return false;
  }

  if (IsLimited(addr))
  {
    return false;
  }

  printf("AddLocal(%s,%i)\n", addr.ToString().c_str(), nScore);
  {
    LOCK(cs_mapLocalHost);
    bool fAlready = mapLocalHost.count(addr) > 0;
    LocalServiceInfo &info = mapLocalHost[addr];

    if (!fAlready || nScore >= info.nScore)
    {
      info.nScore = nScore + (fAlready ? 1 : 0);
      info.nPort = addr.GetPort();
    }

    SetReachable(addr.GetNetwork());
  }
  AdvertizeLocal();
  return true;
}
bool AddLocal(const CNetAddr &addr, int nScore)
{
  return AddLocal(CService(addr, GetListenPort()), nScore);
}
void SetLimited(enum Network net, bool fLimited)
{
  if (net == NET_UNROUTABLE)
  {
    return;
  }

  LOCK(cs_mapLocalHost);
  vfLimited[net] = fLimited;
}
bool IsLimited(enum Network net)
{
  LOCK(cs_mapLocalHost);
  return vfLimited[net];
}
bool IsLimited(const CNetAddr &addr)
{
  return IsLimited(addr.GetNetwork());
}
bool SeenLocal(const CService& addr)
{
  {
    LOCK(cs_mapLocalHost);

    if (mapLocalHost.count(addr) == 0)
    {
      return false;
    }

    mapLocalHost[addr].nScore++;
  }
  AdvertizeLocal();
  return true;
}
bool IsLocal(const CService& addr)
{
  LOCK(cs_mapLocalHost);
  return mapLocalHost.count(addr) > 0;
}
bool IsReachable(const CNetAddr& addr)
{
  LOCK(cs_mapLocalHost);
  enum Network net = addr.GetNetwork();
  return vfReachable[net] && !vfLimited[net];
}
bool GetMyExternalIP2(const CService& addrConnect, const char* pszGet, const char* pszKeyword, CNetAddr& ipRet)
{
  SOCKET hSocket;

  if (!ConnectSocket(addrConnect, hSocket))
  {
    return error("GetMyExternalIP() : connection to %s failed", addrConnect.ToString().c_str());
  }

  send(hSocket, pszGet, strlen(pszGet), MSG_NOSIGNAL);
  string strLine;

  while (RecvLine(hSocket, strLine))
  {
    if (strLine.empty())
    {
      while (true)
      {
        if (!RecvLine(hSocket, strLine))
        {
          closesocket(hSocket);
          return false;
        }

        if (pszKeyword == NULL)
        {
          break;
        }

        if (strLine.find(pszKeyword) != string::npos)
        {
          strLine = strLine.substr(strLine.find(pszKeyword) + strlen(pszKeyword));
          break;
        }
      }

      closesocket(hSocket);

      if (strLine.find("<") != string::npos)
      {
        strLine = strLine.substr(0, strLine.find("<"));
      }

      strLine = strLine.substr(strspn(strLine.c_str(), " \t\n\r"));

      while (strLine.size() > 0 && isspace(strLine[strLine.size()-1]))
      {
        strLine.resize(strLine.size()-1);
      }

      CService addr(strLine,0,true);
      printf("GetMyExternalIP() received [%s] %s\n", strLine.c_str(), addr.ToString().c_str());

      if (!addr.IsValid() || !addr.IsRoutable())
      {
        return false;
      }

      ipRet.SetIP(addr);
      return true;
    }
  }

  closesocket(hSocket);
  return error("GetMyExternalIP() : connection closed");
}
bool GetMyExternalIP(CNetAddr& ipRet)
{
  CService addrConnect;
  const char* pszGet;
  const char* pszKeyword;

  for (int nLookup = 0; nLookup <= 1; nLookup++)
    for (int nHost = 1; nHost <= 2; nHost++)
    {
      if (nHost == 1)
      {
        addrConnect = CService("91.198.22.70",80);

        if (nLookup == 1)
        {
          CService addrIP("checkip.dyndns.org", 80, true);

          if (addrIP.IsValid())
          {
            addrConnect = addrIP;
          }
        }

        pszGet = "GET / HTTP/1.1\r\n"
                 "Host: checkip.dyndns.org\r\n"
                 "User-Agent: I/OCoin\r\n"
                 "Connection: close\r\n"
                 "\r\n";
        pszKeyword = "Address:";
      }
      else if (nHost == 2)
      {
        addrConnect = CService("74.208.43.192", 80);

        if (nLookup == 1)
        {
          CService addrIP("www.showmyip.com", 80, true);

          if (addrIP.IsValid())
          {
            addrConnect = addrIP;
          }
        }

        pszGet = "GET /simple/ HTTP/1.1\r\n"
                 "Host: www.showmyip.com\r\n"
                 "User-Agent: I/OCoin\r\n"
                 "Connection: close\r\n"
                 "\r\n";
        pszKeyword = NULL;
      }

      if (GetMyExternalIP2(addrConnect, pszGet, pszKeyword, ipRet))
      {
        return true;
      }
    }

  return false;
}
void ThreadGetMyExternalIP(void* parg)
{
  RenameThread("iocoin-ext-ip");
  CNetAddr addrLocalHost;

  if (GetMyExternalIP(addrLocalHost))
  {
    printf("GetMyExternalIP() returned %s\n", addrLocalHost.ToStringIP().c_str());
    AddLocal(addrLocalHost, LOCAL_HTTP);
  }
}
void AddressCurrentlyConnected(const CService& addr)
{
  addrman.Connected(addr);
}
CNodeRef FindNode(const CNetAddr& ip)
{
  {
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNodeRef pnode, vNodes)

    if ((CNetAddr)pnode->addr == ip)
    {
      return pnode;
    }
  }
  return NULL;
}
CNodeRef FindNode(std::string addrName)
{
  LOCK(cs_vNodes);
  BOOST_FOREACH(CNodeRef pnode, vNodes)

  if (pnode->addrName == addrName)
  {
    return pnode;
  }

  return NULL;
}
CNodeRef FindNode(const CService& addr)
{
  {
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNodeRef pnode, vNodes)

    if ((CService)pnode->addr == addr)
    {
      return pnode;
    }
  }
  return NULL;
}
CNodeRef ConnectNode(CAddress addrConnect, const char *pszDest)
{
  if (pszDest == NULL)
  {
    if (IsLocal(addrConnect))
    {
      return NULL;
    }

    CNodeRef pnode = FindNode((CService)addrConnect);

    if (pnode)
    {
      return pnode;
    }
  }

  printf("trying connection %s lastseen=%.1fhrs\n",
         pszDest ? pszDest : addrConnect.ToString().c_str(),
         pszDest ? 0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);
  SOCKET hSocket;

  if (pszDest ? ConnectSocketByName(addrConnect, hSocket, pszDest, GetDefaultPort()) : ConnectSocket(addrConnect, hSocket))
  {
    addrman.Attempt(addrConnect);
    printf("connected %s\n", pszDest ? pszDest : addrConnect.ToString().c_str());
#ifdef WIN32
    u_long nOne = 1;

    if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
    {
      printf("ConnectSocket() : ioctlsocket non-blocking setting failed, error %d\n", WSAGetLastError());
    }

#else

    if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
    {
      printf("ConnectSocket() : fcntl non-blocking setting failed, error %d\n", errno);
    }

#endif
    CNodeRef pnode = std::make_shared<CNode>(hSocket, addrConnect, pszDest ? pszDest : "", false);
    {
      LOCK(cs_vNodes);
      vNodes.push_back(pnode);
    }
    pnode->nTimeConnected = GetTime();
    return pnode;
  }
  else
  {
    return NULL;
  }
}
void CNode::CloseSocketDisconnect()
{
  fDisconnect = true;

  if (hSocket != INVALID_SOCKET)
  {
    printf("disconnecting node %s\n", addrName.c_str());
    closesocket(hSocket);
    hSocket = INVALID_SOCKET;
    TRY_LOCK(cs_vRecvMsg, lockRecv);

    if (lockRecv)
    {
      vRecvMsg.clear();
    }
  }
}
void CNode::PushVersion()
{
  int64_t nTime = (fInbound ? GetAdjustedTime() : GetTime());
  CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService("0.0.0.0",0)));
  CAddress addrMe = GetLocalAddress(&addr);
  RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
  printf("send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", PROTOCOL_VERSION, nBestHeight, addrMe.ToString().c_str(), addrYou.ToString().c_str(), addr.ToString().c_str());
  PushMessage("version", PROTOCOL_VERSION, nLocalServices, nTime, addrYou, addrMe,
              nLocalHostNonce, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<string>()), nBestHeight);
}
std::map<CNetAddr, int64_t> CNode::setBanned;
CCriticalSection CNode::cs_setBanned;
void CNode::ClearBanned()
{
  setBanned.clear();
}
bool CNode::IsBanned(CNetAddr ip)
{
  bool fResult = false;
  {
    LOCK(cs_setBanned);
    std::map<CNetAddr, int64_t>::iterator i = setBanned.find(ip);

    if (i != setBanned.end())
    {
      int64_t t = (*i).second;

      if (GetTime() < t)
      {
        fResult = true;
      }
    }
  }
  return fResult;
}
bool CNode::Misbehaving(int howmuch)
{
  if (addr.IsLocal())
  {
    printf("Warning: Local node %s misbehaving (delta: %d)!\n", addrName.c_str(), howmuch);
    return false;
  }

  nMisbehavior += howmuch;

  if (nMisbehavior >= GetArg("-banscore", 100))
  {
    int64_t banTime = GetTime()+GetArg("-bantime", 60*60*24);
    printf("Misbehaving: %s (%d -> %d) DISCONNECTING\n", addr.ToString().c_str(), nMisbehavior-howmuch, nMisbehavior);
    {
      LOCK(cs_setBanned);

      if (setBanned[addr] < banTime)
      {
        setBanned[addr] = banTime;
      }
    }
    CloseSocketDisconnect();
    return true;
  }
  else
  {
    printf("Misbehaving: %s (%d -> %d)\n", addr.ToString().c_str(), nMisbehavior-howmuch, nMisbehavior);
  }

  return false;
}
#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats)
{
  X(nServices);
  X(nLastSend);
  X(nLastRecv);
  X(nTimeConnected);
  X(addrName);
  X(nVersion);
  X(strSubVer);
  X(fInbound);
  X(nStartingHeight);
  X(nMisbehavior);
}
#undef X
bool CNode::ReceiveMsgBytes(const char *pch, unsigned int nBytes)
{
  while (nBytes > 0)
  {
    if (vRecvMsg.empty() ||
        vRecvMsg.back().complete())
    {
      vRecvMsg.push_back(CNetMessage(SER_NETWORK, nRecvVersion));
    }

    CNetMessage& msg = vRecvMsg.back();
    int handled;

    if (!msg.in_data)
    {
      handled = msg.readHeader(pch, nBytes);
    }
    else
    {
      handled = msg.readData(pch, nBytes);
    }

    if (handled < 0)
    {
      return false;
    }

    pch += handled;
    nBytes -= handled;
  }

  return true;
}
int CNetMessage::readHeader(const char *pch, unsigned int nBytes)
{
  unsigned int nRemaining = 24 - nHdrPos;
  unsigned int nCopy = std::min(nRemaining, nBytes);
  memcpy(&hdrbuf[nHdrPos], pch, nCopy);
  nHdrPos += nCopy;

  if (nHdrPos < 24)
  {
    return nCopy;
  }

  try
  {
    hdrbuf >> hdr;
  }
  catch (std::exception &e)
  {
    return -1;
  }

  if (hdr.nMessageSize > MAX_SIZE)
  {
    return -1;
  }

  in_data = true;
  return nCopy;
}
int CNetMessage::readData(const char *pch, unsigned int nBytes)
{
  unsigned int nRemaining = hdr.nMessageSize - nDataPos;
  unsigned int nCopy = std::min(nRemaining, nBytes);

  if (vRecv.size() < nDataPos + nCopy)
  {
    vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
  }

  memcpy(&vRecv[nDataPos], pch, nCopy);
  nDataPos += nCopy;
  return nCopy;
}
void SocketSendData(CNodeRef pnode)
{
  std::deque<CSerializeData>::iterator it = pnode->vSendMsg.begin();

  while (it != pnode->vSendMsg.end())
  {
    const CSerializeData &data = *it;
    assert(data.size() > pnode->nSendOffset);
    int nBytes = send(pnode->hSocket, &data[pnode->nSendOffset], data.size() - pnode->nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);

    if (nBytes > 0)
    {
      pnode->nLastSend = GetTime();
      pnode->nSendOffset += nBytes;

      if (pnode->nSendOffset == data.size())
      {
        pnode->nSendOffset = 0;
        pnode->nSendSize -= data.size();
        it++;
      }
      else
      {
        break;
      }
    }
    else
    {
      if (nBytes < 0)
      {
        int nErr = WSAGetLastError();

        if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
        {
          printf("socket send error %d\n", nErr);
          pnode->CloseSocketDisconnect();
        }
      }

      break;
    }
  }

  if (it == pnode->vSendMsg.end())
  {
    assert(pnode->nSendOffset == 0);
    assert(pnode->nSendSize == 0);
  }

  pnode->vSendMsg.erase(pnode->vSendMsg.begin(), it);
}
void ThreadSocketHandler(void* parg)
{
  RenameThread("iocoin-net");

  try
  {
    vnThreadsRunning[THREAD_SOCKETHANDLER]++;
    ThreadSocketHandler2(parg);
    vnThreadsRunning[THREAD_SOCKETHANDLER]--;
  }
  catch (std::exception& e)
  {
    vnThreadsRunning[THREAD_SOCKETHANDLER]--;
    PrintException(&e, "ThreadSocketHandler()");
  }
  catch (...)
  {
    vnThreadsRunning[THREAD_SOCKETHANDLER]--;
    throw;
  }

  printf("ThreadSocketHandler exited\n");
}
void ThreadSocketHandler2(void* parg)
{
  printf("ThreadSocketHandler started\n");
  list<CNodeRef> vNodesDisconnected;
  unsigned int nPrevNodeCount = 0;

  while (true)
  {
    {
      LOCK(cs_vNodes);
      vector<CNodeRef> vNodesCopy = vNodes;
      BOOST_FOREACH(CNodeRef pnode, vNodesCopy)
      {
        if (pnode->fDisconnect ||
            (pnode.use_count() == 1 && pnode->vRecvMsg.empty() && pnode->nSendSize == 0 && pnode->ssSend.empty()))
        {
          vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());
          pnode->grantOutbound.Release();
          pnode->CloseSocketDisconnect();

          vNodesDisconnected.push_back(pnode);
        }
      }
      list<CNodeRef> vNodesDisconnectedCopy = vNodesDisconnected;
      BOOST_FOREACH(CNodeRef pnode, vNodesDisconnectedCopy)
      {
        if (pnode.use_count() == 1)
        {
          bool fDelete = false;
          {
            TRY_LOCK(pnode->cs_vSend, lockSend);

            if (lockSend)
            {
              TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);

              if (lockRecv)
              {
                TRY_LOCK(pnode->cs_mapRequests, lockReq);

                if (lockReq)
                {
                  TRY_LOCK(pnode->cs_inventory, lockInv);

                  if (lockInv)
                  {
                    fDelete = true;
                  }
                }
              }
            }
          }

          if (fDelete)
          {
            vNodesDisconnected.remove(pnode);
            // pnode will be freed when no shared_ptr references remain
          }
        }
      }
    }

    if (vNodes.size() != nPrevNodeCount)
    {
      nPrevNodeCount = vNodes.size();
      uiInterface.NotifyNumConnectionsChanged(vNodes.size());
    }

    struct timeval timeout;

    timeout.tv_sec = 0;

    timeout.tv_usec = 50000;

    fd_set fdsetRecv;

    fd_set fdsetSend;

    fd_set fdsetError;

    FD_ZERO(&fdsetRecv);

    FD_ZERO(&fdsetSend);

    FD_ZERO(&fdsetError);

    SOCKET hSocketMax = 0;

    bool have_fds = false;

    BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)
    {
      FD_SET(hListenSocket, &fdsetRecv);
      hSocketMax = max(hSocketMax, hListenSocket);
      have_fds = true;
    }

    {
      LOCK(cs_vNodes);
      BOOST_FOREACH(CNodeRef pnode, vNodes)
      {
        if (pnode->hSocket == INVALID_SOCKET)
        {
          continue;
        }

        {
          TRY_LOCK(pnode->cs_vSend, lockSend);

          if (lockSend)
          {
            if (!pnode->vSendMsg.empty())
            {
              FD_SET(pnode->hSocket, &fdsetSend);
            }
            else
            {
              FD_SET(pnode->hSocket, &fdsetRecv);
            }

            FD_SET(pnode->hSocket, &fdsetError);
            hSocketMax = max(hSocketMax, pnode->hSocket);
            have_fds = true;
          }
        }
      }
    }

    vnThreadsRunning[THREAD_SOCKETHANDLER]--;
    int nSelect = select(have_fds ? hSocketMax + 1 : 0,
                         &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
    vnThreadsRunning[THREAD_SOCKETHANDLER]++;

    if (fShutdown)
    {
      return;
    }

    if (nSelect == SOCKET_ERROR)
    {
      if (have_fds)
      {
        int nErr = WSAGetLastError();
        printf("socket select error %d\n", nErr);

        for (unsigned int i = 0; i <= hSocketMax; i++)
        {
          FD_SET(i, &fdsetRecv);
        }
      }

      FD_ZERO(&fdsetSend);
      FD_ZERO(&fdsetError);
      MilliSleep(timeout.tv_usec/1000);
    }

    BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)

    if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
    {
      struct sockaddr_storage sockaddr;
      socklen_t len = sizeof(sockaddr);
      SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
      CAddress addr;
      int nInbound = 0;

      if (hSocket != INVALID_SOCKET)
        if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr))
        {
          printf("Warning: Unknown socket family\n");
        }

      {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNodeRef pnode, vNodes)

        if (pnode->fInbound)
        {
          nInbound++;
        }
      }

      if (hSocket == INVALID_SOCKET)
      {
        int nErr = WSAGetLastError();

        if (nErr != WSAEWOULDBLOCK)
        {
          printf("socket error accept failed: %d\n", nErr);
        }
      }
      else if (nInbound >= GetArg("-maxconnections", 125) - MAX_OUTBOUND_CONNECTIONS)
      {
        closesocket(hSocket);
      }
      else if (CNode::IsBanned(addr))
      {
        printf("connection from %s dropped (banned)\n", addr.ToString().c_str());
        closesocket(hSocket);
      }
      else
      {
        printf("accepted connection %s\n", addr.ToString().c_str());
        CNodeRef pnode = std::make_shared<CNode>(hSocket, addr, "", true);
        {
          LOCK(cs_vNodes);
          vNodes.push_back(pnode);
        }
      }
    }

    vector<CNodeRef> vNodesCopy;
    {
      LOCK(cs_vNodes);
      vNodesCopy = vNodes;
    }
    BOOST_FOREACH(CNodeRef pnode, vNodesCopy)
    {
      if (fShutdown)
      {
        return;
      }

      if (pnode->hSocket == INVALID_SOCKET)
      {
        continue;
      }

      if (FD_ISSET(pnode->hSocket, &fdsetRecv) || FD_ISSET(pnode->hSocket, &fdsetError))
      {
        TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);

        if (lockRecv)
        {
          if (pnode->GetTotalRecvSize() > ReceiveFloodSize())
          {
            if (!pnode->fDisconnect)
            {
              printf("socket recv flood control disconnect (%u bytes)\n", pnode->GetTotalRecvSize());
            }

            pnode->CloseSocketDisconnect();
          }
          else
          {
            char pchBuf[0x10000];
            int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);

            if (nBytes > 0)
            {
              if (!pnode->ReceiveMsgBytes(pchBuf, nBytes))
              {
                pnode->CloseSocketDisconnect();
              }

              pnode->nLastRecv = GetTime();
            }
            else if (nBytes == 0)
            {
              if (!pnode->fDisconnect)
              {
                printf("socket closed\n");
              }

              pnode->CloseSocketDisconnect();
            }
            else if (nBytes < 0)
            {
              int nErr = WSAGetLastError();

              if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
              {
                if (!pnode->fDisconnect)
                {
                  printf("socket recv error %d\n", nErr);
                }

                pnode->CloseSocketDisconnect();
              }
            }
          }
        }
      }

      if (pnode->hSocket == INVALID_SOCKET)
      {
        continue;
      }

      if (FD_ISSET(pnode->hSocket, &fdsetSend))
      {
        TRY_LOCK(pnode->cs_vSend, lockSend);

        if (lockSend)
        {
          SocketSendData(pnode);
        }
      }

      if (pnode->vSendMsg.empty())
      {
        pnode->nLastSendEmpty = GetTime();
      }

      if (false && GetTime() - pnode->nTimeConnected > 60)
      {
        if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
        {
          printf("socket no message in first 60 seconds, %d %d\n", pnode->nLastRecv != 0, pnode->nLastSend != 0);
          pnode->fDisconnect = true;
        }
        else if (GetTime() - pnode->nLastSend > 90*60 && GetTime() - pnode->nLastSendEmpty > 90*60)
        {
          printf("socket not sending\n");
          pnode->fDisconnect = true;
        }
        else if (GetTime() - pnode->nLastRecv > 90*60)
        {
          printf("socket inactivity timeout\n");
          pnode->fDisconnect = true;
        }
      }
    }
    {
      LOCK(cs_vNodes);
      // shared_ptr handles reference counting
    }
    MilliSleep(10);
  }
}
void MapPort()
{
}
void DumpAddresses()
{
  int64_t nStart = GetTimeMillis();
  CAddrDB adb;
  adb.Write(addrman);
  printf("Flushed %d addresses to peers.dat  %" PRId64 "ms\n",
         addrman.size(), GetTimeMillis() - nStart);
}
void ThreadDumpAddress2(void* parg)
{
  vnThreadsRunning[THREAD_DUMPADDRESS]++;

  while (!fShutdown)
  {
    DumpAddresses();
    vnThreadsRunning[THREAD_DUMPADDRESS]--;
    MilliSleep(600000);
    vnThreadsRunning[THREAD_DUMPADDRESS]++;
  }

  vnThreadsRunning[THREAD_DUMPADDRESS]--;
}
void ThreadDumpAddress(void* parg)
{
  RenameThread("iocoin-adrdump");

  try
  {
    ThreadDumpAddress2(parg);
  }
  catch (std::exception& e)
  {
    PrintException(&e, "ThreadDumpAddress()");
  }

  printf("ThreadDumpAddress exited\n");
}
void ThreadOpenConnections(void* parg)
{
  RenameThread("iocoin-opencon");

  try
  {
    vnThreadsRunning[THREAD_OPENCONNECTIONS]++;
    ThreadOpenConnections2(parg);
    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
  }
  catch (std::exception& e)
  {
    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    PrintException(&e, "ThreadOpenConnections()");
  }
  catch (...)
  {
    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    PrintException(NULL, "ThreadOpenConnections()");
  }

  printf("ThreadOpenConnections exited\n");
}
void static ProcessOneShot()
{
  string strDest;
  {
    LOCK(cs_vOneShots);

    if (vOneShots.empty())
    {
      return;
    }

    strDest = vOneShots.front();
    vOneShots.pop_front();
  }
  CAddress addr;
  CSemaphoreGrant grant(*semOutbound, true);

  if (grant)
  {
    if (!OpenNetworkConnection(addr, &grant, strDest.c_str(), true))
    {
      AddOneShot(strDest);
    }
  }
}
void Miner::ThreadStakeMiner()
{
  printf("MINER::ThreadStakeMiner started\n");
  try
  {
    vnThreadsRunning[THREAD_STAKE_MINER]++;
    this->StakeMiner();
    vnThreadsRunning[THREAD_STAKE_MINER]--;
  }
  catch (std::exception& e)
  {
    vnThreadsRunning[THREAD_STAKE_MINER]--;
    PrintException(&e, "ThreadStakeMiner()");
  }
  catch (...)
  {
    vnThreadsRunning[THREAD_STAKE_MINER]--;
    PrintException(NULL, "ThreadStakeMiner()");
  }

  printf("ThreadStakeMiner exiting, %d threads remaining\n", vnThreadsRunning[THREAD_STAKE_MINER]);
}
void ThreadOpenConnections2(void* parg)
{
  printf("ThreadOpenConnections started\n");

  if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0)
  {
    for (int64_t nLoop = 0;; nLoop++)
    {
      ProcessOneShot();
      BOOST_FOREACH(string strAddr, mapMultiArgs["-connect"])
      {
        CAddress addr;
        OpenNetworkConnection(addr, NULL, strAddr.c_str());

        for (int i = 0; i < 10 && i < nLoop; i++)
        {
          MilliSleep(500);

          if (fShutdown)
          {
            return;
          }
        }
      }
      MilliSleep(500);
    }
  }

  while (true)
  {
    ProcessOneShot();
    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    MilliSleep(500);
    vnThreadsRunning[THREAD_OPENCONNECTIONS]++;

    if (fShutdown)
    {
      return;
    }

    vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
    CSemaphoreGrant grant(*semOutbound);
    vnThreadsRunning[THREAD_OPENCONNECTIONS]++;

    if (fShutdown)
    {
      return;
    }

    CAddress addrConnect;
    int nOutbound = 0;
    set<vector<unsigned char> > setConnected;
    {
      LOCK(cs_vNodes);
      BOOST_FOREACH(CNodeRef pnode, vNodes)
      {
        if (!pnode->fInbound)
        {
          setConnected.insert(pnode->addr.GetGroup());
          nOutbound++;
        }
      }
    }
    int64_t nANow = GetAdjustedTime();
    int nTries = 0;

    while (true)
    {
      CAddress addr = addrman.Select(10 + min(nOutbound,8)*10);

      if (!addr.IsValid() || setConnected.count(addr.GetGroup()) || IsLocal(addr))
      {
        break;
      }

      nTries++;

      if (nTries > 100)
      {
        break;
      }

      if (IsLimited(addr))
      {
        continue;
      }

      if (nANow - addr.nLastTry < 600 && nTries < 30)
      {
        continue;
      }

      if (addr.GetPort() != GetDefaultPort() && nTries < 50)
      {
        continue;
      }

      addrConnect = addr;
      break;
    }

    if (addrConnect.IsValid())
    {
      OpenNetworkConnection(addrConnect, &grant);
    }
  }
}
void ThreadOpenAddedConnections(void* parg)
{
  RenameThread("iocoin-opencon");

  try
  {
    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]++;
    ThreadOpenAddedConnections2(parg);
    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
  }
  catch (std::exception& e)
  {
    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
    PrintException(&e, "ThreadOpenAddedConnections()");
  }
  catch (...)
  {
    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
    PrintException(NULL, "ThreadOpenAddedConnections()");
  }

  printf("ThreadOpenAddedConnections exited\n");
}
void ThreadOpenAddedConnections2(void* parg)
{
  printf("ThreadOpenAddedConnections started\n");

  if (mapArgs.count("-addnode") == 0)
  {
    return;
  }

  if (HaveNameProxy())
  {
    while(!fShutdown)
    {
      BOOST_FOREACH(string& strAddNode, mapMultiArgs["-addnode"])
      {
        CAddress addr;
        CSemaphoreGrant grant(*semOutbound);
        OpenNetworkConnection(addr, &grant, strAddNode.c_str());
        MilliSleep(500);
      }
      vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
      MilliSleep(120000);
      vnThreadsRunning[THREAD_ADDEDCONNECTIONS]++;
    }

    return;
  }

  vector<vector<CService> > vservAddressesToAdd(0);
  BOOST_FOREACH(string& strAddNode, mapMultiArgs["-addnode"])
  {
    vector<CService> vservNode(0);

    if(Lookup(strAddNode.c_str(), vservNode, GetDefaultPort(), fNameLookup, 0))
    {
      vservAddressesToAdd.push_back(vservNode);
      {
        LOCK(cs_setservAddNodeAddresses);
        BOOST_FOREACH(CService& serv, vservNode)
        setservAddNodeAddresses.insert(serv);
      }
    }
  }

  while (true)
  {
    vector<vector<CService> > vservConnectAddresses = vservAddressesToAdd;
    {
      LOCK(cs_vNodes);
      BOOST_FOREACH(CNodeRef pnode, vNodes)

      for (vector<vector<CService> >::iterator it = vservConnectAddresses.begin(); it != vservConnectAddresses.end(); it++)
        BOOST_FOREACH(CService& addrNode, *(it))
        if (pnode->addr == addrNode)
        {
          it = vservConnectAddresses.erase(it);
          it--;
          break;
        }
    }
    BOOST_FOREACH(vector<CService>& vserv, vservConnectAddresses)
    {
      CSemaphoreGrant grant(*semOutbound);
      OpenNetworkConnection(CAddress(*(vserv.begin())), &grant);
      MilliSleep(500);

      if (fShutdown)
      {
        return;
      }
    }

    if (fShutdown)
    {
      return;
    }

    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]--;
    MilliSleep(120000);
    vnThreadsRunning[THREAD_ADDEDCONNECTIONS]++;

    if (fShutdown)
    {
      return;
    }
  }
}
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound, const char *strDest, bool fOneShot)
{
  if (fShutdown)
  {
    return false;
  }

  if (!strDest)
    if (IsLocal(addrConnect) ||
        FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect) ||
        FindNode(addrConnect.ToStringIPPort().c_str()))
    {
      return false;
    }

  if (strDest && FindNode(strDest))
  {
    return false;
  }

  vnThreadsRunning[THREAD_OPENCONNECTIONS]--;
  CNodeRef pnode = ConnectNode(addrConnect, strDest);
  vnThreadsRunning[THREAD_OPENCONNECTIONS]++;

  if (fShutdown)
  {
    return false;
  }

  if (!pnode)
  {
    return false;
  }

  if (grantOutbound)
  {
    grantOutbound->MoveTo(pnode->grantOutbound);
  }

  pnode->fNetworkNode = true;

  if (fOneShot)
  {
    pnode->fOneShot = true;
  }

  return true;
}
void ThreadMessageHandler(void* parg)
{
  RenameThread("iocoin-msghand");

  try
  {
    vnThreadsRunning[THREAD_MESSAGEHANDLER]++;
    ThreadMessageHandler2(parg);
    vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
  }
  catch (std::exception& e)
  {
    vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
    PrintException(&e, "ThreadMessageHandler()");
  }
  catch (...)
  {
    vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
    PrintException(NULL, "ThreadMessageHandler()");
  }

  printf("ThreadMessageHandler exited\n");
}
void ThreadMessageHandler2(void* parg)
{
  printf("ThreadMessageHandler started\n");
  SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);

  while (!fShutdown)
  {
    vector<CNodeRef> vNodesCopy;
    {
      LOCK(cs_vNodes);
      vNodesCopy = vNodes;
    }
    CNodeRef pnodeTrickle;

    if (!vNodesCopy.empty())
    {
      pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];
    }

    BOOST_FOREACH(CNodeRef pnode, vNodesCopy)
    {
      if (pnode->fDisconnect)
      {
        continue;
      }

      {
        TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);

        if (lockRecv)
          if (!ProcessMessages(pnode.get()))
          {
            pnode->CloseSocketDisconnect();
          }
      }

      if (fShutdown)
      {
        return;
      }

      {
        TRY_LOCK(pnode->cs_vSend, lockSend);

        if (lockSend)
        {
          SendMessages(pnode.get(), pnode == pnodeTrickle);
        }
      }

      if (fShutdown)
      {
        return;
      }
    }
    {
      LOCK(cs_vNodes);
      // shared_ptr handles reference counting
    }
    vnThreadsRunning[THREAD_MESSAGEHANDLER]--;
    MilliSleep(100);

    if (fRequestShutdown)
    {
      StartShutdown();
    }

    vnThreadsRunning[THREAD_MESSAGEHANDLER]++;

    if (fShutdown)
    {
      return;
    }
  }
}
bool BindListenPort(const CService &addrBind, string& strError)
{
  strError = "";
  int nOne = 1;
#ifdef WIN32
  WSADATA wsadata;
  int ret = WSAStartup(MAKEWORD(2,2), &wsadata);

  if (ret != NO_ERROR)
  {
    strError = strprintf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);
    printf("%s\n", strError.c_str());
    return false;
  }

#endif
  struct sockaddr_storage sockaddr;
  socklen_t len = sizeof(sockaddr);

  if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
  {
    strError = strprintf("Error: bind address family for %s not supported", addrBind.ToString().c_str());
    printf("%s\n", strError.c_str());
    return false;
  }

  SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);

  if (hListenSocket == INVALID_SOCKET)
  {
    strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", WSAGetLastError());
    printf("%s\n", strError.c_str());
    return false;
  }

#ifdef SO_NOSIGPIPE
  setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif
#ifndef WIN32
  setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
#endif
#ifdef WIN32

  if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
  if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
  {
    strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %d)", WSAGetLastError());
    printf("%s\n", strError.c_str());
    return false;
  }

  if (addrBind.IsIPv6())
  {
#ifdef IPV6_V6ONLY
#ifdef WIN32
    setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
    setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef WIN32
    int nProtLevel = 10 ;
    int nParameterId = 23 ;
    setsockopt(hListenSocket, IPPROTO_IPV6, nParameterId, (const char*)&nProtLevel, sizeof(int));
#endif
  }

  if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
  {
    int nErr = WSAGetLastError();

    if (nErr == WSAEADDRINUSE)
    {
      strError = strprintf(_("Unable to bind to %s on this computer. I/OCoin is probably already running."), addrBind.ToString().c_str());
    }
    else
    {
      strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %d, %s)"), addrBind.ToString().c_str(), nErr, strerror(nErr));
    }

    printf("%s\n", strError.c_str());
    return false;
  }

  printf("Bound to %s\n", addrBind.ToString().c_str());

  if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
  {
    strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
    printf("%s\n", strError.c_str());
    return false;
  }

  vhListenSocket.push_back(hListenSocket);

  if (addrBind.IsRoutable() && fDiscover)
  {
    AddLocal(addrBind, LOCAL_BIND);
  }

  return true;
}
void static Discover()
{
  if (!fDiscover)
  {
    return;
  }

#ifdef WIN32
  char pszHostName[1000] = "";

  if (gethostname(pszHostName, sizeof(pszHostName)) != SOCKET_ERROR)
  {
    vector<CNetAddr> vaddr;

    if (LookupHost(pszHostName, vaddr))
    {
      BOOST_FOREACH (const CNetAddr &addr, vaddr)
      {
        AddLocal(addr, LOCAL_IF);
      }
    }
  }

#else
  struct ifaddrs* myaddrs;

  if (getifaddrs(&myaddrs) == 0)
  {
    for (struct ifaddrs* ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL)
      {
        continue;
      }

      if ((ifa->ifa_flags & IFF_UP) == 0)
      {
        continue;
      }

      if (strcmp(ifa->ifa_name, "lo") == 0)
      {
        continue;
      }

      if (strcmp(ifa->ifa_name, "lo0") == 0)
      {
        continue;
      }

      if (ifa->ifa_addr->sa_family == AF_INET)
      {
        struct sockaddr_in* s4 = (struct sockaddr_in*)(ifa->ifa_addr);
        CNetAddr addr(s4->sin_addr);

        if (AddLocal(addr, LOCAL_IF))
        {
          printf("IPv4 %s: %s\n", ifa->ifa_name, addr.ToString().c_str());
        }
      }
      else if (ifa->ifa_addr->sa_family == AF_INET6)
      {
        struct sockaddr_in6* s6 = (struct sockaddr_in6*)(ifa->ifa_addr);
        CNetAddr addr(s6->sin6_addr);

        if (AddLocal(addr, LOCAL_IF))
        {
          printf("IPv6 %s: %s\n", ifa->ifa_name, addr.ToString().c_str());
        }
      }
    }

    freeifaddrs(myaddrs);
  }

#endif

  if (!IsLimited(NET_IPV4))
  {
    NewThread(ThreadGetMyExternalIP, NULL);
  }
}
void StartNode(void* parg)
{
  RenameThread("iocoin-start");

  if (semOutbound == NULL)
  {
    int nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, (int)GetArg("-maxconnections", 125));
    semOutbound = new CSemaphore(nMaxOutbound);
  }

  if (!pnodeLocalHost)
  {
    pnodeLocalHost = std::make_shared<CNode>(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));
  }

  Discover();

  if (fUseUPnP)
  {
    MapPort();
  }

  if (!NewThread(ThreadSocketHandler, NULL))
  {
    printf("Error: NewThread(ThreadSocketHandler) failed\n");
  }

  if (!NewThread(ThreadOpenAddedConnections, NULL))
  {
    printf("Error: NewThread(ThreadOpenAddedConnections) failed\n");
  }

  if (!NewThread(ThreadOpenConnections, NULL))
  {
    printf("Error: NewThread(ThreadOpenConnections) failed\n");
  }

  if (!NewThread(ThreadMessageHandler, NULL))
  {
    printf("Error: NewThread(ThreadMessageHandler) failed\n");
  }

  if (!NewThread(ThreadDumpAddress, NULL))
  {
    printf("Error; NewThread(ThreadDumpAddress) failed\n");
  }
}
bool StopNode()
{
  printf("StopNode()\n");
  fShutdown = true;
  CTxMemPool::nTransactionsUpdated++;
  int64_t nStart = GetTime();

  if (semOutbound)
    for (int i=0; i<MAX_OUTBOUND_CONNECTIONS; i++)
    {
      semOutbound->post();
    }

  do
  {
    int nThreadsRunning = 0;

    for (int n = 0; n < THREAD_MAX; n++)
    {
      nThreadsRunning += vnThreadsRunning[n];
    }

    if (nThreadsRunning == 0)
    {
      break;
    }

    if (GetTime() - nStart > 20)
    {
      break;
    }

    MilliSleep(20);
  }
  while(true);

  if (vnThreadsRunning[THREAD_SOCKETHANDLER] > 0)
  {
    printf("ThreadSocketHandler still running\n");
  }

  if (vnThreadsRunning[THREAD_OPENCONNECTIONS] > 0)
  {
    printf("ThreadOpenConnections still running\n");
  }

  if (vnThreadsRunning[THREAD_MESSAGEHANDLER] > 0)
  {
    printf("ThreadMessageHandler still running\n");
  }

  if (vnThreadsRunning[THREAD_RPCLISTENER] > 0)
  {
    printf("ThreadRPCListener still running\n");
  }

  if (vnThreadsRunning[THREAD_RPCHANDLER] > 0)
  {
    printf("ThreadsRPCServer still running\n");
  }

  if (vnThreadsRunning[THREAD_DNSSEED] > 0)
  {
    printf("ThreadDNSAddressSeed still running\n");
  }

  if (vnThreadsRunning[THREAD_ADDEDCONNECTIONS] > 0)
  {
    printf("ThreadOpenAddedConnections still running\n");
  }

  if (vnThreadsRunning[THREAD_DUMPADDRESS] > 0)
  {
    printf("ThreadDumpAddresses still running\n");
  }

  if (vnThreadsRunning[THREAD_STAKE_MINER] > 0)
  {
    printf("ThreadStakeMiner still running\n");
  }

  while (vnThreadsRunning[THREAD_MESSAGEHANDLER] > 0 || vnThreadsRunning[THREAD_RPCHANDLER] > 0)
  {
    MilliSleep(20);
  }

  MilliSleep(50);
  DumpAddresses();
  return true;
}
class CNetCleanup
{
public:
  CNetCleanup()
  {
  }
  ~CNetCleanup()
  {
    BOOST_FOREACH(CNodeRef pnode, vNodes)

    if (pnode->hSocket != INVALID_SOCKET)
    {
      closesocket(pnode->hSocket);
    }

    BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)

    if (hListenSocket != INVALID_SOCKET)
      if (closesocket(hListenSocket) == SOCKET_ERROR)
      {
        printf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());
      }

#ifdef WIN32
    WSACleanup();
#endif
  }
}
instance_of_cnetcleanup;
void RelayTransaction(const CTransaction& tx, const uint256& hash)
{
  CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
  ss.reserve(10000);
  ss << tx;
  RelayTransaction(tx, hash, ss);
}
void RelayTransaction(const CTransaction& tx, const uint256& hash, const CDataStream& ss)
{
  CInv inv(MSG_TX, hash);
  {
    LOCK(cs_mapRelay);

    while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime())
    {
      mapRelay.erase(vRelayExpiration.front().second);
      vRelayExpiration.pop_front();
    }

    mapRelay.insert(std::make_pair(inv, ss));
    vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
  }
  RelayInventory(inv);
}
