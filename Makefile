all: server.exe client.exe server_processtop10d.exe server_infomemd.exe

server.exe: server.cpp
	g++ -o $@ $< 
	
client.exe: client.cpp
	g++ -o $@ $<

server_processtop10d.exe: daemon_server_processtop10d.cpp
	g++ -o $@ $< 

server_infomemd.exe: daemon_server_infomemd.cpp
	g++ -o $@ $< 

clean:
	rm server.exe client.exe server_processtop10d.exe server_infomemd.exe
	pkill -f server_processtop10d
	pkill -f server_infomemd
