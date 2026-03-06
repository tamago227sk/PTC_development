'''PTC client : a simple command line utility to send peek/poke/ping commands to the PTC server for now.'''
# This program expects eomthign like "python3 ptc_client.py peek 0x800201FC" or
# python ptc_client.py -s 192.168.1.10 peek 0x800201FC

from ptc import PTC
import argparse

# parsing the argument from command line
parser = argparse.ArgumentParser()
parser.add_argument("cmd")                                  # the first argument is the command, which can be "peek" or "poke" (or other in the future)
parser.add_argument("args", nargs="*")                      # the rest of the arguments are the arguments for the command (can be 1 for peek (the address) or 2 for poke (the address and the value) etc)
parser.add_argument("-s","--server", default="127.0.0.1")   # the server address, default to 127.0.0.1 (NEEDS TO BE CHANGED)
args = parser.parse_args()                                  # fill the structuer of args.cmd, args.args, args.server

# construct PTC object 
ptc = PTC(args.server)


# if the command is peek, then we expect 1 argument, which is the address to peek
if args.cmd == "peek":

    # if the number of arguments is not 1, then print the usage and exit
    if len(args.args) != 1:
        print("Usage: peek <addr>")
        exit(1)

    val = ptc.peek(int(args.args[0],16))
    print(hex(val))


# if the command is poke, then we expect 2 arguments, which are the address to poke and the value to poke
elif args.cmd == "poke":

    # if the number of arguments is not 2, then print the usage and exit
    if len(args.args) != 2:
        print("Usage: poke <addr> <val>")
        exit(1)

    val = ptc.poke(int(args.args[0],16), int(args.args[1],16))
    print(hex(val))

# if the command is ping, then we expect no arguments, and we just want to check if the PTC server is alive
elif args.cmd == "ping":

    # if the number of arguments is not 0, then print the usage and exit
    if len(args.args) != 0:
        print("Usage: ping")
        exit(1)

    success = ptc.ping()
    print("PTC alive" if success else "PTC not responding")