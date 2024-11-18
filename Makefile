ALL : server

server : mofe.cpp
	g++ -std=c++17 mofe.cpp ./buffer/*.cpp \
	./connectpool/*.cpp ./epoller/*.cpp \
	./heaptimer/*.cpp ./http/*.cpp \
	./threadpool/*.cpp ./webserver/*.cpp \
	-o server -lmysqlclient -lpthread -Wall -g

clean :
	-rm -rf server

.PHONY : clean ALL