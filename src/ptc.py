import zmq
import ptc_pb2 # this gets created by the build.sh script

class PTC:
    def __init__(self, server='127.0.0.1', port=3345):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect(f"tcp://{server}:{port}")

    def send_command(self, req, rep):
        cmd = ptc_pb2.Command()
        cmd.cmd.Pack(req)
        self.socket.send(cmd.SerializeToString())
        rep.ParseFromString(self.socket.recv())

    def peek(self, addr):
        req = ptc_pb2.Peek()
        req.addr = addr
        rep = ptc_pb2.RegValue()
        self.send_command(req, rep)
        return rep.value

    def poke(self, addr, value):
        req = ptc_pb2.Poke()
        req.addr = addr
        req.value = value
        rep = ptc_pb2.RegValue()
        self.send_command(req, rep)
        return rep.value