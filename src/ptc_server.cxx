// assuming ZeroMQ and Protobuf libraries are available
#include <zmq.hpp>
#include <string>
#include <iostream>
#include "ptc.pb.h"
#include "ptc.h"
#include "log.h"

//main server loop (while): receive command, dispatch to PTC, send back response
int main(int argc, char **argv) {

    // Set output to line buffering for consistent logging
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    // Initialize the PTC hardware interface
    PTC ptc; 
    
    // Setup ZeroMQ context and socket
    // use port 7820 () as a placeholder; ensure this matches your client configuration
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:7820");

    glog.log("PTC Server: Started and listening on port 7820\n");

    while (true) {
        zmq::message_t request;
        
        // Wait for next request from client
        auto res = socket.recv(request, zmq::recv_flags::none);
        if (!res) continue;

        ptc::Command command;
        if (!command.ParseFromArray(request.data(), request.size())) {
            glog.log("PTC Server: Failed to parse incoming command\n");
            continue;
        }

        std::string reply_str;

        // --- Dispatch Logic (Symmetric to WIB Server) ---

        // Handle PEEK
        if (command.cmd().Is<ptc::Peek>()) {
            ptc::Peek peek_msg;
            command.cmd().UnpackTo(&peek_msg);
            
            uint32_t val = ptc.peek(peek_msg.addr());
            
            ptc::RegValue rep;
            rep.set_addr(peek_msg.addr());
            rep.set_value(val);
            rep.SerializeToString(&reply_str);
        } 

        // Handle POKE
        else if (command.cmd().Is<ptc::Poke>()) {
            ptc::Poke poke_msg;
            command.cmd().UnpackTo(&poke_msg);
            
            ptc.poke(poke_msg.addr(), poke_msg.value());

            //read back the value to confirm the poke
            uint32_t rb = ptc.peek(poke_msg.addr());
            
            if (rb != poke_msg.value()) {
                glog.log("POKE readback mismatch: wrote 0x%08x, read 0x%08x", poke_msg.value(), rb);
            }

            ptc::RegValue rep;
            rep.set_addr(poke_msg.addr());
            rep.set_value(rb);
            rep.SerializeToString(&reply_str);
        }

        // handle PING
        else if (command.cmd().Is<ptc::Ping>()) {
            bool alive = ptc.ping();
            ptc::Status status;
            status.set_success(alive);
            status.SerializeToString(&reply_str);
        }

        else {
            glog.log("PTC Server: Received unknown command type\n");
            ptc::Status status;
            status.set_success(false);
            status.SerializeToString(&reply_str);
        }

        // Send the serialized reply back to client
        zmq::message_t reply(reply_str.size());
        memcpy(reply.data(), reply_str.data(), reply_str.size());
        socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}