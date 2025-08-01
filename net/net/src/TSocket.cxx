// @(#)root/net:$Id$
// Author: Fons Rademakers   18/12/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TSocket                                                              //
//                                                                      //
// This class implements client sockets. A socket is an endpoint for    //
// communication between two machines.                                  //
// The actual work is done via the TSystem class (either TUnixSystem    //
// or TWinNTSystem).                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "Bytes.h"
#include "Compression.h"
#include "NetErrors.h"
#include "TError.h"
#include "TMessage.h"
#include "TObjString.h"
#include "TPSocket.h"
#include "TPluginManager.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"
#include "TUrl.h"
#include "TVirtualAuth.h"
#include "TStreamerInfo.h"
#include "TProcessID.h"


ULong64_t TSocket::fgBytesSent = 0;
ULong64_t TSocket::fgBytesRecv = 0;

//
// Client "protocol changes"
//
// This was in TNetFile and TAuthenticate before, but after the introduction
// of TSocket::CreateAuthSocket the common place for all the clients is TSocket,
// so this seems to be the right place for a version number
//
// 7: added support for ReOpen(), kROOTD_BYE and kROOTD_PROTOCOL2
// 8: added support for update being a create (open stat = 2 and not 1)
// 9: added new authentication features (see README.AUTH)
// 10: added support for authenticated socket via TSocket::CreateAuthSocket(...)
// 11: modified SSH protocol + support for server 'no authentication' mode
// 12: add random tags to avoid reply attacks (password+token)
// 13: LEGACY: authentication re-organization; cleanup in PROOF
// 14: support for SSH authentication via SSH tunnel
// 15: cope with fixes in TUrl::GetFile
// 16: add env setup message exchange
//
Int_t TSocket::fgClientProtocol = 17;  // increase when client protocol changes

TVirtualMutex *gSocketAuthMutex = 0;

ClassImp(TSocket);

////////////////////////////////////////////////////////////////////////////////
/// Create a socket. Connect to the named service at address addr.
/// Use tcpwindowsize to specify the size of the receive buffer, it has
/// to be specified here to make sure the window scale option is set (for
/// tcpwindowsize > 65KB and for platforms supporting window scaling).
/// Returns when connection has been accepted by remote side. Use IsValid()
/// to check the validity of the socket. Every socket is added to the TROOT
/// sockets list which will make sure that any open sockets are properly
/// closed on program termination.

TSocket::TSocket(TInetAddress addr, const char *service, Int_t tcpwindowsize)
         : TNamed(addr.GetHostName(), service), fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fService = service;
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   if (fService.Contains("root"))
      fServType = kROOTD;
   fAddress = addr;
   fAddress.fPort = gSystem->GetServiceByName(service);
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = tcpwindowsize;
   fUUIDs = 0;
   fLastUsageMtx = 0;
   ResetBit(TSocket::kBrokenConn);

   if (fAddress.GetPort() != -1) {
      fSocket = gSystem->OpenConnection(addr.GetHostName(), fAddress.GetPort(),
                                        tcpwindowsize);

      if (fSocket != kInvalid) {
         gROOT->GetListOfSockets()->Add(this);
      }
   } else
      fSocket = kInvalid;

}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket. Connect to the specified port # at address addr.
/// Use tcpwindowsize to specify the size of the receive buffer, it has
/// to be specified here to make sure the window scale option is set (for
/// tcpwindowsize > 65KB and for platforms supporting window scaling).
/// Returns when connection has been accepted by remote side. Use IsValid()
/// to check the validity of the socket. Every socket is added to the TROOT
/// sockets list which will make sure that any open sockets are properly
/// closed on program termination.

TSocket::TSocket(TInetAddress addr, Int_t port, Int_t tcpwindowsize)
         : TNamed(addr.GetHostName(), ""), fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fService = gSystem->GetServiceByPort(port);
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   if (fService.Contains("root"))
      fServType = kROOTD;
   fAddress = addr;
   fAddress.fPort = port;
   SetTitle(fService);
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = tcpwindowsize;
   fUUIDs = 0;
   fLastUsageMtx = 0;
   ResetBit(TSocket::kBrokenConn);

   fSocket = gSystem->OpenConnection(addr.GetHostName(), fAddress.GetPort(),
                                     tcpwindowsize);
   if (fSocket == kInvalid)
      fAddress.fPort = -1;
   else {
      gROOT->GetListOfSockets()->Add(this);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket. Connect to named service on the remote host.
/// Use tcpwindowsize to specify the size of the receive buffer, it has
/// to be specified here to make sure the window scale option is set (for
/// tcpwindowsize > 65KB and for platforms supporting window scaling).
/// Returns when connection has been accepted by remote side. Use IsValid()
/// to check the validity of the socket. Every socket is added to the TROOT
/// sockets list which will make sure that any open sockets are properly
/// closed on program termination.

TSocket::TSocket(const char *host, const char *service, Int_t tcpwindowsize)
         : TNamed(host, service), fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fService = service;
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   if (fService.Contains("root"))
      fServType = kROOTD;
   fAddress = gSystem->GetHostByName(host);
   fAddress.fPort = gSystem->GetServiceByName(service);
   SetName(fAddress.GetHostName());
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = tcpwindowsize;
   fUUIDs = 0;
   fLastUsageMtx = 0;
   ResetBit(TSocket::kBrokenConn);

   if (fAddress.GetPort() != -1) {
      fSocket = gSystem->OpenConnection(host, fAddress.GetPort(), tcpwindowsize);
      if (fSocket != kInvalid) {
         gROOT->GetListOfSockets()->Add(this);
      }
   } else
      fSocket = kInvalid;
}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket; see CreateAuthSocket for the form of url.
/// Connect to the specified port # on the remote host.
/// If user is specified in url, try authentication as user.
/// Use tcpwindowsize to specify the size of the receive buffer, it has
/// to be specified here to make sure the window scale option is set (for
/// tcpwindowsize > 65KB and for platforms supporting window scaling).
/// Returns when connection has been accepted by remote side. Use IsValid()
/// to check the validity of the socket. Every socket is added to the TROOT
/// sockets list which will make sure that any open sockets are properly
/// closed on program termination.

