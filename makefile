image_server:image_server.cc
	g++ image_server.cc -g -o $@ -L /usr/lib64/mysql -l mysqlclient -ljsoncpp -lpthread -lcrypto
test:test.cpp
	g++ test.cpp -g -o $@ -L /usr/lib64/mysql -l mysqlclient -ljsoncpp
