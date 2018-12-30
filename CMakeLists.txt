cmake_minimum_required(VERSION 3.10)
project(forwarder)

set(CMAKE_CXX_STANDARD 11)

add_executable(forwarder main.cpp utils.hpp utils.cpp server.hpp server.cpp connection.hpp connection.cpp DNSResolver.cpp DNSResolver.h SOCK5ConnectionMaker.cpp SOCK5ConnectionMaker.h)