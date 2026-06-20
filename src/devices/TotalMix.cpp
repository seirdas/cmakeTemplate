#include "devices/TotalMix.hpp"
#include "system/SystemMgr.hpp"

// Totalmix solo se puede descargar para Windows/Mac. Se usan las funciones de socket de Windows directamente.
#ifdef WIN32

    #include <cstring>
    #include <cstdio>

    #include <winsock2.h>
    #include <ws2tcpip.h>       // inet_pton
    #include <windows.h>

    static constexpr const char* MODULE = "TotalMix";

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

    bool TotalMix::init(int localPort, const std::string& localIP,
                        int remotePort, const std::string& remoteIP,
                        int numInputs, int numPlaybacks, int numOutputs)
    {
        // Iniciar WSA (Contexto de red Windows)
        if (!wsaStarted_) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                SYS_WARN(MODULE, "init: WSAStartup failed.");
                return false;
            }
            wsaStarted_ = true;
        }
    
        // Cerrar socket previo si existía
        if (pimpl_->socket != INVALID_SOCKET) {
            closesocket(pimpl_->socket);
            pimpl_->socket = INVALID_SOCKET;
        }
    
        pimpl_->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (pimpl_->socket == INVALID_SOCKET) {
            SYS_WARN(MODULE,"Cannot create UDP Socket");
            return false;
        }
    
        sockaddr_in local{};
        local.sin_family      = AF_INET;
        local.sin_port        = htons(static_cast<u_short>(localPort));

        if (inet_pton(AF_INET, localIP.c_str(), &local.sin_addr) != 1) {
            closesocket(pimpl_->socket);
            pimpl_->socket = INVALID_SOCKET;
            SYS_WARN(MODULE, "Invalid local IP address: " + localIP);
            return false;
        }
    
        remotePort_   = remotePort;
        remoteIP_     = remoteIP;
        numInputs_    = numInputs;
        numPlaybacks_ = numPlaybacks;
        numOutputs_   = numOutputs;
    
        BuildBankMaps();
        return true;
    }


    // Control de volumen -----------------------------------------------------------------------

    bool TotalMix::SetOutputVolume(int out, float value, bool in_dB_units)
    {
        return SendVolume(Bus::Output, 0, out,  (in_dB_units) ? (value) : PctTodB(value));
    }

    bool TotalMix::SetInputVolume(int out, int in, float value, bool in_dB_units)
    {
        return SendVolume(Bus::Input, in, out, (in_dB_units) ? (value) : PctTodB(value));
    }

    bool TotalMix::SetPlaybackVolume(int out, int pb, float value, bool in_dB_units)
    {
        return SendVolume(Bus::Playback, pb, out, (in_dB_units) ? (value) : PctTodB(value));
    }


    // Control de Mute -----------------------------------------------------------------------

    bool TotalMix::SetMuteOutput(int out, bool mute)
    {
        return SendMute(Bus::Output, out, mute);
    }

    bool TotalMix::SetMuteInput(int in, bool mute)
    {
        return SendMute(Bus::Input, in, mute);
    }

    bool TotalMix::SetMutePlayback(int pb, bool mute)
    {
        return SendMute(Bus::Playback, pb, mute);
    }


    // Miscelánea ---------------------------------------------------------------------------

    bool TotalMix::SetSnapshot(int index)
    {
        char addr[64];
        snprintf(addr, sizeof(addr), "/3/snapshots/8/%d", index);

        OscReset();
        OscTimeTag tt{ 0, 1 };

        if (!OscOpenBundle(tt)                  ||
            !OscWriteAddrAndTypes(addr,  ",f")  ||
            !OscWriteFloat(1.0f)                ||
            !OscCloseAll()                      ) 
        {
            SYS_WARN(MODULE,"SetSnapshot: Buffer overflow");
            return false;
        }

        return SendPacket();
    }

    bool TotalMix::SetInputThreshold(int in, float threshold)
    {
        if (in < 1 || in > numInputs_) {
            SYS_WARN(MODULE,"InputThreshold: Channel " + std::to_string(in) + " out of range");
            return false;
        }

        float threshold_effective = threshold/100.0f;

        OscReset();
        OscTimeTag tt{ 0, 1 };
        if (!OscOpenBundle(tt) ||
            !OscWriteAddrAndTypes("/1/busInput",  ",f")     ||
            !OscWriteFloat(1.0f)                            ||
            !OscWriteAddrAndTypes("/setBankStart", ",f")    ||
            !OscWriteFloat(static_cast<float>(in - 1))      ||
            !OscWriteAddrAndTypes("/2/expTrsh",   ",f")     ||
            !OscWriteFloat(threshold_effective)             ||
            !OscCloseAll()                                  ) 
        {
            SYS_WARN(MODULE,"InputThreshold: Buffer overflow");
            return false;
        }

        return SendPacket();
    }


    // Envío de paquete OSC -----------------------------------------------------------------

    bool TotalMix::SendPacket()
    {
        int size = static_cast<int>(oscBuf_.ptr - oscBuf_.data);
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port   = htons(static_cast<u_short>(remotePort_));
        
        // CAMBIO: inet_addr por inet_pton
        if (inet_pton(AF_INET, remoteIP_.c_str(), &dest.sin_addr) != 1) {
            SYS_WARN(MODULE, "Invalid remote IP address: " + remoteIP_);
            return false;
        }

        int sent = sendto(pimpl_->socket, oscBuf_.data, size, 0,
                         reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

        if (sent == SOCKET_ERROR) {
            SYS_WARN(MODULE, "Socket error on send packet.");
            return false;
        }
        else return true;
    }

    bool TotalMix::SendVolume(Bus bus, int out, int channel, float dB)
    {
        int         maxChannel;
        const char* busAddr;
        const std::array<int, MAX_TOTAL_CHANNELS + 1>* bankMap = nullptr;

        switch (bus) {
            case Bus::Output:   maxChannel = numOutputs_;   busAddr = "/1/busOutput";   bankMap = &bankPosOutput_;   break;
            case Bus::Input:    maxChannel = numInputs_;    busAddr = "/1/busInput";    bankMap = &bankPosInput_;    break;
            case Bus::Playback: maxChannel = numPlaybacks_; busAddr = "/1/busPlayback"; bankMap = &bankPosPlayback_; break;
            default: 
                SYS_WARN(MODULE, "SendVolume: Error getting bus.");
                return false;
        }

        if (channel < 1 || channel > maxChannel) {
            SYS_WARN(MODULE,"SendVolume: Channel " + std::to_string(channel) + " out of range");
            return false;
        }
        if (bus != Bus::Output && (out < 1 || out > numOutputs_)) {
            SYS_WARN(MODULE,"SendVolume: Channel " + std::to_string(out) + " out of range");
            return false;
        }

        char volumeAddr[32];
        snprintf(volumeAddr, sizeof(volumeAddr), "/1/volume%d", (*bankMap)[channel]);

        // Escribe el mensaje OSC
        OscReset();
        OscTimeTag tt{ 0, 1 };
        if (!OscOpenBundle(tt)                  || 
            !OscWriteAddrAndTypes(busAddr,",f") ||
            !OscWriteFloat(1.0f)                ) 
        {
            SYS_WARN(MODULE,"SendVolume: Buffer overflow");
            return false;
        }

        if (bus != Bus::Output) {
            if (!OscWriteAddrAndTypes("/setSubmix", ",f")   ||
                !OscWriteFloat(static_cast<float>(out - 1)) )
            {
                SYS_WARN(MODULE,"SendVolume: Buffer overflow");
                return false;
            }
        }

        if (!OscWriteAddrAndTypes("/setBankStart", ",f")                ||
            !OscWriteFloat(static_cast<float>(BankStartFor(channel)))   ||
            !OscWriteAddrAndTypes(volumeAddr,      ",f")                ||
            !OscWriteFloat(dBtoFader(dB))                               ||
            !OscCloseAll()                                              )
        {
            SYS_WARN(MODULE,"SendVolume: Buffer overflow");
            return false;
        }       
        return SendPacket();
    }

    bool TotalMix::SendMute(Bus bus, int channel, bool mute)
    {
        int         maxChannel;
        const char* busAddr;
        const std::array<int, MAX_TOTAL_CHANNELS + 1>* bankMap = nullptr;

        switch (bus) {
            case Bus::Output:   maxChannel = numOutputs_;   busAddr = "/1/busOutput";   bankMap = &bankPosOutput_;   break;
            case Bus::Input:    maxChannel = numInputs_;    busAddr = "/1/busInput";    bankMap = &bankPosInput_;    break;
            case Bus::Playback: maxChannel = numPlaybacks_; busAddr = "/1/busPlayback"; bankMap = &bankPosPlayback_; break;
            default: 
                SYS_WARN(MODULE, "SendMute: Error getting bus.");
                return false;
        }

        if (channel < 1 || channel > maxChannel) {
            SYS_WARN(MODULE,"SendMute: Channel " + std::to_string(channel) + " out of range");
            return false;
        }

        char muteAddr[32];
        snprintf(muteAddr, sizeof(muteAddr), "/1/mute/1/%d", (*bankMap)[channel]);

        // Escribe el mensaje OSC
        OscReset();
        OscTimeTag tt{ 0, 1 };
        if (!OscOpenBundle(tt)                                          ||
            !OscWriteAddrAndTypes(busAddr,       ",f")                  ||
            !OscWriteFloat(1.0f)                                        ||
            !OscWriteAddrAndTypes("/setBankStart", ",f")                ||
            !OscWriteFloat(static_cast<float>(BankStartFor(channel)))   ||
            !OscWriteAddrAndTypes(muteAddr,       ",f")                 ||
            !OscWriteFloat(mute ? 1.0f : 0.0f)                          ||
            !OscCloseAll()                                              )
        {
            SYS_WARN(MODULE,"SendMute: Buffer overflow");
            return false;
        }
        return SendPacket();
    }


    // Banks --------------------------------------------------------------------------------

    void TotalMix::BuildBankMaps()
    {
        auto fill = [](std::array<int, MAX_TOTAL_CHANNELS + 1>& map, int total) {
            // Eliminadas las líneas de resize y assign porque el tamaño ya es estático y está a 0.
        
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
        return oscBuf_.size - static_cast<int>(oscBuf_.ptr - oscBuf_.data);
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
        int size = static_cast<int>( oscBuf_.ptr - reinterpret_cast<char*>(oscBuf_.thisMsgSize - 4) );
        *oscBuf_.thisMsgSize = htonl(size);
    }

    int TotalMix::OscPadString(char* dest, const char* str)
    {
        size_t len = std::strlen(str);
        int padLen = (len + 4) & ~3; 

        std::memcpy(dest, str, len);
        // Llenar el resto con ceros (cubriendo el \0 y el padding)
        std::memset(dest + len, 0, padLen - len); 

        return padLen;
    }

    int TotalMix::OscEffectiveStringLen(const char* str) const
    {
        // Sumamos 4 (1 para el \0 obligatorio + 3 para el redondeo) y aplicamos la máscara
        return (static_cast<int>(std::strlen(str)) + 4) & ~3;
    }

    bool TotalMix::OscOpenBundle(OscTimeTag tt)
    {
        if (oscBuf_.bundleDepth >= OSC_MAX_BUNDLE_NESTING) return false;
        if (!OscCheckTag('\0'))                             return false;
        if (oscBuf_.state == OSC_GET_ARGS) OscPatchMsgSize();

        int needed = (oscBuf_.state == OSC_EMPTY) ? 16 : 20;
        if (OscFreeSpace() < needed) return false;

        ++oscBuf_.bundleDepth;
        if (oscBuf_.state != OSC_EMPTY) {
            *reinterpret_cast<int*> (oscBuf_.ptr) = 0xaaaaaaaa;
            oscBuf_.prevCounts[oscBuf_.bundleDepth] = reinterpret_cast<int*>(oscBuf_.ptr);
            oscBuf_.ptr += 4;
        }
        
        oscBuf_.ptr += OscPadString(oscBuf_.ptr, "#bundle");

        *(reinterpret_cast<OscTimeTag*> (oscBuf_.ptr)) = tt;
        if (oscBuf_.state == OSC_EMPTY)
            oscBuf_.outerStamp = reinterpret_cast<OscTimeTag*> (oscBuf_.ptr);

        if (htonl(1) != 1) {   // big-endian byte swap
            int* p = reinterpret_cast<int*>(oscBuf_.ptr);
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
            int size = (int)(oscBuf_.ptr - reinterpret_cast<char*>(oscBuf_.prevCounts[oscBuf_.bundleDepth] - 4) );
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
            oscBuf_.thisMsgSize  = reinterpret_cast<int*> (oscBuf_.ptr);
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
        if (OscFreeSpace() < 4 || !OscCheckTag('f')) return false;

        uint32_t net_val;
        std::memcpy(&net_val, &val, sizeof(float));
        net_val = htonl(net_val);

        std::memcpy(oscBuf_.ptr, &net_val, 4);

        oscBuf_.ptr += 4;
        oscBuf_.firstUntyped = false;
        return true;
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct TotalMix::Impl {};

// General ------------------------------------------------------------------------------
    TotalMix::TotalMix() : pimpl_(std::make_unique<Impl>()) {
        SYS_WARN("GuiMgr", "Totalmix not compatible in non-Windows SO.");
    }
    TotalMix::~TotalMix() { }
    bool TotalMix::init(int localPort, const std::string& localIP,
                        int remotePort, const std::string& remoteIP,
                        int numInputs, int numPlaybacks, int numOutputs) 
    {
        return false;
    }

// Control de volumen -------------------------------------------------------
    bool TotalMix::SetOutputVolume(int out, float pct, bool in_dB_units)            { return false; }
    bool TotalMix::SetInputVolume(int out, int in, float pct, bool in_dB_units)     { return false; }
    bool TotalMix::SetPlaybackVolume(int out, int pb, float pct, bool in_dB_units)  { return false; }

// Control de Mute ----------------------------------------------------------
    bool TotalMix::SetMuteOutput(int out, bool mute)                    { return false; }
    bool TotalMix::SetMuteInput(int in, bool mute)                      { return false; }
    bool TotalMix::SetMutePlayback(int pb, bool mute)                   { return false; }

// Miscelánea ---------------------------------------------------------------
    bool TotalMix::SetSnapshot(int index)                               { return false; }
    bool TotalMix::SetInputThreshold(int in, float threshold)           { return false; }

// Privados - Envío OSC -----------------------------------------------------
    bool TotalMix::SendVolume(Bus bus, int out, int channel, float dB)  { return false; }
    bool TotalMix::SendMute(Bus bus, int channel, bool mute)            { return false; }
    bool TotalMix::SendPacket()                                         { return false; }

// Bancos -------------------------------------------------------------------
    void TotalMix::BuildBankMaps()                                      { return;   }
    int TotalMix::BankStartFor(int channel) const                       { return 0; }

// Utilidades ---------------------------------------------------------------
    float TotalMix::dBtoFader(float dB)                                 { return 0; }
    float TotalMix::PctTodB(float pct)                                  { return 0; }

// Métodos OSC --------------------------------------------------------------
    void TotalMix::OscReset()                                                   { return; }
    int TotalMix::OscFreeSpace() const                                          { return 0; }
    bool TotalMix::OscCheckTag(char expected)                                   { return true; }
    void TotalMix::OscPatchMsgSize()                                            { return; }
    int TotalMix::OscPadString(char* dest, const char* str)                     { return 0; }
    int TotalMix::OscEffectiveStringLen(const char* str) const                  { return 0; }
    bool TotalMix::OscOpenBundle(OscTimeTag tt)                                 { return true; }
    bool TotalMix::OscCloseBundle()                                             { return true; }
    bool TotalMix::OscCloseAll()                                                { return true; }
    bool TotalMix::OscWriteAddrAndTypes(const char* name, const char* types)    { return true; }
    bool TotalMix::OscWriteFloat(float val)                                     { return true; }

#endif
