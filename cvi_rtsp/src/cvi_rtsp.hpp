#pragma once

#include <RTSPServer.hh>
#include <RTSPCommon.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

typedef void (ConnCallback) (const char *ip, void *arg);

#define DEFAULT_TIMEOUT 65
#define DEFAULT_TCP_BUF_SIZE 512 * 1024

class CVI_RTSP: public RTSPServer {
    public:
        static CVI_RTSP* createNew(UsageEnvironment *env, CVI_RTSP_CONFIG *config);
        virtual ~CVI_RTSP();
        void setListener(ConnCallback *connCb, void *argConn, ConnCallback *disconnCb, void *argDisconn);
    protected:
        CVI_RTSP(UsageEnvironment& env, int ourSocket, Port ourPort, unsigned timeout, const CVI_RTSP_CONFIG &config);
        virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr);
        virtual ClientSession* createNewClientSession(u_int32_t sessionId);
    protected:
        friend class CVI_ClientConn;
        friend class CVI_ClientSession;
        ConnCallback *onConnect;
        ConnCallback *onDisconnect;
        void *_argConn;
        void *_argDisconn;
        CVI_RTSP_CONFIG _config;
    public:
        class CVI_ClientConn: public RTSPServer::RTSPClientConnection {
            public:
                virtual ~CVI_ClientConn();
                CVI_ClientConn(CVI_RTSP& ourServer, int clientSocket, struct sockaddr_in clientAddr);
            protected:
                friend class CVI_RTSP;
        };

        class CVI_ClientSession: public RTSPServer::RTSPClientSession {
            public:
                virtual ~CVI_ClientSession();
                CVI_ClientSession(CVI_RTSP& ourServer, u_int32_t sessionId);
            protected:
                virtual void handleCmd_PLAY(RTSPClientConnection* ourClientConnection, ServerMediaSubsession* subsession, char const* fullRequestStr);
                friend class CVI_RTSP;
        };
};

CVI_RTSP* CVI_RTSP::createNew(UsageEnvironment *env, CVI_RTSP_CONFIG *config) {
    Port port(config->port == 0? 554: config->port);
    int ourSocket = setUpOurSocket(*env, port);
    if (-1 == ourSocket) {
        return NULL;
    }

    unsigned timeout = config->timeout <= 0? DEFAULT_TIMEOUT: config->timeout;

    return new CVI_RTSP(*env, ourSocket, port, timeout, *config);
}

CVI_RTSP::CVI_RTSP(UsageEnvironment& env, int ourSocket, Port ourPort, unsigned timeout, const CVI_RTSP_CONFIG &config): RTSPServer(env, ourSocket, ourPort, NULL, timeout)
{
    onConnect = NULL;
    onDisconnect = NULL;
    _argConn = NULL;
    _argDisconn = NULL;
    _config = config;
}

CVI_RTSP::~CVI_RTSP()
{

}

void CVI_RTSP::setListener(ConnCallback *connCb, void *argConn, ConnCallback *disconnCb, void *argDisconn)
{
    onConnect = connCb;
    _argConn = argConn;
    onDisconnect = disconnCb;
    _argDisconn = argDisconn;
}

CVI_RTSP::CVI_ClientConn::CVI_ClientConn(CVI_RTSP& ourServer, int clientSocket, struct sockaddr_in clientAddr): RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr)
{
    CVI_RTSP &server = static_cast<CVI_RTSP &>(fOurServer);
    if (server.onConnect != NULL) {
        char ip[32] = {0};
        inet_ntop(AF_INET, &fClientAddr.sin_addr, ip, sizeof(ip));
        server.onConnect(ip, server._argConn);
    }
}

CVI_RTSP::CVI_ClientConn::~CVI_ClientConn()
{
    CVI_RTSP &server = static_cast<CVI_RTSP &>(fOurServer);
    if (server.onDisconnect != NULL) {
        server.onDisconnect(inet_ntoa(fClientAddr.sin_addr), server._argDisconn);
    }
}

GenericMediaServer::ClientConnection* CVI_RTSP::createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
{
    // increase tcp buf size
    // live555 default tcp buf is 50KB, it is tool small for high bitrate video
    // don't worry the memory usage, default system limit is 320KB
    increaseSendBufferTo(envir(), clientSocket, _config.tcpBufSize == 0? DEFAULT_TCP_BUF_SIZE: _config.tcpBufSize);

    return new CVI_RTSP::CVI_ClientConn(*this, clientSocket, clientAddr);
}

// client session
GenericMediaServer::ClientSession* CVI_RTSP::createNewClientSession(u_int32_t sessionId) {
    return new CVI_RTSP::CVI_ClientSession(*this, sessionId);
}

CVI_RTSP::CVI_ClientSession::CVI_ClientSession(CVI_RTSP& ourServer, u_int32_t sessionId): RTSPServer::RTSPClientSession(ourServer, sessionId)
{
}

CVI_RTSP::CVI_ClientSession::~CVI_ClientSession()
{
}

void CVI_RTSP::CVI_ClientSession::handleCmd_PLAY(RTSPServer::RTSPClientConnection* ourClientConnection, ServerMediaSubsession* subsession, char const* fullRequestStr)
{
    CVI_ServerMediaSession *sms = static_cast<CVI_ServerMediaSession *>(fOurServerMediaSession);
    if (sms->maxConn() > 0 && sms->referenceCount() > (unsigned)sms->maxConn()) {
        std::cout << "max connections reached, current connections: " << sms->referenceCount() << std::endl;
        setRTSPResponse(ourClientConnection, "403 forbidden, max connections reached ", fOurSessionId);
        sms->decrementReferenceCount();
        return;
    }

    RTSPServer::RTSPClientSession::handleCmd_PLAY(ourClientConnection, subsession, fullRequestStr);
}
