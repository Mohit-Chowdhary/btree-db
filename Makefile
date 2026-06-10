all:
	g++ -g src/main.cpp src/btree.cpp src/pager.cpp -o src/main.exe

clean:
	del src\main.exe