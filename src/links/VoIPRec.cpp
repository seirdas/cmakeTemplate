#include "links/VoIPRec.hpp"


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
