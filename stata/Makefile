CC=gcc
CFLAGS=-c
LDFLAGS=`pkg-config --cflags --libs jansson`
SOURCES=stataread.c statawrite.c stata.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=libstata

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@echo 'Building target: $@. First dep: $<'
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
		@echo 'Building target: $@. First dep: $<'
		$(CC) $(CFLAGS) $< -o $@

demo:
	${CC} -o demo demo.c `pkg-config --cflags --libs jansson`

clean:
		rm -rf *.o 
		rm -rf demo

