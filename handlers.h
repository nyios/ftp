#pragma once
#define FUNC(name) void handle_ ## name (char* argument, int client_socket)

#define NUM_COMMANDS 8

extern void(*func_ptrs [NUM_COMMANDS])(char*, int);
extern char* commands [NUM_COMMANDS];

FUNC(user);
FUNC(password);
FUNC(passive);
FUNC(list);
FUNC(retrieve);
FUNC(quit);
FUNC(type);
FUNC(syst);

