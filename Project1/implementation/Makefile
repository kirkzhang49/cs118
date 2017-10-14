CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=c++0x

SERVER_OBJS =		cs118_webserver.o utils.o HttpProtocol.o

CLIENT_OBJS =		client.o

TEST_OBJS   =		test.o utils.o HttpProtocol.o

LIBS =

SERVER =	cs118_webserver

CLIENT = 	client

TEST   =	test

all:	$(SERVER) $(CLIENT) $(TEST)

$(SERVER):	$(SERVER_OBJS)
	$(CXX) -o $(SERVER) $(SERVER_OBJS) $(LIBS)

$(CLIENT):	$(CLIENT_OBJS)
	$(CXX) -o $(CLIENT) $(CLIENT_OBJS) $(LIBS)

$(TEST):	$(TEST_OBJS)
	$(CXX) -o $(TEST) $(TEST_OBJS) $(LIBS)
	
clean:
	rm -f $(SERVER_OBJS) $(SERVER) $(CLIENT_OBJS) $(CLIENT) $(TEST_OBJS) $(TEST)
