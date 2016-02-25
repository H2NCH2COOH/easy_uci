EXEC = libeasy_uci.so
CFLAGS += -Wall -Wextra -fPIC
LIBS += -luci

.PHONY: default all clean

default: $(EXEC)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(EXEC) $(OBJECTS)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -shared -Wall -Wextra $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(EXEC)
