#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <string>
#include <cstring>
#include "utils.h"
#include <regex.h>

#include "HttpProtocol.h"
using namespace std;

class Test {
public:
	void printString(string t);
};
int main(int argc, char *argv[]) {
	printf("test\n");
}

void Test::printString(string s) {
	printf("%s\n", s.c_str());
}

