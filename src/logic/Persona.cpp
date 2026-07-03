#include "logic/Persona.hpp"
#include "system/SystemMgr.hpp"

// General ------------------------------------------------------------------------------

Persona::Persona() {

}

Persona::~Persona() {

}



void Persona::logInfo() {
    SYS_INFO("Persona","Not yet implemented.");
    return;
}

std::string Persona::name() {
    return name_;
}

bool Persona::isTransmiting() {
    return !TXs.empty();
}

bool Persona::isReceiving() {
    return !RXs.empty();
}