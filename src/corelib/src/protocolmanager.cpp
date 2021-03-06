#include <memory>

#include "ds/torprotocolmanager.h"

using namespace std;


namespace ds {
namespace core {


ProtocolManager::ptr_t ProtocolManager::create(QSettings& settings, ProtocolManager::Transport)
{
    return make_shared<ds::prot::TorProtocolManager>(settings);
}


}} // namespaces

