INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O3 -Wall -Werror -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= seqgen3.c 

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

PYTHON_REQUIRED_MODULES=requirements.txt

PRODUCT=seqgen3

build: $(PRODUCT).o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(PRODUCT) $(PRODUCT).o -lpthread -lrt
	-rm -f *.o *.d

all: install_python_requirements run plot_results

clean:
	-rm -f *.o *.d
	-rm -f $(PRODUCT)
	-rm *.png


run: build
	sudo ./$(PRODUCT)

plot_results:
	python plot_results.py

.c.o:
	$(CC) $(CFLAGS) -c $<

install_python_requirements:
	sudo apt-get install libatlas-base-dev -y
	pip install -r $(PYTHON_REQUIRED_MODULES)