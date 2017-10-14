#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <regex.h>
#include <algorithm>

#include "Packet.h"
using namespace std;

void printErrorAndExit(string msg) {
	printf("\n!!!Error: %s.\n\n", msg.c_str());
	exit(1);
}

void resetBuffer(char *buffer) {
	bzero((char*) buffer, sizeof(char) * PACKET_SIZE);
}

string getStringFromBuffer(char buffer[]) {
	int stringLength = strlen(buffer);
	char text[stringLength + 1];
	bzero(text, sizeof(char) * (stringLength + 1));
	text[stringLength] = '\0';
	strncpy(text, buffer, stringLength);
	string ret(text);
	ret = text;
	return ret;
}

unsigned long getFileLength(char* filename) {
	unsigned long length;
	FILE * f = fopen(filename, "r");

	fseek(f, 0L, SEEK_END);
	length = (unsigned long) ftell(f);
	fseek(f, 0L, SEEK_SET);
	fclose(f);
	return length;
}

char * readFileToCharArray(char* filename) {
	size_t r = 0;
	char *buffer = NULL;
	size_t size = 0;
	FILE *fp = fopen(filename, "r");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	buffer = (char*) malloc((size) * sizeof(*buffer));
	r = fread(buffer, size, 1, fp);
	if (r == 0) {
		printErrorAndExit("reading 0 size file\n");
	}
	return buffer;
}

void writeCharArrayToFile(char* array, char* filename, long file_length) {
	FILE* f = fopen(filename, "wb");
	if (f == NULL) {
		printErrorAndExit("ERROR with opening file");
	}
	fwrite(array, sizeof(char), file_length, f);
	fclose(f);
}

bool fileExists(string filename) {
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

unsigned int getMax(unsigned int a, unsigned int b) {
	return a > b ? a : b;
}

