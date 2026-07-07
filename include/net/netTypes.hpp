#pragma once

#include <string>
#include <vector>

// Datos del paquete recibidos para implementar en cola centralizada de NetMgr
struct NetPacket {
    std::string         socket_name;        ///< Nombre del socket
    unsigned short      port = 0;           ///< Puerto del socket
    std::vector<char>   data_rcv;           ///< Datos recibidos
};