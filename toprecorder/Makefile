all : toprecorder process

toprecorder : toprecorder.c
	gcc -o $@ $^

process : process.cpp
	g++ -o $@ $^ -O1 -g

clean :
	rm -rf *.o *~ toprecorder process

