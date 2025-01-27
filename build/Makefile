CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = webserver
OBJS = ../log/log.cpp ../pool/*.cpp ../timer/heaptimer.cpp \
       ../http/*.cpp ../server/*.cpp \
       ../buffer/buffer.cpp ../main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




