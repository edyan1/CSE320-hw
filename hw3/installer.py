#!/usr/bin/env python3

# Copy files to /usr/include and /usr/lib
import os
import sys
import shutil
import signal


def signal_handler(signal, frame):
        print("Qutting now will stop the installation of criterion...")


def rootPermission():
    euid = os.geteuid()
    if euid != 0:
        print("Script not started as root. Running sudo..")
        args = ['sudo', sys.executable] + sys.argv + [os.environ]
        os.execlpe('sudo', *args)


def includeFiles():
    print("Moved over Include files.")
    include = "criterion/include/criterion"
    includeDestination = "/usr/include"
    shutil.move(include, includeDestination)


def libFile():
    print("Moved over Lib file.")
    lib = "criterion/lib/libcriterion.so"
    libDestination = "/usr/lib"
    shutil.move(lib, libDestination)


def main():
    signal.signal(signal.SIGINT, signal_handler)
    rootPermission()
    includeFiles()
    libFile()
    shutil.rmtree("criterion")
    os.remove("installer.py")


if __name__ == "__main__":
        main()
