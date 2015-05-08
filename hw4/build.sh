#!/bin/bash
gcc hw4.c `pkg-config --cflags --libs sdl2 gl glew` -o hw4out
