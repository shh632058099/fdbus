/*
 * Copyright (C) 2015   Jeremy Chen jeremy_cz@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CFDINTERFACENAMESERVER_H__
#define __CFDINTERFACENAMESERVER_H__

#include <common_base/CFdbSimpleSerializer.h>
#include <utils/CFdbIfMsgTokens.h>

namespace NFdbBase {
enum FdbNsMsgCode
{
    REQ_ALLOC_SERVICE_ADDRESS = 0,
    REQ_REGISTER_SERVICE = 1,
    REQ_UNREGISTER_SERVICE = 2,

    REQ_QUERY_SERVICE = 3,
    REQ_QUERY_SERVICE_INTER_MACHINE = 4,

    REQ_QUERY_HOST_LOCAL = 5,

    NTF_SERVICE_ONLINE = 6,
    NTF_SERVICE_ONLINE_INTER_MACHINE = 7,
    NTF_MORE_ADDRESS = 8,
    NTF_SERVICE_ONLINE_MONITOR = 9,
    NTF_SERVICE_ONLINE_MONITOR_INTER_MACHINE = 10,

    NTF_HOST_ONLINE_LOCAL = 11,
    NTF_HOST_INFO = 12,

    NTF_WATCHDOG = 13
};

enum FdbHsMsgCode
{
    REQ_REGISTER_HOST = 0,
    REQ_UNREGISTER_HOST = 1,
    REQ_QUERY_HOST = 2,
    REQ_HEARTBEAT_OK = 3,
    REQ_HOST_READY = 4,

    NTF_HOST_ONLINE = 5,
    NTF_HEART_BEAT = 6,
};

class FdbMsgAddressItem : public IFdbParcelable
{
public:
    FdbMsgAddressItem()
        : mOptions(0)
    {
    }
    std::string &tcp_ipc_address()
    {
        return mTCPIPCAddress;
    }
    void set_tcp_ipc_address(const char *addr)
    {
        mTCPIPCAddress = addr;
    }
    void set_tcp_ipc_address(const std::string &addr)
    {
        mTCPIPCAddress = addr;
    }
    int32_t tcp_port() const
    {
        return mTCPPort;
    }
    void set_tcp_port(int32_t port)
    {
        mTCPPort = port;
    }
    EFdbSocketType address_type()
    {
        return mType;
    }
    void set_address_type(EFdbSocketType type)
    {
        mType = type;
    }
    std::string &tcp_ipc_url()
    {
        return mTCPIPCUrl;
    }
    void set_tcp_ipc_url(const char *url)
    {
        mTCPIPCUrl = url;
    }
    void set_tcp_ipc_url(const std::string &url)
    {
        mTCPIPCUrl = url;
    }
    int32_t udp_port() const
    {
        return mUDPPort;
    }
    void set_udp_port(int32_t port)
    {
        mUDPPort = port;
        mOptions |= mMaskHasUDPPort;
    }
    bool has_udp_port() const
    {
        return !!(mOptions & mMaskHasUDPPort);
    }
    void fromSocketAddress(const CFdbSocketAddr &sckt_addr)
    {
        mTCPIPCAddress = sckt_addr.mAddr;
        mTCPPort = sckt_addr.mPort;
        mType = sckt_addr.mType;
        mTCPIPCUrl = sckt_addr.mUrl;
    }
    void toSocketAddress(CFdbSocketAddr &sckt_addr)
    {
        sckt_addr.mAddr = mTCPIPCAddress;
        sckt_addr.mPort = mTCPPort;
        sckt_addr.mType = mType;
        sckt_addr.mUrl = mTCPIPCUrl;
    }
    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mTCPIPCAddress
                   << mTCPPort
                   << (uint8_t)mType
                   << mTCPIPCUrl
                   << mOptions;
        if (mOptions & mMaskHasUDPPort)
        {
            serializer << mUDPPort;
        }
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        uint8_t type;
        deserializer >> mTCPIPCAddress
                     >> mTCPPort
                     >> type
                     >> mTCPIPCUrl
                     >> mOptions;
        mType = (EFdbSocketType)type;
        if (mOptions & mMaskHasUDPPort)
        {
            deserializer >> mUDPPort;
        }
    }
private:
    std::string mTCPIPCAddress;
    int32_t mTCPPort;
    EFdbSocketType mType;

    std::string mTCPIPCUrl;
    int32_t mUDPPort;
    uint8_t mOptions;
        static const uint8_t mMaskHasUDPPort = 1 << 0;
};

class FdbMsgAddressList : public IFdbParcelable
{
public:
    FdbMsgAddressList()
        : mOptions(0)
    {}
    std::string &service_name()
    {
        return mServiceName;
    }
    void set_service_name(const std::string &name)
    {
        mServiceName = name;
    }
    void set_service_name(const char *name)
    {
        mServiceName = name;
    }
    std::string &host_name()
    {
        return mHostName;
    }
    void set_host_name(const std::string &name)
    {
        mHostName = name;
    }
    void set_host_name(const char *name)
    {
        mHostName = name;
    }
    bool is_local() const
    {
        return mIsLocal;
    }
    void set_is_local(bool local)
    {
        mIsLocal = local;
    }
    CFdbParcelableArray<FdbMsgAddressItem> &address_list()
    {
        return mAddressList;
    }
    FdbMsgAddressItem *add_address_list()
    {
        return mAddressList.Add();
    }
    NFdbBase::FdbMsgTokens &token_list()
    {
        mOptions |= mMaskTokenList;
        return mTokenList;
    }
    bool has_token_list() const
    {
        return !!(mOptions & mMaskTokenList);
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mServiceName
                   << mHostName
                   << mIsLocal
                   << mAddressList
                   << mOptions;
        if (mOptions & mMaskTokenList)
        {
            serializer << mTokenList;
        }
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mServiceName
                     >> mHostName
                     >> mIsLocal
                     >> mAddressList
                     >> mOptions;
        if (mOptions & mMaskTokenList)
        {
            deserializer >> mTokenList;
        }
    }
private:
    std::string mServiceName;
    std::string mHostName;
    bool mIsLocal;
    CFdbParcelableArray<FdbMsgAddressItem> mAddressList;
    FdbMsgTokens mTokenList;
    uint8_t mOptions;
        static const uint8_t mMaskTokenList = 1 << 0;
};

class FdbAddrBindStatus : public IFdbParcelable
{
public:
    const std::string &request_address() const
    {
        return mRequestAddr;
    }
    void request_address(const std::string &addr)
    {
        mRequestAddr = addr;
    }
    const std::string &bind_address() const
    {
        return mBindAddr;
    }
    void bind_address(const std::string &addr)
    {
        mBindAddr = addr;
    }
    int32_t udp_port() const
    {
        return mUDPPort;
    }
    void set_udp_port(int32_t port)
    {
        mUDPPort = port;
    }
protected:
    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mRequestAddr << mBindAddr << mUDPPort;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mRequestAddr >> mBindAddr >> mUDPPort;
    }

private:
    std::string mRequestAddr;
    std::string mBindAddr;
    int32_t mUDPPort;
};

class FdbMsgAddrBindResults : public IFdbParcelable 
{
public:
    const std::string &service_name() const
    {
        return mServiceName;
    }
    void service_name(const std::string &name)
    {
        mServiceName = name;
    }
    CFdbParcelableArray<FdbAddrBindStatus> &address_list()
    {
        return mAddrList;
    }
    FdbAddrBindStatus *add_address_list()
    {
        return mAddrList.Add();
    }
protected:
    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mServiceName
                   << mAddrList;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mServiceName
                     >> mAddrList;
    }
private:
    std::string mServiceName;
    CFdbParcelableArray<FdbAddrBindStatus> mAddrList;
};

class FdbMsgServerName : public IFdbParcelable
{
public:
    std::string &name()
    {
        return mName;
    }
    void set_name(const std::string &n)
    {
        mName = n;
    }
    void set_name(const char *n)
    {
        mName = n;
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mName;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mName;
    }
private:
    std::string mName;
};

class FdbMsgHostAddress : public IFdbParcelable
{
public:
    FdbMsgHostAddress()
        : mOptions(0)
    {}
    std::string &ip_address()
    {
        return mIpAddress;
    }
    void set_ip_address(const std::string &address)
    {
        mIpAddress = address;
    }
    void set_ip_address(const char *address)
    {
        mIpAddress = address;
    }
    std::string &ns_url()
    {
        return mNsUrl;
    }
    void set_ns_url(const std::string &url)
    {
        mNsUrl = url;
    }
    void set_ns_url(const char *url)
    {
        mNsUrl = url;
    }
    std::string &host_name()
    {
        return mHostName;
    }
    void set_host_name(const std::string &name)
    {
        mHostName = name;
    }
    void set_host_name(const char *name)
    {
        mHostName = name;
    }
    FdbMsgTokens &token_list()
    {
        mOptions |= mMaskTokenList;
        return mTokenList;
    }
    bool has_token_list() const
    {
        return !!(mOptions & mMaskTokenList);
    }
    void set_cred(const char *cred)
    {
        mCred = cred;
        mOptions |= mMaskCred;
    }
    std::string &cred()
    {
        return mCred;
    }
    bool has_cred() const
    {
        return !!(mOptions & mMaskCred);
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mIpAddress
                   << mNsUrl
                   << mHostName
                   << mOptions;
        if (mOptions & mMaskTokenList)
        {
            serializer << mTokenList;
        }
        if (mOptions & mMaskCred)
        {
            serializer << mCred;
        }
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mIpAddress
                     >> mNsUrl
                     >> mHostName
                     >> mOptions;
        if (mOptions & mMaskTokenList)
        {
            deserializer >> mTokenList;
        }
        if (mOptions & mMaskCred)
        {
            deserializer >> mCred;
        }
    }
private:
    std::string mIpAddress;
    std::string mNsUrl;
    std::string mHostName;
    FdbMsgTokens mTokenList;
    std::string mCred;
    uint8_t mOptions;
        static const uint8_t mMaskTokenList = 1 << 0;
        static const uint8_t mMaskCred = 1 << 1;
};

class FdbMsgHostRegisterAck : public IFdbParcelable
{
public:
    FdbMsgHostRegisterAck()
        : mOptions(0)
    {}
    NFdbBase::FdbMsgTokens &token_list()
    {
        mOptions |= mMaskTokenList;
        return mTokenList;
    }
    bool has_token_list() const
    {
        return !!(mOptions & mMaskTokenList);
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mOptions;
        if (mOptions & mMaskTokenList)
        {
            serializer << mTokenList;
        }
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mOptions;
        if (mOptions & mMaskTokenList)
        {
            deserializer >> mTokenList;
        }
    }
private:
    FdbMsgTokens mTokenList;
    uint8_t mOptions;
        static const uint8_t mMaskTokenList = 1 << 0;
};

class FdbMsgHostInfo : public IFdbParcelable
{
public:
    std::string &name()
    {
        return mName;
    }
    void set_name(const std::string &n)
    {
        mName = n;
    }
    void set_name(const char *n)
    {
        mName = n;
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mName;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mName;
    }
private:
    std::string mName;
};

class FdbMsgHostAddressList : public IFdbParcelable
{
public:
    CFdbParcelableArray<FdbMsgHostAddress> &address_list()
    {
        return mAddressList;
    }
    FdbMsgHostAddress *add_address_list()
    {
        return mAddressList.Add();
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mAddressList;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mAddressList;
    }
private:
    CFdbParcelableArray<FdbMsgHostAddress> mAddressList;
};

class FdbMsgServiceInfo : public IFdbParcelable
{
public:
    FdbMsgAddressList &service_addr()
    {
        return mServiceAddr;
    }
    FdbMsgHostAddress &host_addr()
    {
        return mHostAddr;
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mServiceAddr
                   << mHostAddr;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mServiceAddr
                     >> mHostAddr;
    }
private:
   FdbMsgAddressList mServiceAddr;
   FdbMsgHostAddress mHostAddr;
};

class FdbMsgServiceTable : public IFdbParcelable
{
public:
    CFdbParcelableArray<FdbMsgServiceInfo> &service_tbl()
    {
        return mServiceTbl;
    }
    FdbMsgServiceInfo *add_service_tbl()
    {
        return mServiceTbl.Add();
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mServiceTbl;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mServiceTbl;
    }
private:
    CFdbParcelableArray<FdbMsgServiceInfo> mServiceTbl;
};

enum FdbMsgDogStatus
{
    FDB_DOG_ST_DIE = -1,
    FDB_DOG_ST_OK = 0,
    FDB_DOG_ST_NON_EXIST = 1
};

class FdbMsgClientInfo : public IFdbParcelable
{
public:
    const std::string &peer_name() const
    {
        return mPeerName;
    }
    void set_peer_name(const char *name)
    {
        mPeerName = name;
    }
    const std::string &peer_address() const
    {
        return mPeerAddress;
    }
    void set_peer_address(const char *address)
    {
        mPeerAddress = address;
    }
    int32_t security_level() const
    {
        return mSecurityLevel;
    }
    void set_security_level(int32_t sec_level)
    {
        mSecurityLevel = sec_level;
    }
    int32_t udp_port() const
    {
        return mUDPPort;
    }
    void set_udp_port(int32_t port)
    {
        mUDPPort = port;
    }
    FdbMsgDogStatus dog_status() const
    {
        return mDogStatus;
    }
    void set_dog_status(FdbMsgDogStatus dog_status)
    {
        mDogStatus = dog_status;
    }
    int32_t pid() const
    {
        return mPid;
    }
    void set_pid(uint32_t pid)
    {
        mPid = pid;
    }
    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mPeerName
                   << mPeerAddress
                   << mSecurityLevel
                   << mUDPPort
                   << (int8_t)mDogStatus
                   << mPid;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        int8_t dog_status;
        deserializer >> mPeerName
                     >> mPeerAddress
                     >> mSecurityLevel
                     >> mUDPPort
                     >> dog_status
                     >> mPid;
        mDogStatus = (FdbMsgDogStatus)dog_status;
    }
private:
    std::string mPeerName;
    std::string mPeerAddress;
    int32_t mSecurityLevel;
    int32_t mUDPPort;
    FdbMsgDogStatus mDogStatus;
    uint32_t mPid;
};

class FdbMsgClientTable : public IFdbParcelable
{
public:
    const std::string &endpoint_name() const
    {
        return mEndpointName;
    }
    void set_endpoint_name(const char *endpoint)
    {
        mEndpointName = endpoint;
    }
    const std::string &server_name() const
    {
        return mServerName;
    }
    void set_server_name(const char *name)
    {
        mServerName = name;
    }
    CFdbParcelableArray<FdbMsgClientInfo> &client_tbl()
    {
        return mClientTbl;
    }
    FdbMsgClientInfo *add_client_tbl()
    {
        return mClientTbl.Add();
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mEndpointName
                   << mServerName 
                   << mClientTbl;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mEndpointName
                     >> mServerName 
                     >> mClientTbl;
    }
private:
    std::string mEndpointName;
    std::string mServerName;
    CFdbParcelableArray<FdbMsgClientInfo> mClientTbl;
};

class FdbMsgEventCacheItem : public IFdbParcelable
{
public:
    int32_t event() const
    {
        return mEvent;
    }
    void set_event(int32_t event)
    {
        mEvent = event;
    }
    const std::string &topic() const
    {
        return mTopic;
    }
    void set_topic(const char *topic)
    {
        mTopic = topic;
    }
    int32_t size() const
    {
        return mSize;
    }
    void set_size(int32_t size)
    {
        mSize = size;
    }

    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mEvent 
                   << mTopic 
                   << mSize;
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mEvent 
                     >> mTopic 
                     >> mSize;
    }
private:
    int32_t mEvent;
    std::string mTopic;
    int32_t mSize;
};

class FdbMsgEventCache : public IFdbParcelable
{
public:
    CFdbParcelableArray<FdbMsgEventCacheItem> &cache()
    {
        return mCache;
    }
    FdbMsgEventCacheItem *add_cache()
    {
        return mCache.Add();
    }
    void serialize(CFdbSimpleSerializer &serializer) const
    {
        serializer << mCache; 
    }
    void deserialize(CFdbSimpleDeserializer &deserializer)
    {
        deserializer >> mCache; 
    }
    
private:
    CFdbParcelableArray<FdbMsgEventCacheItem> mCache;
};

}

#endif

