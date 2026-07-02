#include "links/VoIPRec.hpp"
#include "net/NetMgr.hpp"           // Necesario para el acceso a los sockets
#include "sound/SoundMgr.hpp"       // necesario para el acceso a las muestras de audio
#include "system/SystemMgr.hpp"     // logs de la app
#include <cstring>
#include <algorithm>

// General ------------------------------------------------------------------------------

VoIPRec::VoIPRec() : 
    sampleRateHz_(11025),    
    channels_(1),             // legacy: deintercalaba a mono
    framesPerPacket_(250),    // legacy: 250x2 = 500 bytes
    encoding_(AudioEncoding::Legacy_sendComs),
    voxThreshold_(0),
    voxTailFrames_(100)       // legacy: ≈ceroscont<20
{

}

VoIPRec::~VoIPRec() {

}

bool VoIPRec::init() {

    return false;
}


// Estado -------------------------------------------------------------------------------

bool VoIPRec::start() {
    
    // Comprobar si ya está corriendo
    if (!running_)
        return true;

    // Comprobar que el socket existe   (#TODO)    
    if (!net_->socketExists(5555))
        return false;



    return true;
}

bool VoIPRec::stop() {

    return false;
}


// Paquete de datos de audio ------------------------------------------------------------

std::vector<char> VoIPRec::buildLegacyPacket() {
    LegacyAudioPacket pkt{};

    for (int i = 0; i < 8; ++i)
        pkt.txIds[i] = 0x01;        // #TODO

    pkt.coding    = 0;
    pkt.freq      = static_cast<uint16_t>(sampleRateHz_);
    pkt.b_s       = 2;
    pkt.nsec      = nsec_;
    pkt.timeStamp = 0;
    pkt.spare     = 0;
    pkt.size      = static_cast<uint16_t>(framesPerPacket_);

    size_t n = std::min(framesPerPacket_, static_cast<uint32_t>(250));
    std::memcpy(pkt.voice, audioBuffer_.data(), n * sizeof(int16_t));

    const char* raw = reinterpret_cast<const char*>(&pkt);
    return std::vector<char>(raw, raw + sizeof(LegacyAudioPacket));
}