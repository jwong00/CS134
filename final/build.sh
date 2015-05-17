#!/bin/bash
gcc fmrl.c `pkg-config --cflags --libs sdl2 gl glew` -o fmrlout
