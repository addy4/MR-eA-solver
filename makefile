flags = -Wall -O3

all:: mrils

mrils: main.o WL_MRILS.o WL_Instance.o WL_Solution.o pcea-solution.o
	g++ -std=c++11 $(flags) main.o WL_MRILS.o pcea-solution.o WL_Instance.o WL_Solution.o -o mrils -I./include -L. -lfpmax -Wl,-rpath,.

main.o:
	g++ -std=c++11 $(flags) -c main.cpp

WL_MRILS.o:
	g++ -std=c++11 $(flags) -c WL_MRILS.cpp -I./include

WL_Instance.o:
	g++ -std=c++11 $(flags) -c WL_Instance.cpp

WL_Solution.o:
	g++ -std=c++11 $(flags) -c WL_Solution.cpp

pcea-solution.o:
	gcc -c pcea-solution.c

clean:
	rm -f *.o mrils
