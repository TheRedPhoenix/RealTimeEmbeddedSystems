INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc
DO_NOT_OPTIMIZE=-O0

SOURCE_FILE=posix_clock.c
PYTHON_REQUIRED_MODULES=requirements.txt
PYTHON_PLOT_SCRIPT=csvplot.py

# Prints all compilation warnings and considers them errors
WARNING_FLAGS=-Wall -Werror
# extension of the log files where the output of the program is saved
OUTPUT_FILE_EXTENSION=log
CDEFS= 
CFLAGS= $(DO_NOT_OPTIMIZE) $(WARNING_FLAGS) -g $(INCLUDE_DIRS) $(CDEFS)
# Libraries to link
LIBS= -lpthread -lrt

# Executables
PRODUCT= posix_clock_realtime posix_clock_monotonic posix_clock_realtime_coarse posix_clock_monotonic_coarse posix_clock_monotonic_raw

# Cleans, compiles and runs all executables, by also storing the outcome in a $(OUTPUT_FILE_EXTENSION) file
lifecycle: clean build_all clean_objects_post run install_python_requirements plot

# the default target: compiles all the executables
build_all: ${PRODUCT}

# plots the data collected in the csv files
install_python_requirements:
	sudo apt-get install libatlas-base-dev -y
	pip install -r $(PYTHON_REQUIRED_MODULES)

# produces comparable plots to analyse the execution outcome
plot:
	python $(PYTHON_PLOT_SCRIPT)

# Runs all products 
run: run_monotonic run_realtime run_monotonic_coarse run_realtime_coarse run_monotonic_raw

# Cleans objects before compilation
clean_objects_pre:
	-rm -f *.o *.d

# Cleans objects after compilation
clean_objects_post:
	-rm -f *.o *.d

# Cleans executables
clean_executables:
	-rm -f ${PRODUCT} ${GARBAGE}

# Cleans txt and csv files generated in execution
clean_outcomes:
	-rm -f *.csv *.png *.$(OUTPUT_FILE_EXTENSION)

# Invokes all the previously defined cleaning targets
clean: clean_objects_pre clean_executables clean_outcomes
	-rm -f *.NEW *~ 

# Creates executable posix_clock_realtime
posix_clock_realtime: posix_clock_realtime.o
	$(CC) $(LDFLAGS) $(CFLAGS) -DUSE_CLOCK_REALTIME -o $@ posix_clock_realtime.o $(LIBS)

# Creates executable posix_clock_monotonic
posix_clock_monotonic: posix_clock_monotonic.o
	$(CC) $(LDFLAGS) $(CFLAGS) -DUSE_CLOCK_MONOTONIC -o $@ posix_clock_monotonic.o $(LIBS)

# Creates executable posix_clock_realtime_coarse
posix_clock_realtime_coarse: posix_clock_realtime_coarse.o
	$(CC) $(LDFLAGS) $(CFLAGS) -DUSE_CLOCK_REALTIME_COARSE -o $@ posix_clock_realtime_coarse.o $(LIBS)

# Creates executable posix_clock_monotonic_coarse
posix_clock_monotonic_coarse: posix_clock_monotonic_coarse.o
	$(CC) $(LDFLAGS) $(CFLAGS) -DUSE_CLOCK_MONOTONIC_COARSE -o $@ posix_clock_monotonic_coarse.o $(LIBS)

# Creates executable posix_clock_monotonic_raw
posix_clock_monotonic_raw: posix_clock_monotonic_raw.o
	$(CC) $(LDFLAGS) $(CFLAGS) -DUSE_CLOCK_MONOTONIC_RAW -o $@ posix_clock_monotonic_raw.o $(LIBS)

# Compiles object posix_clock_realtime.o
posix_clock_realtime.o:
	$(CC) -MD  $(CFLAGS) -DUSE_CLOCK_REALTIME -o$@ -c $(SOURCE_FILE)

# Compiles object posix_clock_monotonic.o
posix_clock_monotonic.o:
	$(CC) -MD  $(CFLAGS) -DUSE_CLOCK_MONOTONIC -o$@ -c $(SOURCE_FILE)

# Compiles object posix_clock_realtime_coarse.o
posix_clock_realtime_coarse.o:
	$(CC) -MD  $(CFLAGS) -DUSE_CLOCK_REALTIME_COARSE -o$@ -c $(SOURCE_FILE)

# Compiles object posix_clock_monotonic_coarse.o
posix_clock_monotonic_coarse.o:
	$(CC) -MD  $(CFLAGS) -DUSE_CLOCK_MONOTONIC_COARSE -o$@ -c $(SOURCE_FILE)

# Compiles object posix_clock_monotonic_raw.o
posix_clock_monotonic_raw.o:
	$(CC) -MD  $(CFLAGS) -DUSE_CLOCK_MONOTONIC_RAW -o$@ -c $(SOURCE_FILE)

# Runs all products 
run: run_monotonic run_realtime run_monotonic_coarse run_realtime_coarse run_monotonic_raw

# Run posix_clock_monotonic and store outcome to posix_clock_monotonic.*
run_monotonic: 
	sudo ./posix_clock_monotonic > posix_clock_monotonic.$(OUTPUT_FILE_EXTENSION)

# Run posix_clock_realtime and store outcome to posix_clock_realtime.*
run_realtime: 
	sudo ./posix_clock_realtime > posix_clock_realtime.$(OUTPUT_FILE_EXTENSION)

# Run posix_clock_monotonic_coarse and store outcome to posix_clock_monotonic_coarse.*
run_monotonic_coarse: 
	sudo ./posix_clock_monotonic_coarse > posix_clock_monotonic_coarse.$(OUTPUT_FILE_EXTENSION)

# Run posix_clock_realtime_coarse and store outcome to posix_clock_realtime_coarse.*
run_realtime_coarse: 
	sudo ./posix_clock_realtime_coarse > posix_clock_realtime_coarse.$(OUTPUT_FILE_EXTENSION)

# Run posix_clock_monotonic_raw and store outcome to posix_clock_monotonic_raw.*
run_monotonic_raw: 
	sudo ./posix_clock_monotonic_raw > posix_clock_monotonic_raw.$(OUTPUT_FILE_EXTENSION)
