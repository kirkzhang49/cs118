#ifndef UTILS_H_   /* Include guard */
#define UTILS_H_
#include <string>
#include <vector>
#include "Packet.h"

void resetBuffer(char *buffer);
void writeCharArrayToFile(char* array, char* filename, long file_length);
void printErrorAndExit(string msg);
char* readFileToCharArray(char* path);
bool fileExists(string filename);
unsigned long getFileLength(char* filename);
string getStringFromBuffer(char buffer[]);
unsigned int getMax(unsigned int a, unsigned int b);
#endif // UTILS_H_