TSocket::TSocket(const char *url, Int_t port, Int_t tcpwindowsize)
         : TNamed(TUrl(url).GetHost(), ""), fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fUrl = TString(url);
   TString host(TUrl(fUrl).GetHost());

   fService = gSystem->GetServiceByPort(port);
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   if (fUrl.Contains("root"))
      fServType = kROOTD;
   fAddress = gSystem->GetHostByName(host);
   fAddress.fPort = port;
   SetName(fAddress.GetHostName());
   SetTitle(fService);
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = tcpwindowsize;
   fUUIDs = 0;
   fLastUsageMtx = 0;
   ResetBit(TSocket::kBrokenConn);

   fSocket = gSystem->OpenConnection(host, fAddress.GetPort(), tcpwindowsize);
   if (fSocket == kInvalid) {
      fAddress.fPort = kInvalid;
   } else {
      gROOT->GetListOfSockets()->Add(this);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket in the Unix domain on 'sockpath'.
/// Returns when connection has been accepted by the server. Use IsValid()
/// to check the validity of the socket. Every socket is added to the TROOT
/// sockets list which will make sure that any open sockets are properly
/// closed on program termination.

TSocket::TSocket(const char *sockpath) : TNamed(sockpath, ""),
                                         fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fUrl = sockpath;

   fService = "unix";
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   fAddress.fPort = -1;
   fName.Form("unix:%s", sockpath);
   SetTitle(fService);
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = -1;
   fUUIDs = 0;
   fLastUsageMtx  = 0;
   ResetBit(TSocket::kBrokenConn);

   fSocket = gSystem->OpenConnection(sockpath, -1, -1);
   if (fSocket > 0) {
      gROOT->GetListOfSockets()->Add(this);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket. The socket will adopt previously opened TCP socket with
/// descriptor desc.

TSocket::TSocket(Int_t desc) : TNamed("", ""), fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fSecContext     = 0;
   fRemoteProtocol = 0;
   fService        = (char *)kSOCKD;
   fServType       = kSOCKD;
   fBytesSent      = 0;
   fBytesRecv      = 0;
   fTcpWindowSize = -1;
   fUUIDs          = 0;
   fLastUsageMtx   = 0;
   ResetBit(TSocket::kBrokenConn);

   if (desc >= 0) {
      fSocket  = desc;
      fAddress = gSystem->GetPeerName(fSocket);
      gROOT->GetListOfSockets()->Add(this);
   } else
      fSocket = kInvalid;
}

////////////////////////////////////////////////////////////////////////////////
/// Create a socket. The socket will adopt previously opened Unix socket with
/// descriptor desc. The sockpath arg is for info purposes only. Use
/// this method to adopt e.g. a socket created via socketpair().

TSocket::TSocket(Int_t desc, const char *sockpath) : TNamed(sockpath, ""),
                                                     fCompress(ROOT::RCompressionSetting::EAlgorithm::kUseGlobal)
{
   R__ASSERT(gROOT);
   R__ASSERT(gSystem);

   fUrl = sockpath;

   fService = "unix";
   fSecContext = 0;
   fRemoteProtocol= -1;
   fServType = kSOCKD;
   fAddress.fPort = -1;
   fName.Form("unix:%s", sockpath);
   SetTitle(fService);
   fBytesSent = 0;
   fBytesRecv = 0;
   fTcpWindowSize = -1;
   fUUIDs = 0;
   fLastUsageMtx  = 0;
   ResetBit(TSocket::kBrokenConn);

   if (desc >= 0) {
      fSocket  = desc;
      gROOT->GetListOfSockets()->Add(this);
   } else
      fSocket = kInvalid;
}


////////////////////////////////////////////////////////////////////////////////
/// TSocket copy ctor.

TSocket::TSocket(const TSocket &s) : TNamed(s)
{
   fSocket         = s.fSocket;
   fService        = s.fService;
   fAddress        = s.fAddress;
   fLocalAddress   = s.fLocalAddress;
   fBytesSent      = s.fBytesSent;
   fBytesRecv      = s.fBytesRecv;
   fCompress       = s.fCompress;
   fSecContext     = s.fSecContext;
   fRemoteProtocol = s.fRemoteProtocol;
   fServType       = s.fServType;
   fTcpWindowSize  = s.fTcpWindowSize;
   fUUIDs          = 0;
   fLastUsageMtx   = 0;
   ResetBit(TSocket::kBrokenConn);

   if (fSocket != kInvalid) {
      gROOT->GetListOfSockets()->Add(this);
   }
}
////////////////////////////////////////////////////////////////////////////////
/// Close the socket and mark as due to a broken connection.

void TSocket::MarkBrokenConnection()
{
   SetBit(TSocket::kBrokenConn);
   if (IsValid()) {
      gSystem->CloseConnection(fSocket, kFALSE);
      fSocket = kInvalidStillInList;
   }

   SafeDelete(fUUIDs);
   SafeDelete(fLastUsageMtx);
}

////////////////////////////////////////////////////////////////////////////////
/// Close the socket. If option is "force", calls shutdown(id,2) to
/// shut down the connection. This will close the connection also
/// for the parent of this process. Also called via the dtor (without
/// option "force", call explicitly Close("force") if this is desired).

void TSocket::Close(Option_t *option)
{
   Bool_t force = option ? (!strcmp(option, "force") ? kTRUE : kFALSE) : kFALSE;

   if (fSocket != kInvalid) {
      if (IsValid()) { // Filter out kInvalidStillInList case (disconnected but not removed from list)
         gSystem->CloseConnection(fSocket, force);
      }
      gROOT->GetListOfSockets()->Remove(this);
   }
   fSocket = kInvalid;

   SafeDelete(fUUIDs);
   SafeDelete(fLastUsageMtx);
}

////////////////////////////////////////////////////////////////////////////////
/// Return internet address of local host to which the socket is bound.
/// In case of error TInetAddress::IsValid() returns kFALSE.

TInetAddress TSocket::GetLocalInetAddress()
{
   if (IsValid()) {
      if (fLocalAddress.GetPort() == -1)
         fLocalAddress = gSystem->GetSockName(fSocket);
      return fLocalAddress;
   }
   return TInetAddress();
}

////////////////////////////////////////////////////////////////////////////////
/// Return the local port # to which the socket is bound.
/// In case of error return -1.

Int_t TSocket::GetLocalPort()
{
   if (IsValid()) {
      if (fLocalAddress.GetPort() == -1)
         GetLocalInetAddress();
      return fLocalAddress.GetPort();
   }
   return -1;
}

////////////////////////////////////////////////////////////////////////////////
/// Waits for this socket to change status. If interest=kRead,
/// the socket will be watched to see if characters become available for
/// reading; if interest=kWrite the socket will be watched to
/// see if a write will not block.
/// The argument 'timeout' specifies a maximum time to wait in millisec.
/// Default no timeout.
/// Returns 1 if a change of status of interest has been detected within
/// timeout; 0 in case of timeout; < 0 if an error occured.

Int_t TSocket::Select(Int_t interest, Long_t timeout)
{
   Int_t rc = 1;

   // Associate a TFileHandler to this socket
   TFileHandler fh(fSocket, interest);

   // Wait for an event now
   rc = gSystem->Select(&fh, timeout);

   return rc;
}

////////////////////////////////////////////////////////////////////////////////
/// Send a single message opcode. Use kind (opcode) to set the
/// TMessage "what" field. Returns the number of bytes that were sent
/// (always sizeof(Int_t)) and -1 in case of error. In case the kind has
/// been or'ed with kMESS_ACK, the call will only return after having
/// received an acknowledgement, making the sending process synchronous.

Int_t TSocket::Send(Int_t kind)
{
   TMessage mess(kind);

   Int_t nsent;
   if ((nsent = Send(mess)) < 0)
      return -1;

   return nsent;
}

////////////////////////////////////////////////////////////////////////////////
/// Send a status and a single message opcode. Use kind (opcode) to set the
/// TMessage "what" field. Returns the number of bytes that were sent
/// (always 2*sizeof(Int_t)) and -1 in case of error. In case the kind has
/// been or'ed with kMESS_ACK, the call will only return after having
/// received an acknowledgement, making the sending process synchronous.

Int_t TSocket::Send(Int_t status, Int_t kind)
{
   TMessage mess(kind);
   mess << status;

   Int_t nsent;
   if ((nsent = Send(mess)) < 0)
      return -1;

   return nsent;
}

////////////////////////////////////////////////////////////////////////////////
/// Send a character string buffer. Use kind to set the TMessage "what" field.
/// Returns the number of bytes in the string str that were sent and -1 in
/// case of error. In case the kind has been or'ed with kMESS_ACK, the call
/// will only return after having received an acknowledgement, making the
/// sending process synchronous.

Int_t TSocket::Send(const char *str, Int_t kind)
{
   TMessage mess(kind);
   if (str) mess.WriteString(str);

   Int_t nsent;
   if ((nsent = Send(mess)) < 0)
      return -1;

   return nsent - sizeof(Int_t);    // - TMessage::What()
}

////////////////////////////////////////////////////////////////////////////////
/// Send a TMessage object. Returns the number of bytes in the TMessage
/// that were sent and -1 in case of error. In case the TMessage::What
/// has been or'ed with kMESS_ACK, the call will only return after having
/// received an acknowledgement, making the sending process synchronous.
/// Returns -4 in case of kNoBlock and errno == EWOULDBLOCK.
/// Returns -5 if pipe broken or reset by peer (EPIPE || ECONNRESET).
/// support for streaming TStreamerInfo added by Rene Brun May 2008
/// support for streaming TProcessID added by Rene Brun June 2008

Int_t TSocket::Send(const TMessage &mess)
{
   TSystem::ResetErrno();

   if (fSocket < 0) return -1;

   if (mess.IsReading()) {
      Error("Send", "cannot send a message used for reading");
      return -1;
   }

   // send streamer infos in case schema evolution is enabled in the TMessage
   SendStreamerInfos(mess);

   // send the process id's so TRefs work
   SendProcessIDs(mess);

   mess.SetLength();   //write length in first word of buffer

   if (GetCompressionLevel() > 0 && mess.GetCompressionLevel() == 0)
      const_cast<TMessage&>(mess).SetCompressionSettings(fCompress);

   if (mess.GetCompressionLevel() > 0)
      const_cast<TMessage&>(mess).Compress();

   char *mbuf = mess.Buffer();
   Int_t mlen = mess.Length();
   if (mess.CompBuffer()) {
      mbuf = mess.CompBuffer();
      mlen = mess.CompLength();
   }

   ResetBit(TSocket::kBrokenConn);
   Int_t nsent;
   if ((nsent = gSystem->SendRaw(fSocket, mbuf, mlen, 0)) <= 0) {
      if (nsent == -5) {
         // Connection reset by peer or broken
         MarkBrokenConnection();
      }
      return nsent;
   }

   fBytesSent  += nsent;
   fgBytesSent += nsent;

   // If acknowledgement is desired, wait for it
   if (mess.What() & kMESS_ACK) {
      TSystem::ResetErrno();
      ResetBit(TSocket::kBrokenConn);
      char buf[2];
      Int_t n = 0;
      if ((n = gSystem->RecvRaw(fSocket, buf, sizeof(buf), 0)) < 0) {
         if (n == -5) {
            // Connection reset by peer or broken
            MarkBrokenConnection();
         } else
            n = -1;
         return n;
      }
      if (strncmp(buf, "ok", 2)) {
         Error("Send", "bad acknowledgement");
         return -1;
      }
      fBytesRecv  += 2;
      fgBytesRecv += 2;
   }

   Touch();  // update usage timestamp

   return nsent - sizeof(UInt_t);  //length - length header
}

////////////////////////////////////////////////////////////////////////////////
/// Send an object. Returns the number of bytes sent and -1 in case of error.
/// In case the "kind" has been or'ed with kMESS_ACK, the call will only
/// return after having received an acknowledgement, making the sending
/// synchronous.

Int_t TSocket::SendObject(const TObject *obj, Int_t kind)
{
   //stream object to message buffer
   TMessage mess(kind);
   mess.WriteObject(obj);

   //now sending the object itself
   Int_t nsent;
   if ((nsent = Send(mess)) < 0)
      return -1;

   return nsent;
}

////////////////////////////////////////////////////////////////////////////////
/// Send a raw buffer of specified length. Using option kOob one can send
/// OOB data. Returns the number of bytes sent or -1 in case of error.
/// Returns -4 in case of kNoBlock and errno == EWOULDBLOCK.
/// Returns -5 if pipe broken or reset by peer (EPIPE || ECONNRESET).

Int_t TSocket::SendRaw(const void *buffer, Int_t length, ESendRecvOptions opt)
{
   TSystem::ResetErrno();

   if (!IsValid()) return -1;

   ResetBit(TSocket::kBrokenConn);
   Int_t nsent;
   if ((nsent = gSystem->SendRaw(fSocket, buffer, length, (int) opt)) <= 0) {
      if (nsent == -5) {
         // Connection reset or broken: close
         MarkBrokenConnection();
      }
      return nsent;
   }

   fBytesSent  += nsent;
   fgBytesSent += nsent;

   Touch();  // update usage timestamp

   return nsent;
}

////////////////////////////////////////////////////////////////////////////////
/// Check if TStreamerInfo must be sent. The list of TStreamerInfo of classes
/// in the object in the message is in the fInfos list of the message.
/// We send only the TStreamerInfos not yet sent on this socket.

void TSocket::SendStreamerInfos(const TMessage &mess)
{
   if (mess.fInfos && mess.fInfos->GetEntries()) {
      TIter next(mess.fInfos);
      TStreamerInfo *info;
      TList *minilist = 0;
      while ((info = (TStreamerInfo*)next())) {
         Int_t uid = info->GetNumber();
         if (fBitsInfo.TestBitNumber(uid))
            continue; //TStreamerInfo had already been sent
         fBitsInfo.SetBitNumber(uid);
         if (!minilist)
            minilist = new TList();
         if (gDebug > 0)
            Info("SendStreamerInfos", "sending TStreamerInfo: %s, version = %d",
                 info->GetName(),info->GetClassVersion());
         minilist->Add(info);
      }
      if (minilist) {
         TMessage messinfo(kMESS_STREAMERINFO);
         messinfo.WriteObject(minilist);
         delete minilist;
         if (messinfo.fInfos)
            messinfo.fInfos->Clear();
         if (Send(messinfo) < 0)
            Warning("SendStreamerInfos", "problems sending TStreamerInfo's ...");
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Check if TProcessIDs must be sent. The list of TProcessIDs
/// in the object in the message is found by looking in the TMessage bits.
/// We send only the TProcessIDs not yet send on this socket.

void TSocket::SendProcessIDs(const TMessage &mess)
{
   if (mess.TestBitNumber(0)) {
      TObjArray *pids = TProcessID::GetPIDs();
      Int_t npids = pids->GetEntries();
      TProcessID *pid;
      TList *minilist = 0;
      for (Int_t ipid = 0; ipid < npids; ipid++) {
         pid = (TProcessID*)pids->At(ipid);
         if (!pid || !mess.TestBitNumber(pid->GetUniqueID()+1))
            continue;
         //check if a pid with this title has already been sent through the socket
         //if not add it to the fUUIDs list
         if (!fUUIDs) {
            fUUIDs = new TList();
            fUUIDs->SetOwner(kTRUE);
         } else {
            if (fUUIDs->FindObject(pid->GetTitle()))
               continue;
         }
         fUUIDs->Add(new TObjString(pid->GetTitle()));
         if (!minilist)
            minilist = new TList();
         if (gDebug > 0)
            Info("SendProcessIDs", "sending TProcessID: %s", pid->GetTitle());
         minilist->Add(pid);
      }
      if (minilist) {
         TMessage messpid(kMESS_PROCESSID);
         messpid.WriteObject(minilist);
         delete minilist;
         if (Send(messpid) < 0)
            Warning("SendProcessIDs", "problems sending TProcessID's ...");
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a character string message of maximum max length. The expected
/// message must be of type kMESS_STRING. Returns length of received string
/// (can be 0 if otherside of connection is closed) or -1 in case of error
/// or -4 in case a non-blocking socket would block (i.e. there is nothing
/// to be read).

Int_t TSocket::Recv(char *str, Int_t max)
{
   Int_t n, kind;

   ResetBit(TSocket::kBrokenConn);
   if ((n = Recv(str, max, kind)) <= 0) {
      if (n == -5) {
         SetBit(TSocket::kBrokenConn);
         n = -1;
      }
      return n;
   }

   if (kind != kMESS_STRING) {
      Error("Recv", "got message of wrong kind (expected %d, got %d)",
            kMESS_STRING, kind);
      return -1;
   }

   return n;
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a character string message of maximum max length. Returns in
/// kind the message type. Returns length of received string+4 (can be 0 if
/// other side of connection is closed) or -1 in case of error or -4 in
/// case a non-blocking socket would block (i.e. there is nothing to be read).

Int_t TSocket::Recv(char *str, Int_t max, Int_t &kind)
{
   Int_t     n;
   TMessage *mess;

   ResetBit(TSocket::kBrokenConn);
   if ((n = Recv(mess)) <= 0) {
      if (n == -5) {
         SetBit(TSocket::kBrokenConn);
         n = -1;
      }
      return n;
   }

   kind = mess->What();
   if (str) {
      if (mess->BufferSize() > (Int_t)sizeof(Int_t)) // if mess contains more than kind
         mess->ReadString(str, max);
      else
         str[0] = 0;
   }

   delete mess;

   return n;   // number of bytes read (len of str + sizeof(kind)
}

////////////////////////////////////////////////////////////////////////////////
/// Receives a status and a message type. Returns length of received
/// integers, 2*sizeof(Int_t) (can be 0 if other side of connection
/// is closed) or -1 in case of error or -4 in case a non-blocking
/// socket would block (i.e. there is nothing to be read).

Int_t TSocket::Recv(Int_t &status, Int_t &kind)
{
   Int_t     n;
   TMessage *mess;

   ResetBit(TSocket::kBrokenConn);
   if ((n = Recv(mess)) <= 0) {
      if (n == -5) {
         SetBit(TSocket::kBrokenConn);
         n = -1;
      }
      return n;
   }

   kind = mess->What();
   (*mess) >> status;

   delete mess;

   return n;   // number of bytes read (2 * sizeof(Int_t)
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a TMessage object. The user must delete the TMessage object.
/// Returns length of message in bytes (can be 0 if other side of connection
/// is closed) or -1 in case of error or -4 in case a non-blocking socket
/// would block (i.e. there is nothing to be read) or -5 if pipe broken
/// or reset by peer (EPIPE || ECONNRESET). In those case mess == 0.

Int_t TSocket::Recv(TMessage *&mess)
{
   TSystem::ResetErrno();

   if (!IsValid()) {
      mess = 0;
      return -1;
   }

oncemore:
   ResetBit(TSocket::kBrokenConn);
   Int_t  n;
   UInt_t len;
   if ((n = gSystem->RecvRaw(fSocket, &len, sizeof(UInt_t), 0)) <= 0) {
      if (n == 0 || n == -5) {
         // Connection closed, reset or broken
         MarkBrokenConnection();
      }
      mess = 0;
      return n;
   }
   len = net2host(len);  //from network to host byte order

   ResetBit(TSocket::kBrokenConn);
   char *buf = new char[len+sizeof(UInt_t)];
   if ((n = gSystem->RecvRaw(fSocket, buf+sizeof(UInt_t), len, 0)) <= 0) {
      if (n == 0 || n == -5) {
         // Connection closed, reset or broken
         MarkBrokenConnection();
      }
      delete [] buf;
      mess = 0;
      return n;
   }

   fBytesRecv  += n + sizeof(UInt_t);
   fgBytesRecv += n + sizeof(UInt_t);

   mess = new TMessage(buf, len+sizeof(UInt_t));

   // receive any streamer infos
   if (RecvStreamerInfos(mess))
      goto oncemore;

   // receive any process ids
   if (RecvProcessIDs(mess))
      goto oncemore;

   if (mess->What() & kMESS_ACK) {
      ResetBit(TSocket::kBrokenConn);
      char ok[2] = { 'o', 'k' };
      Int_t n2 = 0;
      if ((n2 = gSystem->SendRaw(fSocket, ok, sizeof(ok), 0)) < 0) {
         if (n2 == -5) {
            // Connection reset or broken
            MarkBrokenConnection();
         }
         delete mess;
         mess = 0;
         return n2;
      }
      mess->SetWhat(mess->What() & ~kMESS_ACK);

      fBytesSent  += 2;
      fgBytesSent += 2;
   }

   Touch();  // update usage timestamp

   return n;
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a raw buffer of specified length bytes. Using option kPeek
/// one can peek at incoming data. Returns number of received bytes.
/// Returns -1 in case of error. In case of opt == kOob: -2 means
/// EWOULDBLOCK and -3 EINVAL. In case of non-blocking mode (kNoBlock)
/// -4 means EWOULDBLOCK. Returns -5 if pipe broken or reset by
/// peer (EPIPE || ECONNRESET).

Int_t TSocket::RecvRaw(void *buffer, Int_t length, ESendRecvOptions opt)
{
   TSystem::ResetErrno();

   if (!IsValid()) return -1;
   if (length == 0) return 0;

   ResetBit(TSocket::kBrokenConn);
   Int_t n;
   if ((n = gSystem->RecvRaw(fSocket, buffer, length, (int) opt)) <= 0) {
      if (n == 0 || n == -5) {
         // Connection closed, reset or broken
         MarkBrokenConnection();
      }
      return n;
   }

   fBytesRecv  += n;
   fgBytesRecv += n;

   Touch();  // update usage timestamp

   return n;
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a message containing streamer infos. In case the message contains
/// streamer infos they are imported, the message will be deleted and the
/// method returns kTRUE.

Bool_t TSocket::RecvStreamerInfos(TMessage *mess)
{
   if (mess->What() == kMESS_STREAMERINFO) {
      TList *list = (TList*)mess->ReadObject(TList::Class());
      TIter next(list);
      TStreamerInfo *info;
      TObjLink *lnk = list->FirstLink();
      // First call BuildCheck for regular class
      while (lnk) {
         info = (TStreamerInfo*)lnk->GetObject();
         TObject *element = info->GetElements()->UncheckedAt(0);
         Bool_t isstl = element && strcmp("This",element->GetName())==0;
         if (!isstl) {
            info->BuildCheck();
            if (gDebug > 0)
               Info("RecvStreamerInfos", "importing TStreamerInfo: %s, version = %d",
                    info->GetName(), info->GetClassVersion());
         }
         lnk = lnk->Next();
      }
      // Then call BuildCheck for stl class
      lnk = list->FirstLink();
      while (lnk) {
         info = (TStreamerInfo*)lnk->GetObject();
         TObject *element = info->GetElements()->UncheckedAt(0);
         Bool_t isstl = element && strcmp("This",element->GetName())==0;
         if (isstl) {
            info->BuildCheck();
            if (gDebug > 0)
               Info("RecvStreamerInfos", "importing TStreamerInfo: %s, version = %d",
                    info->GetName(), info->GetClassVersion());
         }
         lnk = lnk->Next();
     }
      delete list;
      delete mess;

      return kTRUE;
   }
   return kFALSE;
}

////////////////////////////////////////////////////////////////////////////////
/// Receive a message containing process ids. In case the message contains
/// process ids they are imported, the message will be deleted and the
/// method returns kTRUE.

Bool_t TSocket::RecvProcessIDs(TMessage *mess)
{
   if (mess->What() == kMESS_PROCESSID) {
      TList *list = (TList*)mess->ReadObject(TList::Class());
      TIter next(list);
      TProcessID *pid;
      while ((pid = (TProcessID*)next())) {
         // check that a similar pid is not already registered in fgPIDs
         TObjArray *pidslist = TProcessID::GetPIDs();
         TIter nextpid(pidslist);
         TProcessID *p;
         while ((p = (TProcessID*)nextpid())) {
            if (!strcmp(p->GetTitle(), pid->GetTitle())) {
               delete pid;
               pid = 0;
               break;
            }
         }
         if (pid) {
            if (gDebug > 0)
               Info("RecvProcessIDs", "importing TProcessID: %s", pid->GetTitle());
            pid->IncrementCount();
            pidslist->Add(pid);
            Int_t ind = pidslist->IndexOf(pid);
            pid->SetUniqueID((UInt_t)ind);
         }
      }
      delete list;
      delete mess;

      return kTRUE;
   }
   return kFALSE;
}

////////////////////////////////////////////////////////////////////////////////
/// Set socket options.

Int_t TSocket::SetOption(ESockOptions opt, Int_t val)
{
   if (!IsValid()) return -1;

   return gSystem->SetSockOpt(fSocket, opt, val);
}

////////////////////////////////////////////////////////////////////////////////
/// Get socket options. Returns -1 in case of error.

Int_t TSocket::GetOption(ESockOptions opt, Int_t &val)
{
   if (!IsValid()) return -1;

   return gSystem->GetSockOpt(fSocket, opt, &val);
}

////////////////////////////////////////////////////////////////////////////////
/// Returns error code. Meaning depends on context where it is called.
/// If no error condition returns 0 else a value < 0.
/// For example see TServerSocket ctor.

Int_t TSocket::GetErrorCode() const
{
   if (!IsValid())
      return fSocket;

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// See comments for function SetCompressionSettings

void TSocket::SetCompressionAlgorithm(Int_t algorithm)
{
   if (algorithm < 0 || algorithm >= ROOT::RCompressionSetting::EAlgorithm::kUndefined) algorithm = 0;
   if (fCompress < 0) {
      fCompress = 100 * algorithm + ROOT::RCompressionSetting::ELevel::kUseMin;
   } else {
      int level = fCompress % 100;
      fCompress = 100 * algorithm + level;
   }
}

////////////////////////////////////////////////////////////////////////////////
/// See comments for function SetCompressionSettings

void TSocket::SetCompressionLevel(Int_t level)
{
   if (level < 0) level = 0;
   if (level > 99) level = 99;
   if (fCompress < 0) {
      // if the algorithm is not defined yet use 0 as a default
      fCompress = level;
   } else {
      int algorithm = fCompress / 100;
      if (algorithm >= ROOT::RCompressionSetting::EAlgorithm::kUndefined) algorithm = 0;
      fCompress = 100 * algorithm + level;
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Used to specify the compression level and algorithm:
///  settings = 100 * algorithm + level
///
///  level = 0, objects written to this file will not be compressed.
///  level = 1, minimal compression level but fast.
///  ....
///  level = 9, maximal compression level but slower and might use more memory.
/// (For the currently supported algorithms, the maximum level is 9)
/// If compress is negative it indicates the compression level is not set yet.
///
/// The enumeration ROOT::RCompressionSetting::EAlgorithm associates each
/// algorithm with a number. There is a utility function to help
/// to set the value of the argument. For example,
///   ROOT::CompressionSettings(ROOT::kLZMA, 1)
/// will build an integer which will set the compression to use
/// the LZMA algorithm and compression level 1.  These are defined
/// in the header file Compression.h.
///
/// Note that the compression settings may be changed at any time.
/// The new compression settings will only apply to branches created
/// or attached after the setting is changed and other objects written
/// after the setting is changed.

void TSocket::SetCompressionSettings(Int_t settings)
{
   fCompress = settings;
}

////////////////////////////////////////////////////////////////////////////////
/// Authenticated the socket with specified user.

Bool_t TSocket::Authenticate(const char *user)
{
   Bool_t rc = kFALSE;

   // Parse protocol name
   TString sproto = TUrl(fUrl).GetProtocol();
   if (sproto.Contains("sockd")) {
      fServType = kSOCKD;
   } else if (sproto.Contains("rootd")) {
      fServType = kROOTD;
   }
   if (gDebug > 2)
      Info("Authenticate","Local protocol: %s",sproto.Data());

   // Get server protocol level
   Int_t kind = kROOTD_PROTOCOL;
   // Warning: for backward compatibility reasons here we have to
   // send exactly 4 bytes: for fgClientClientProtocol > 99
   // the space in the format must be dropped
   if (fRemoteProtocol == -1) {
      if (Send(Form(" %d", fgClientProtocol), kROOTD_PROTOCOL) < 0) {
         return rc;
      }
      if (Recv(fRemoteProtocol, kind) < 0) {
         return rc;
      }
      //
      // If we are talking to an old rootd server we get a fatal
      // error here and we need to reopen the connection,
      // communicating first the size of the parallel socket
      if (kind == kROOTD_ERR) {
         fRemoteProtocol = 9;
         return kFALSE;
      }
   }

   // Find out whether authentication is required
   Bool_t runauth = kTRUE;
   if (fRemoteProtocol > 1000) {
      // Authentication not required by the remote server
      runauth = kFALSE;
      fRemoteProtocol %= 1000;
   }

   // If authentication is required, we need to find out which library
   // has to be loaded (preparation for near future, 9/7/05)
   TString host = GetInetAddress().GetHostName();
   if (runauth) {

      // Default (future)
      TString alib = "Xrd";
      if (fRemoteProtocol < 100) {
         // Standard Authentication lib
         alib = "Root";
      }

      // Load the plugin
      TPluginHandler *h =
         gROOT->GetPluginManager()->FindHandler("TVirtualAuth", alib);
      if (!h || h->LoadPlugin() != 0) {
         Error("Authenticate",
               "could not load properly %s authentication plugin", alib.Data());
         return rc;
      }

      // Get an instance of the interface class
      TVirtualAuth *auth = (TVirtualAuth *)(h->ExecPlugin(0));
      if (!auth) {
         Error("Authenticate", "could not instantiate the interface class");
         return rc;
      }
      if (gDebug > 1)
         Info("Authenticate", "class for '%s' authentication loaded", alib.Data());

      Option_t *opts = "";
      if (!(auth->Authenticate(this, host, user, opts))) {
         Error("Authenticate",
               "authentication attempt failed for %s@%s", user, host.Data());
      } else {
         rc = kTRUE;
      }
   } else {

      // Communicate who we are and our target user
      UserGroup_t *u = gSystem->GetUserInfo();
      if (u) {
         if (Send(Form("%s %s", u->fUser.Data(), user), kROOTD_USER) < 0)
            Warning("Authenticate", "problem sending kROOTD_USER (%s,%s)", u->fUser.Data(), user);
         delete u;
      } else
         if (Send(Form("-1 %s", user), kROOTD_USER) < 0)
            Warning("Authenticate", "problem sending kROOTD_USER (-1,%s)", user);

      rc = kFALSE;

      // Receive confirmation that everything went well
      Int_t stat;
      if (Recv(stat, kind) > 0) {

         if (kind == kROOTD_ERR) {
            if (gDebug > 0)
               TSocket::NetError("TSocket::Authenticate", stat);
         } else if (kind == kROOTD_AUTH) {

            // Authentication was not required: create inactive
            // security context for consistency
            fSecContext = new TSecContext(user, host, 0, -4, 0, 0);
            if (gDebug > 3)
               Info("Authenticate", "no authentication required remotely");

            // Set return flag;
            rc = 1;
         } else {
            if (gDebug > 0)
               Info("Authenticate", "expected message type %d, received %d",
                    kROOTD_AUTH, kind);
         }
      } else {
         if (gDebug > 0)
            Info("Authenticate", "error receiving message");
      }

   }

   return rc;
}

////////////////////////////////////////////////////////////////////////////////
/// Creates a socket or a parallel socket and authenticates to the
/// remote server.
///
/// url: [[proto][p][auth]://][user@]host[:port][/service]
///
/// where  proto = "sockd", "rootd"
///                indicates the type of remote server;
///                if missing "sockd" is assumed ("sockd" indicates
///                any remote server session using TServerSocket)
///       [auth] = "up" or "k" to force UsrPwd or Krb5 authentication
///       [port] = is the remote port number
///    [service] = service name used to determine the port
///                (for backward compatibility, specification of
///                 port as priority)
///
/// An already opened connection can be used by passing its socket
/// in opensock.
///
/// If 'err' is defined, '*err' on return from a failed call contains an error
/// code (see NetErrors.h).
///
/// Example:
///
///   TSocket::CreateAuthSocket("pk://qwerty@machine.fq.dn:5052",3)
///
///   creates an authenticated parallel socket of size 3 to a sockd
///   server running on remote machine machine.fq.dn on port 5052;
///   authentication will attempt protocol Kerberos first.
///
/// NB: may hang if the remote server is not of the correct type;
///     at present TSocket has no way to find out the type of the
///     remote server automatically
///
/// Returns pointer to an authenticated socket or 0 if creation or
/// authentication is unsuccessful.

TSocket *TSocket::CreateAuthSocket(const char *url, Int_t size, Int_t tcpwindowsize,
                                   TSocket *opensock, Int_t *err)
{
   R__LOCKGUARD2(gSocketAuthMutex);

   // Url to be passed to chosen constructor
   TString eurl(url);

   // Parse protocol, if any
   Bool_t parallel = kFALSE;
   TString proto(TUrl(url).GetProtocol());
   TString protosave = proto;

   // Get rid of authentication suffix
   TString asfx = "";
   if (proto.EndsWith("up") || proto.EndsWith("ug")) {
      asfx = proto;
      asfx.Remove(0,proto.Length()-2);
      proto.Resize(proto.Length()-2);
   } else if (proto.EndsWith("s") || proto.EndsWith("k") ||
              proto.EndsWith("g") || proto.EndsWith("h")) {
      asfx = proto;
      asfx.Remove(0,proto.Length()-1);
      proto.Resize(proto.Length()-1);
   }

   // Find out if parallel (force if rootd)
   if ((proto.EndsWith("p") || size > 1) ||
         proto.BeginsWith("root") ) {
      parallel = kTRUE;
      if (proto.EndsWith("p"))
         proto.Resize(proto.Length()-1);
   }

   // Force "sockd" if the rest is not recognized
   if (!proto.BeginsWith("sock") &&
       !proto.BeginsWith("root"))
      proto = "sockd";

   // Substitute this for original proto in eurl
   protosave += "://";
   proto += asfx;
   proto += "://";
   eurl.ReplaceAll(protosave,proto);

   // Create the socket now

   TSocket *sock = 0;
   if (!parallel) {

      // Simple socket
      if (opensock && opensock->IsValid())
         sock = opensock;
      else
         sock = new TSocket(eurl, TUrl(url).GetPort(), tcpwindowsize);

      // Authenticate now
      if (sock && sock->IsValid()) {
         if (!sock->Authenticate(TUrl(url).GetUser())) {
            // Nothing to do except setting the error code (if required) and sock to NULL
            if (err) {
               *err = (Int_t)kErrAuthNotOK;
               if (sock->TestBit(TSocket::kBrokenConn)) *err = (Int_t)kErrConnectionRefused;
            }
            sock->Close();
            delete sock;
            sock = 0;
         }
      }

   } else {

      // Tell TPSocket that we want authentication, which has to
      // be done using the original socket before creation of set
      // of parallel sockets
      if (eurl.Contains("?"))
         eurl.Resize(eurl.Index("?"));
      eurl += "?A";

      // Parallel socket
      if (opensock && opensock->IsValid())
         sock = new TPSocket(eurl, TUrl(url).GetPort(), size, opensock);
      else
         sock = new TPSocket(eurl, TUrl(url).GetPort(), size, tcpwindowsize);

      // Cleanup if failure ...
      if (sock && !sock->IsAuthenticated()) {
         // Nothing to do except setting the error code (if required) and sock to NULL
         if (err) {
            *err = (Int_t)kErrAuthNotOK;
            if (sock->TestBit(TSocket::kBrokenConn)) *err = (Int_t)kErrConnectionRefused;
         }
         if (sock->IsValid())
            // And except when the sock is valid; this typically
            // happens when talking to a old server, because the
            // the parallel socket system is open before authentication
            delete sock;
         sock = 0;
      }
   }

   return sock;
}

////////////////////////////////////////////////////////////////////////////////
/// Creates a socket or a parallel socket and authenticates to the
/// remote server specified in 'url' on remote 'port' as 'user'.
///
/// url: [[proto][auth]://]host
///
/// where  proto = "sockd", "rootd"
///                indicates the type of remote server
///                if missing "sockd" is assumed ("sockd" indicates
///                any remote server session using TServerSocket)
///       [auth] = "up" or "k" to force UsrPwd or Krb5 authentication
///
/// An already opened connection can be used by passing its socket
/// in opensock.
///
/// If 'err' is defined, '*err' on return from a failed call contains an error
/// code (see NetErrors.h).
///
/// Example:
///
///   TSocket::CreateAuthSocket("qwerty","pk://machine.fq.dn:5052",3)
///
///   creates an authenticated parallel socket of size 3 to a sockd
///   server running on remote machine machine.fq.dn on port 5052;
///   authentication will attempt protocol Kerberos first.
///
/// NB: may hang if the remote server is not of the correct type;
///     at present TSocket has no way to find out the type of the
///     remote server automatically
///
/// Returns pointer to an authenticated socket or 0 if creation or
/// authentication is unsuccessful.

TSocket *TSocket::CreateAuthSocket(const char *user, const char *url,
                                   Int_t port, Int_t size, Int_t tcpwindowsize,
                                   TSocket *opensock, Int_t *err)
{
   R__LOCKGUARD2(gSocketAuthMutex);

   // Extended url to be passed to base call
   TString eurl;

   // Add protocol, if any
   if (TString(TUrl(url).GetProtocol()).Length() > 0) {
      eurl += TString(TUrl(url).GetProtocol());
      eurl += TString("://");
   }
   // Add user, if any
   if (!user || strlen(user) > 0) {
      eurl += TString(user);
      eurl += TString("@");
   }
   // Add host
   eurl += TString(TUrl(url).GetHost());
   // Add port
   eurl += TString(":");
   eurl += (port > 0 ? port : 0);
   // Add options, if any
   if (TString(TUrl(url).GetOptions()).Length() > 0) {
      eurl += TString("/?");
      eurl += TString(TUrl(url).GetOptions());
   }

   // Create the socket and return it
   return TSocket::CreateAuthSocket(eurl,size,tcpwindowsize,opensock,err);
}

////////////////////////////////////////////////////////////////////////////////
/// Static method returning supported client protocol.

Int_t TSocket::GetClientProtocol()
{
   return fgClientProtocol;
}

////////////////////////////////////////////////////////////////////////////////
/// Print error string depending on error code.

void TSocket::NetError(const char *where, Int_t err)
{
   // Make sure it is in range
   err = (err < kErrError) ? ((err > -1) ? err : 0) : kErrError;

   if (gDebug > 0)
      ::Error(where, "%s", gRootdErrStr[err]);
}

////////////////////////////////////////////////////////////////////////////////
/// Get total number of bytes sent via all sockets.

ULong64_t TSocket::GetSocketBytesSent()
{
   return fgBytesSent;
}

////////////////////////////////////////////////////////////////////////////////
/// Get total number of bytes received via all sockets.

ULong64_t TSocket::GetSocketBytesRecv()
{
   return fgBytesRecv;
}
