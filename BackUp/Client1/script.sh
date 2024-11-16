#!/bin/bash
gcc  -g client1.c  -o client1 -lssl -lcrypto 
./client1