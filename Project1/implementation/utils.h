#ifndef UTILS_H_   /* Include guard */
#define UTILS_H_
#include <string>
#include <vector>
#include "HttpProtocol.h"

char* readFileToCharArray(char* path);
int getFileLength(char* filename);
HttpRequest getRequest(char* buffer);
string getRequestError(HttpRequest r);
char* buildErrorResponseContent(string error);
char* buildHeader(ContentType t, int length, HttpResponseStatus status);
bool fileExists(string filename);

#endif // UTILS_H_
