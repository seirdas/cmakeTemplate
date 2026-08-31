#include "logic/PacketCoreBase.hpp"

#include <string_view>
#include <functional>


// General ------------------------------------------------------------------------------

PacketCoreBase::PacketCoreBase() :
    IModule(),
    last_packet_hash_(0)
{

}

PacketCoreBase::~PacketCoreBase() {

}


// Protegido ------------------------------------------------------------------------------

bool PacketCoreBase::isDuplicatePacket(void const* data, std::size_t size) {
    if (!data || size == 0)
        return false;

    std::string_view sv(static_cast<char const*>(data), size);
    std::size_t hash_actual = std::hash<std::string_view>{}(sv);

    std::lock_guard<std::mutex> lock(dedup_mtx_);
    if (hash_actual == last_packet_hash_)
        return true;

    last_packet_hash_ = static_cast<unsigned long>(hash_actual);
    return false;
}
