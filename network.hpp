//
//  network.hpp
//  basic_sound
//
//  Created by Jack Kilgore on 3/8/22.
//  Copyright © 2022 Force Dimension. All rights reserved.
//

#ifndef network_hpp
#define network_hpp

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <array>


#include <oscpp/client.hpp>
#include <oscpp/server.hpp>
#include <oscpp/print.hpp>


#include "chai3d.h"


using namespace chai3d;

class SocketUDP {
public:
    SocketUDP() {}
    
    void open_client(unsigned int port);
    
    void open_server(unsigned int port);
    
    void close();
    
    void send(const void* buffer, size_t size);
    
    int recieve(const void* buffer, size_t size);
    
private:
    struct sockaddr_in      m_servaddr, m_cliaddr;
    int m_sockfd;
    static const unsigned int MAX_BYTES = 512;
};

// Needs to be run in a different thread.
class ServerOSC {
public:
    ServerOSC() {};
    
    void open(unsigned int port);
    
    void close();
    
    void recv_vec3d(const std::string address);
    
    cVector3d get_vec3() { return out_vec3; }
    
protected:
    size_t recv_3d_float_packet(const std::string address, const OSCPP::Server::Packet& packet);
private:
    SocketUDP sock;
    static const unsigned int MAX_BYTES = 512;
    std::array<char,MAX_BYTES> out_buffer;
    cVector3d out_vec3;
    std::mutex mtx;
    
};



class ClientOSC {
public:
    ClientOSC() {};
    
    void open(unsigned int port);
    
    void close();
    
    void send_vec3d(std::string address, cVector3d coord);
    
protected:
    size_t make_3d_float_packet(std::string address, cVector3d coord);
private:
    SocketUDP sock;
    static const unsigned int MAX_BYTES = 512;
    std::array<char,MAX_BYTES> send_buffer;
    
};

#endif /* network_hpp */
