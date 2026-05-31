#include "devices/TotalMix.hpp"

#include <cstring>
#include <cstdio>

#include <winsock2.h>
#include <windows.h>

// PIMPL para socket Windows
struct TotalMix::Impl {
    SOCKET      socket     = INVALID_SOCKET;   ///< Socket UDP para el envío de paquetes OSC.
};

// General ------------------------------------------------------------------------------

TotalMix::TotalMix() : pimpl_(std::make_unique<Impl>())
{
    // Inicialización del buffer OSC apuntando a oscRaw_
    oscBuf_.data = oscRaw_;
    oscBuf_.size = OSC_BUF_SIZE;
    OscReset();
}

TotalMix::~TotalMix()
{
    if (pimpl_->socket != INVALID_SOCKET) {
        closesocket(pimpl_->socket);
        pimpl_->socket = INVALID_SOCKET;
    }
    WSACleanup();
}

// Conexión a TotalMix ------------------------------------------------------------------

TMError TotalMix::init(int localPort, const std::string& localIP,
                        int remotePort, const std::string& remoteIP,
                        int numInputs, int numPlaybacks, int numOutputs)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return TMError::InitFailed;

    pimpl_->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pimpl_->socket == INVALID_SOCKET)
        return TMError::InitFailed;

    sockaddr_in local{};
    local.sin_family      = AF_INET;
    local.sin_port        = htons((u_short)localPort);
    local.sin_addr.s_addr = inet_addr(localIP.c_str());

    if (bind(pimpl_->socket, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        closesocket(pimpl_->socket);
        pimpl_->socket = INVALID_SOCKET;
        return TMError::InitFailed;
    }

    remotePort_   = remotePort;
    remoteIP_     = remoteIP;
    numInputs_    = numInputs;
    numPlaybacks_ = numPlaybacks;
    numOutputs_   = numOutputs;

    // Redimensiona los vectores 
    bankPosOutput_.resize(numOutputs + 1); 
    bankPosInput_.resize(numInputs + 1);
    bankPosPlayback_.resize(numPlaybacks + 1);

    BuildBankMaps();
    return TMError::OK;
}


// Control de volumen -----------------------------------------------------------------------

TMError TotalMix::SetOutputVolume(int out, float pct)
{
    return SendVolume(Bus::Output, 0, out, PctTodB(pct));
}

TMError TotalMix::SetInputVolume(int out, int in, float pct)
{
    return SendVolume(Bus::Input, in, out, PctTodB(pct));
}

TMError TotalMix::SetPlaybackVolume(int out, int pb, float pct)
{
    return SendVolume(Bus::Playback, pb, out, PctTodB(pct));
}


// Control de volumen  (dB) -------------------------------------------------------------

TMError TotalMix::SetOutputVolumedB(int out, float dB)
{
    return SendVolume(Bus::Output, 0, out, dB);
}

TMError TotalMix::SetInputVolumedB(int out, int in, float dB)
{
    return SendVolume(Bus::Input, out, in, dB);
}

TMError TotalMix::SetPlaybackVolumedB(int out, int pb, float dB)
{
    return SendVolume(Bus::Playback, out, pb, dB);
}


// Control de Mute -----------------------------------------------------------------------

TMError TotalMix::SetMuteOutput(int out, bool mute)
{
    return SendMute(Bus::Output, out, mute);
}

TMError TotalMix::SetMuteInput(int in, bool mute)
{
    return SendMute(Bus::Input, in, mute);
}

TMError TotalMix::SetMutePlayback(int pb, bool mute)
{
    return SendMute(Bus::Playback, pb, mute);
}


// Miscelánea ---------------------------------------------------------------------------

TMError TotalMix::SetSnapshot(int index)
{
    char addr[64];
    snprintf(addr, sizeof(addr), "/3/snapshots/8/%d", index);

    OscReset();
    OscTimeTag tt{ 0, 1 };
    if (!OscOpenBundle(tt))                  return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes(addr,  ",f"))  return TMError::BufferOverflow;
    if (!OscWriteFloat(1.0f))                return TMError::BufferOverflow;
    if (!OscCloseAll())                      return TMError::BufferOverflow;
    return SendPacket();
}

