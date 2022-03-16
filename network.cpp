//
//  network.cpp
//  basic_sound
//
//  Created by Jack Kilgore on 3/8/22.
//  Copyright © 2022 Force Dimension. All rights reserved.
//

#include "network.hpp"
#include <unistd.h>


void SocketUDP::open_client(unsigned int port) {
    // Creating socket file descriptor
   if ( (m_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
       perror("socket creation failed");
       exit(EXIT_FAILURE);
   }
  
   memset(&m_servaddr, 0, sizeof(m_servaddr));
    
    // Filling server information
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    m_servaddr.sin_addr.s_addr = INADDR_ANY;
}

void SocketUDP::open_server(unsigned int port) {
    // Creating socket file descriptor
    if ( (m_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
       
    memset(&m_servaddr, 0, sizeof(m_servaddr));
    memset(&m_cliaddr, 0, sizeof(m_cliaddr));
       
    // Filling server information
    m_servaddr.sin_family    = AF_INET; // IPv4
    m_servaddr.sin_port = htons(port);
    m_servaddr.sin_addr.s_addr = INADDR_ANY;
       
    // Bind the socket with the server address
    if ( bind(m_sockfd, (const struct sockaddr *)&m_servaddr,
            sizeof(m_servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void SocketUDP::close() {
    ::close(m_sockfd);
}


void SocketUDP::send(const void* buffer, size_t size) {
    sendto( m_sockfd, (const char *)buffer, size,
            0, (const struct sockaddr *) &m_servaddr,
            sizeof(m_servaddr)
           );
}

int SocketUDP::recieve(const void* out_buffer, size_t out_size) {
    int32_t addr_len = sizeof(m_cliaddr);
    int32_t n = recvfrom(m_sockfd, (char *)out_buffer, out_size,
    MSG_WAITALL, ( struct sockaddr *) &m_cliaddr,
    (socklen_t *)&addr_len);
    return n;
}

void ServerOSC::open(unsigned int port) {
    sock.open_server(port);
}

void ServerOSC::close() {
    sock.close();
}

void ServerOSC::recv_vec3d(const std::string address) {
    int msg_length = sock.recieve(out_buffer.data(),out_buffer.size()); // BLOCKING
    recv_3d_float_packet(address, OSCPP::Server::Packet(out_buffer.data(),msg_length));
}

size_t ServerOSC::recv_3d_float_packet(const std::string address, const OSCPP::Server::Packet& packet) {
    // Convert to message
    OSCPP::Server::Message msg(packet);

    // Get argument stream
    OSCPP::Server::ArgStream args(msg.args());
    float value = 0.0f;
    if (msg == address.c_str()) {
        value = args.float32();
    }
    
    mtx.lock();
    out_vec3.set(value,value,value);
    mtx.unlock();
    return 0;
}


void ClientOSC::open(unsigned int port) {
    sock.open_client(port);
}

void ClientOSC::close() {
    sock.close();
}

void ClientOSC::send_vec3d(std::string address, cVector3d coord) {
    // Fills send_buffer
    make_3d_float_packet(address, coord);
    sock.send(send_buffer.data(),send_buffer.size());
}

size_t ClientOSC::make_3d_float_packet(std::string address, cVector3d coord)
{
    // Construct a packet
    OSCPP::Client::Packet packet(send_buffer.data(), send_buffer.size());
    packet
        .openMessage(address.c_str(), 3)
            // Write the arguments
            .float32(coord.x())
            .float32(coord.y())
            .float32(coord.z())
        .closeMessage();
    return packet.size();
}



