#ifndef PEER_H
#define PEER_H

#include <array>
#include <cassert>

#include "ds/protocolmanager.h"
#include "ds/connectionsocket.h"
#include "ds/peerconnection.h"
#include "ds/file.h"

namespace ds {
namespace prot {


class Peer : public core::PeerConnection
{
    Q_OBJECT
public:
    using ptr_t = std::shared_ptr<Peer>;
    using mview_t = crypto::MemoryView<uint8_t>;
    using data_t = crypto::MemoryView<uint8_t>;
    using stream_state_t = crypto_secretstream_xchacha20poly1305_state;
    static constexpr size_t crypt_bytes = crypto_secretstream_xchacha20poly1305_ABYTES;
    enum class InState {
        DISABLED,
        CHUNK_SIZE,
        CHUNK_DATA,
        CLOSING
    };

    struct Hello {
        static constexpr size_t bytes = 1 /* version */
                + crypto_secretstream_xchacha20poly1305_KEYBYTES
                + crypto_secretstream_xchacha20poly1305_HEADERBYTES
                + crypto_sign_PUBLICKEYBYTES /* pubkey */
                + crypto_sign_BYTES;
        Hello()
            : buffer{}, version{buffer.data(), 1}
            , key{version.end(), crypto_secretstream_xchacha20poly1305_KEYBYTES}
            , header{key.end(), crypto_secretstream_xchacha20poly1305_HEADERBYTES}
            , pubkey{header.end(), crypto_sign_PUBLICKEYBYTES}
            , signature{pubkey.end(), crypto_sign_BYTES}
        {
            Q_ASSERT((1 + key.size() + header.size() + pubkey.size() + signature.size())
                   == buffer.size());
        }

        std::array<uint8_t, bytes> buffer;
        mview_t version;
        mview_t key;
        mview_t header;
        mview_t pubkey;
        mview_t signature;
    };

    struct Olleh {
        static constexpr size_t bytes = 1 /* version */
                + crypto_secretstream_xchacha20poly1305_KEYBYTES
                + crypto_secretstream_xchacha20poly1305_HEADERBYTES
                + crypto_sign_BYTES;
        Olleh()
            : buffer{}, version{buffer.data(), 1}
            , key{version.end(), crypto_secretstream_xchacha20poly1305_KEYBYTES}
            , header{key.end(), crypto_secretstream_xchacha20poly1305_HEADERBYTES}
            , signature{header.end(), crypto_sign_BYTES}
        {
            Q_ASSERT((1 + key.size() + header.size() + signature.size())
                   == buffer.size());
        }

        std::array<uint8_t, bytes> buffer;
        mview_t version;
        mview_t key;
        mview_t header;
        mview_t signature;
    };

    class Channel {
    public:
        using ptr_t = std::shared_ptr<Channel>;

        virtual ~Channel() = default;
        virtual void onIncoming(Peer& peer, const quint64 id,
                                const mview_t& data, const bool final) = 0;

        // Return 0 on EOF
        virtual uint64_t onOutgoing(Peer& peer) = 0;
    };

    Peer(ConnectionSocket::ptr_t connection,
         core::ConnectData connectionData);
    ~Peer() override = default;

    ConnectionSocket& getConnection() {
        if (!connection_) {
            throw std::runtime_error("No connection object");
        }
        return *connection_;
    }

    ConnectionSocket::ptr_t getConnectionPtr() {
        if (!connection_) {
            throw std::runtime_error("No connection object");
        }
        return connection_;
    }

    const core::ConnectData& getConnectData() const {
        return connectionData_;
    }

public slots:
    virtual void authorize(bool /*authorize*/) override {}

    // Send a request to a connected peer over the encrypted stream
    // Returns a unique id for the request (within the scope of this peer)
    uint64_t send(const QJsonDocument& json);

    // Final is true for the last block of a file-transfer to indicate EOF.
    uint64_t send(const void *data, const size_t bytes, const quint32 channel, const bool final = false);

signals:
    void incomingPeer(const std::shared_ptr<PeerConnection>& peer);
    void closeLater();
    void removeTransfer(core::File::Direction direction, const quint32 id);

private slots:
    void onCloseLater();

protected:
    void onReceivedData(const quint32 channel, const quint64 id,
                        const mview_t& data, const bool final);
    void onReceivedJson(const quint64 id, const mview_t& data);
    void enableEncryptedStream();
    void wantChunkSize();
    void wantChunkData(const size_t bytes);
    void processStream(const data_t& data);
    void prepareEncryption(stream_state_t& state, mview_t& header, mview_t& key);
    void prepareDecryption(stream_state_t& state, const mview_t& header, const mview_t& key);
    void decrypt(mview_t& data, const mview_t& ciphertext, bool& final);
    QByteArray safePayload(const mview_t& data);
    quint32 createChannel(const core::File& file);
    uint64_t startReceive(core::File& file);
    uint64_t startSend(core::File& file);
    void useConnection(ConnectionSocket *cc);

    InState inState_ = InState::DISABLED;
    ConnectionSocket::ptr_t connection_;
    core::ConnectData connectionData_;
    stream_state_t stateIn = {};
    stream_state_t stateOut = {};
    quint64 request_id_ = {}; // Counter for outgoing requests
    quint32 nextInchannel_ = 1;
    std::map<quint32, Channel::ptr_t> outChannels_;
    std::map<quint32, Channel::ptr_t> inChannels_;
    bool notificationsDisabled_ = false;

    // PeerConnection interface
public:
    const QUuid uuid_;
    QUuid getConnectionId() const override;
    crypto::DsCert::ptr_t getPeerCert() const noexcept override;
    void close() override;
    QUuid getIdentityId() const noexcept override;
    uint64_t sendAck(const QString& what, const QString& status, const QString& data) override;
    uint64_t sendAck(const QString& what, const QString& status, const QVariantMap& params) override;
    bool isConnected() const noexcept override;
    uint64_t sendUserInfo(const core::UserInfo &userInfo) override;
    uint64_t sendMessage(const core::Message &message) override;
    uint64_t sendAvatar(const QImage& avatar) override;
    uint64_t offerFile(const core::File& file) override;
    uint64_t startTransfer(core::File& file) override;
    uint64_t sendSome(core::File& file) override;
    void disableNotifications() override;
};

}} // namespaces


#endif // PEER_H