TMError TotalMix::SetInputThreshold(int in, float threshold)
{
    if (in < 1 || in > numInputs_) return TMError::ChannelOutOfRange;
    float threshold_effective = threshold/100.0f;

    OscReset();
    OscTimeTag tt{ 0, 1 };
    if (!OscOpenBundle(tt))                             return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes("/1/busInput",  ",f"))    return TMError::BufferOverflow;
    if (!OscWriteFloat(1.0f))                           return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes("/setBankStart", ",f"))   return TMError::BufferOverflow;
    if (!OscWriteFloat((float)(in - 1)))                return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes("/2/expTrsh",   ",f"))    return TMError::BufferOverflow;
    if (!OscWriteFloat(threshold_effective))            return TMError::BufferOverflow;
    if (!OscCloseAll())                                 return TMError::BufferOverflow;
    return SendPacket();
}


// Métodos OSC --------------------------------------------------------------------------

void TotalMix::OscReset()
{
    oscBuf_.ptr         = oscBuf_.data;
    oscBuf_.state       = OSC_EMPTY;
    oscBuf_.bundleDepth = 0;
    oscBuf_.outerStamp  = nullptr;
    oscBuf_.typePtr     = nullptr;
    oscBuf_.firstUntyped = false;
    memset(oscBuf_.prevCounts, 0, sizeof(oscBuf_.prevCounts));
}

int TotalMix::OscFreeSpace() const
{
    return oscBuf_.size - (int)(oscBuf_.ptr - oscBuf_.data);
}

bool TotalMix::OscCheckTag(char expected)
{
    if (!oscBuf_.typePtr) return true;
    if (*oscBuf_.typePtr != expected) return false;
    ++oscBuf_.typePtr;
    return true;
}

void TotalMix::OscPatchMsgSize()
{
    int size = (int)(oscBuf_.ptr - (char*)oscBuf_.thisMsgSize - 4);
    *oscBuf_.thisMsgSize = htonl(size);
}

int TotalMix::OscPadString(char* dest, const char* str)
{
    size_t len = std::strlen(str);
    std::memcpy(dest, str, len + 1); // Copia el string y el '\0'
    int padLen = static_cast<int>(len) + 1;
    while (padLen % OSC_STRING_ALIGN != 0) {
        dest[padLen++] = '\0';
    }
    return padLen;
}

int TotalMix::OscEffectiveStringLen(const char* str) const
{
    int len = (int)strlen(str) + 1;
    if (len % OSC_STRING_ALIGN != 0)
        len += OSC_STRING_ALIGN - (len % OSC_STRING_ALIGN);
    return len;
}

bool TotalMix::OscOpenBundle(OscTimeTag tt)
{
    if (oscBuf_.bundleDepth >= OSC_MAX_BUNDLE_NESTING) return false;
    if (!OscCheckTag('\0'))                             return false;
    if (oscBuf_.state == OSC_GET_ARGS) OscPatchMsgSize();

    int needed = (oscBuf_.state == OSC_EMPTY) ? 16 : 20;
    if (OscFreeSpace() < needed) return false;

    if (oscBuf_.state != OSC_EMPTY) {
        *((int*)oscBuf_.ptr) = 0xaaaaaaaa;
        oscBuf_.prevCounts[++oscBuf_.bundleDepth] = (int*)oscBuf_.ptr;
        oscBuf_.ptr += 4;
    } else {
        ++oscBuf_.bundleDepth;
    }

    oscBuf_.ptr += OscPadString(oscBuf_.ptr, "#bundle");

    *((OscTimeTag*)oscBuf_.ptr) = tt;
    if (oscBuf_.state == OSC_EMPTY)
        oscBuf_.outerStamp = (OscTimeTag*)oscBuf_.ptr;

    if (htonl(1) != 1) {   // big-endian byte swap
        int* p = (int*)oscBuf_.ptr;
        p[0] = htonl(p[0]);
        p[1] = htonl(p[1]);
    }
    oscBuf_.ptr += sizeof(OscTimeTag);

    oscBuf_.state        = OSC_NEED_COUNT;
    oscBuf_.typePtr      = nullptr;
    oscBuf_.firstUntyped = false;
    return true;
}

