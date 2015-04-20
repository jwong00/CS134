#!/bin/bash
gcc hw5.c `pkg-config --cflags --libs sdl2 gl glew` -o hw5out
