target=bin/server
CC=g++

objs=$(wildcard src/*.cpp src/*.h) main.cpp

$(target):$(objs)
	$(CC) $^ -o $@ -lpthread -fpermissive -std=c++11 -g