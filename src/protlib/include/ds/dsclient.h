#ifndef DSCLIENT_H
#define DSCLIENT_H

#include "ds/peer.h"

namespace ds {
namespace prot {

/*! Client implementation of the DarkSpeak protocol
 *
 */
class DsClient : public Peer
{
    Q_OBJECT
public:
    using ptr_t = std::shared_ptr<DsClient>;

    enum class State {
        CONNECTED,
        GET_OLLEH,
        ENCRYPTED_STREAM
    };

    DsClient(ConnectionSocket::ptr_t connection, core::ConnectData connectionData);

private slots:
    void advance();
    void advance(const data_t& data);


private:
    void sayHello();
    void getHelloReply(const data_t& data);
    void startConnectRetryTimer();
    void initConnections();

    State state_ = State::CONNECTED;
    size_t maxReconnects_ = 20;
    size_t numReconnects_ = {};
    size_t reconnectDelayMilliseconds_ = 20000;

    // PeerConnection interface
public:
    Direction getDirection() const noexcept override {
        return OUTGOING;
    }
};

}} // namespaces

#endif // DSCLIENT_H