bool TotalMix::OscCloseBundle()
{
    if (oscBuf_.bundleDepth == 0) return false;
    if (!OscCheckTag('\0'))       return false;
    if (oscBuf_.state == OSC_GET_ARGS) OscPatchMsgSize();

    if (oscBuf_.bundleDepth == 1) {
        oscBuf_.state = OSC_DONE;
    } else {
        int size = (int)(oscBuf_.ptr - (char*)oscBuf_.prevCounts[oscBuf_.bundleDepth] - 4);
        *oscBuf_.prevCounts[oscBuf_.bundleDepth] = htonl(size);
        oscBuf_.state = OSC_NEED_COUNT;
    }
    --oscBuf_.bundleDepth;
    oscBuf_.typePtr      = nullptr;
    oscBuf_.firstUntyped = false;
    return true;
}

bool TotalMix::OscCloseAll()
{
    if (oscBuf_.bundleDepth == 0) return false;
    if (!OscCheckTag('\0'))       return false;
    while (oscBuf_.bundleDepth > 0) OscCloseBundle();
    oscBuf_.typePtr = nullptr;
    return true;
}

bool TotalMix::OscWriteAddrAndTypes(const char* name, const char* types)
{
    int paddedName  = OscEffectiveStringLen(name);
    int paddedTypes = OscEffectiveStringLen(types);

    if (!OscCheckTag('\0')) return false;

    if (oscBuf_.state == OSC_EMPTY) {
        if (OscFreeSpace() < paddedName) return false;
        oscBuf_.state = OSC_ONE_MSG_ARGS;
    } else {
        if (OscFreeSpace() < 4 + paddedName) return false;
        if (oscBuf_.state == OSC_GET_ARGS) OscPatchMsgSize();
        oscBuf_.thisMsgSize  = (int*)oscBuf_.ptr;
        *oscBuf_.thisMsgSize = 0xbbbbbbbb;
        oscBuf_.ptr += 4;
        oscBuf_.state = OSC_GET_ARGS;
    }

    oscBuf_.ptr += OscPadString(oscBuf_.ptr, name);

    if (OscFreeSpace() < paddedTypes) return false;
    oscBuf_.typePtr = oscBuf_.ptr + 1;   // saltar la coma inicial
    oscBuf_.ptr    += OscPadString(oscBuf_.ptr, types);
    oscBuf_.firstUntyped = false;
    return true;
}

bool TotalMix::OscWriteFloat(float val)
{
    if (OscFreeSpace() < 4) return false;
    if (!OscCheckTag('f'))   return false;
    int* ip = (int*)(&val);
    *((int*)oscBuf_.ptr) = htonl(*ip);
    oscBuf_.ptr += 4;
    oscBuf_.firstUntyped = false;
    return true;
}


// Envío de paquete OSC -----------------------------------------------------------------

TMError TotalMix::SendPacket()
{
    int size = (int)(oscBuf_.ptr - oscBuf_.data);
    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons((u_short)remotePort_);
    dest.sin_addr.s_addr = inet_addr(remoteIP_.c_str());

    int sent = sendto(pimpl_->socket, oscBuf_.data, size, 0,
                      (sockaddr*)&dest, sizeof(dest));
    return (sent == SOCKET_ERROR) ? TMError::SocketError : TMError::OK;
}

TMError TotalMix::SendVolume(Bus bus, int out, int channel, float dB)
{
    int         maxChannel;
    const char* busAddr;
    const std::vector<int>* bankMap = nullptr;

    switch (bus) {
        case Bus::Output:   maxChannel = numOutputs_;   busAddr = "/1/busOutput";   bankMap = &bankPosOutput_;   break;
        case Bus::Input:    maxChannel = numInputs_;    busAddr = "/1/busInput";    bankMap = &bankPosInput_;    break;
        case Bus::Playback: maxChannel = numPlaybacks_; busAddr = "/1/busPlayback"; bankMap = &bankPosPlayback_; break;
        default: return TMError::ChannelOutOfRange;
    }

    if (channel < 1 || channel > maxChannel)              return TMError::ChannelOutOfRange;
    if (bus != Bus::Output && (out < 1 || out > numOutputs_)) return TMError::ChannelOutOfRange;

    char volumeAddr[32];
    snprintf(volumeAddr, sizeof(volumeAddr), "/1/volume%d", (*bankMap)[channel]);

    OscReset();
    OscTimeTag tt{ 0, 1 };
    if (!OscOpenBundle(tt))                           return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes(busAddr,       ",f"))   return TMError::BufferOverflow;
    if (!OscWriteFloat(1.0f))                         return TMError::BufferOverflow;

    if (bus != Bus::Output) {
        if (!OscWriteAddrAndTypes("/setSubmix", ",f")) return TMError::BufferOverflow;
        if (!OscWriteFloat((float)(out - 1)))          return TMError::BufferOverflow;
    }

    if (!OscWriteAddrAndTypes("/setBankStart", ",f"))         return TMError::BufferOverflow;
    if (!OscWriteFloat((float)BankStartFor(channel)))         return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes(volumeAddr,      ",f"))         return TMError::BufferOverflow;
    if (!OscWriteFloat(dBtoFader(dB)))                        return TMError::BufferOverflow;
    if (!OscCloseAll())                                       return TMError::BufferOverflow;
    return SendPacket();
}

