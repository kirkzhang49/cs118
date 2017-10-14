
#ifndef HTTPPROTOCOL_H_
#define HTTPPROTOCOL_H_

#include <string>
using namespace std;

enum ContentType {
	html, gif, jpeg, unknown
};

enum HttpResponseStatus {
	OK, NotFound
};

struct HttpRequest {
	string path;
	ContentType type;
};

#endif /* HTTPPROTOCOL_H_ */
