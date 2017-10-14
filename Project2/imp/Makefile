CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=c++0x

SERVER_OBJS =		server.o utils.o Packet.o Window.o settings.o

CLIENT_OBJS =		client.o utils.o Packet.o Window.o settings.o

TEST_OBJS   =		test.o utils.o Packet.o Window.o settings.o

LIBS =

SERVER =	server

CLIENT = 	client

TEST   =	test

all:	$(SERVER) $(CLIENT) $(TEST)

$(SERVER):	$(SERVER_OBJS)
	$(CXX) -o $(SERVER) $(SERVER_OBJS) $(LIBS) -lrt

$(CLIENT):	$(CLIENT_OBJS)
	$(CXX) -o $(CLIENT) $(CLIENT_OBJS) $(LIBS) -lrt

$(TEST):	$(TEST_OBJS)
	$(CXX) -o $(TEST) $(TEST_OBJS) $(LIBS) -lrt
	
clean:
	rm -f $(SERVER_OBJS) $(SERVER) $(CLIENT_OBJS) $(CLIENT) $(TEST_OBJS) $(TEST) output output.txt
