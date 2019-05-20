rgudir := .
include sources.mk

cc = gcc
out = librgu.so
flags = -Iinclude -fPIC
libs = -lGLESv2

.PHONY: FORCE all

all: FORCE
	$(cc) -shared -o $(out) $(rgusrc) $(libs) $(flags) $(CFLAGS)
