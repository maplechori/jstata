CC=gcc
CFLAGS=-c -fPIC -g
LDFLAGS=`pkg-config --cflags --libs jansson`
SOURCES=stataread.c statawrite.c stata.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=libstata.so

all: $(SOURCES) $(EXECUTABLE) demo

$(EXECUTABLE): $(OBJECTS)
	@echo 'Building target: $@. First dep: $<'
	${CC} -shared -o $(EXECUTABLE) $(OBJECTS) $(LDFLAGS) -fPIC

.c.o:
		@echo 'Building target: $@. First dep: $<'
		$(CC) $(CFLAGS) $< -o $@

demo:
	${CC} -o  demo demo.c -g `pkg-config --cflags --libs jansson` ./libstata.so

clean:
		rm -rf *.o 
		rm -rf demo