TMError TotalMix::SendMute(Bus bus, int channel, bool mute)
{
    int         maxChannel;
    const char* busAddr;
    const std::vector<int>* bankMap = nullptr;

    switch (bus) {
        case Bus::Output:   maxChannel = numOutputs_;   busAddr = "/1/busOutput";   bankMap = &bankPosOutput_;   break;
        case Bus::Input:    maxChannel = numInputs_;    busAddr = "/1/busInput";    bankMap = &bankPosInput_;    break;
        case Bus::Playback: maxChannel = numPlaybacks_; busAddr = "/1/busPlayback"; bankMap = &bankPosPlayback_; break;
        default: return TMError::ChannelOutOfRange;
    }

    if (channel < 1 || channel > maxChannel) return TMError::ChannelOutOfRange;

    char muteAddr[32];
    snprintf(muteAddr, sizeof(muteAddr), "/1/mute/1/%d", bankMap[channel]);

    OscReset();
    OscTimeTag tt{ 0, 1 };
    if (!OscOpenBundle(tt))                           return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes(busAddr,       ",f"))   return TMError::BufferOverflow;
    if (!OscWriteFloat(1.0f))                         return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes("/setBankStart", ",f")) return TMError::BufferOverflow;
    if (!OscWriteFloat((float)BankStartFor(channel))) return TMError::BufferOverflow;
    if (!OscWriteAddrAndTypes(muteAddr,      ",f"))   return TMError::BufferOverflow;
    if (!OscWriteFloat(mute ? 1.0f : 0.0f))           return TMError::BufferOverflow;
    if (!OscCloseAll())                                return TMError::BufferOverflow;
    return SendPacket();
}


// Banks --------------------------------------------------------------------------------

void TotalMix::BuildBankMaps()
{
    auto fill = [](std::vector<int>& map, int total) {
        map.assign(total + 1, 0);
        
        for (int i = 1; i <= total; ++i)
            map[i] = (i % 8 == 0) ? 8 : (i % 8);

        int rem = total % 8;
        if (rem != 0) {
            int groupStart = (total / 8) * 8 + 1;
            int bankPos    = 8 - rem + 1;
            for (int i = groupStart; i <= total; ++i, ++bankPos)
                map[i] = bankPos;
        }
    };

    fill(bankPosOutput_,    numOutputs_);
    fill(bankPosInput_,     numInputs_);
    fill(bankPosPlayback_,  numPlaybacks_);
}

int TotalMix::BankStartFor(int channel) const
{
    return 8 * ((channel - 1) / 8);
}


// Utilidades ---------------------------------------------------------------------------

float TotalMix::dBtoFader(float dB)
{
    if (dB >= 6.0f)   return 1.0f;
    if (dB >= -7.0f)  return dB * 0.0304f  + 0.8176f;
    if (dB >= -9.0f)  return dB * 0.0234f  + 0.7684f;
    if (dB >= -12.0f) return dB * 0.0190f  + 0.7289f;
    if (dB >= -65.0f) return dB * 0.00937f + 0.6105f;
    return 0.0f;
}

float TotalMix::PctTodB(float pct)
{
    if (pct > 100.0f) pct = 100.0f;
    if (pct <   0.01f) pct =  0.01f;
    float dB = 35.0f * log10f(pct / 100.0f) + 6.0f;
    if (dB >   6.0f) dB =   6.0f;
    if (dB < -70.0f) dB = -70.0f;
    return dB;
}
