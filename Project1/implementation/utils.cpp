#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <regex.h>
#include <algorithm>

#include "HttpProtocol.h"
using namespace std;

int getFileLength(char* filename) {
	int length;
	FILE * f = fopen(filename, "r");

	fseek(f, 0L, SEEK_END);
	length = (int) ftell(f);
	fseek(f, 0L, SEEK_SET);
	fclose(f);
	return length;
}
char * readFileToCharArray(char* filename) {
	char * buffer;
	int length;
	FILE * f = fopen(filename, "r");

	fseek(f, 0L, SEEK_END);
	length = (int) ftell(f);
	fseek(f, 0L, SEEK_SET);
	buffer = (char *) malloc(sizeof(char) * length);
	if (buffer) {
		fread(buffer, sizeof(char), length, f);
	}
	fclose(f);
	return buffer;
}

HttpRequest getRequest(char* source) {
	HttpRequest request;
//	regex rgx("GET /(.+\\.(.+)) HTTP");
//	smatch match;

	string path;
	string extension;

	char * regexString = (char *)"GET /(.+\\.(.+)) HTTP";
	size_t maxGroups = 3;

	regex_t regexCompiled;
	regmatch_t groupArray[maxGroups];

	if (regcomp(&regexCompiled, regexString, REG_EXTENDED)) {
		printf("Could not compile regular expression.\n");
	};

	if (regexec(&regexCompiled, source, maxGroups, groupArray, 0) == 0) {
		unsigned int g = 0;
		for (g = 1; g < maxGroups; g++) {
			if (groupArray[g].rm_so == (size_t) -1)
				break;  // No more groups

			char sourceCopy[strlen(source) + 1];
			strcpy(sourceCopy, source);
			sourceCopy[groupArray[g].rm_eo] = 0;
			char buffer[50];
			sprintf(buffer, "%s\n", sourceCopy + groupArray[g].rm_so);
			if (g == 1) {
				string str(buffer);
				path = str;
			}
			if (g == 2) {
				string str(buffer);
				extension = str;
			}
		}
	}

	regfree(&regexCompiled);

	extension.erase(remove(extension.begin(), extension.end(), '\n'), extension.end());
	path.erase(remove(path.begin(), path.end(), '\n'), path.end());

	request.path = path;
	if (extension.compare("html") == 0) {
		request.type = html;
	} else if (extension.compare("gif") == 0 || extension.compare("GIF") == 0) {
		request.type = gif;
	} else if (extension.compare("jpeg") == 0
			|| extension.compare("JPEG") == 0) {
		request.type = jpeg;
	} else {
		request.type = unknown;
	}
	return request;
}

string getRequestError(HttpRequest r) {
	string errorMsg;
	if (r.path.compare("") == 0) {
		errorMsg = "Requested file path is empty.";
	} else if (r.type == unknown) {
		errorMsg = "Requested file type is not supported";
	} else if (!fileExists(r.path)) {
		errorMsg = "Requested file path does not exist.";
	} else {
		errorMsg = "";
	}
	return errorMsg;
}

char* buildErrorResponseContent(string error) {
	string html = "";
	html.append("<html>\n");
	html.append("<head><title>404 Not Found</title></head>\n");
	html.append("<body>\n");
	html.append("<h1>404 Not Found</h1>\n");
	html.append("<p>\n");
	html.append(error);
	html.append("</p>\n");
	html.append("</body>\n");
	html.append("</html>\n");
	char *ret = new char[html.length() + 1];
	strcpy(ret, html.c_str());
	return ret;
}

char* buildHeader(ContentType t, int length, HttpResponseStatus status) {
	string header;
	switch (status) {
	case OK:
		header = "HTTP/1.1 200 OK\n";
		break;
	case NotFound:
		header = "HTTP/1.1 404 Not Found\n";
		break;
	default:
		header = "HTTP/1.1 200 OK\n";
		break;
	}

	switch (t) {
	case html:
		header.append("Content-Type: text/html; charset=utf-8\n");
		break;
	case jpeg:
		header.append("Content-Type: image/jpeg\n");
		break;
	case gif:
		header.append("Content-Type: image/gif\n");
		break;
	default:
		header.append("Content-Type: text/html; charset=utf-8\n");
		break;
	}
	header.append("Connection: close\n");
	header.append("Content-Length:");
	header.append(to_string(length) + "\n\n");

	char *ret = new char[header.length() + 1];
	strcpy(ret, header.c_str());
	return ret;
}

bool fileExists(string filename) {
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}
