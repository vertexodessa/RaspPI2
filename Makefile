
CXXFLAGS = -g -std=c++0x -O3
LIBS = -lrt

all: main
	sudo chown root ./main
	sudo chmod u+s ./main

clean:
	rm *.o main

main: main.cc main.h \
	    pi_gpio.o pi_gpio.h \
	    pi_utils.o pi_utils.h
				g++ $(CXXFLAGS) $(LIBS) main.cc -o main pi_gpio.o pi_utils.o

