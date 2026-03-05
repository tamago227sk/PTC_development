from ptc import PTC
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("cmd")
parser.add_argument("args", nargs="*")
parser.add_argument("-s","--server", default="127.0.0.1")
args = parser.parse_args()

ptc = PTC(args.server)

if args.cmd == "peek":
    val = ptc.peek(int(args.args[0],16))
    print(hex(val))

elif args.cmd == "poke":
    val = ptc.poke(int(args.args[0],16), int(args.args[1],16))
    print(hex(val))