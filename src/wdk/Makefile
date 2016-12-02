
CFLAGS += -I./ -D_FILE_OFFSET_BITS=64
LDFLAGS += 
INCLUDES += -I.

WDK_OBJS  = $(patsubst %.c,%.o,$(wildcard *.c))
WDK_OBJS += ../sys/mtc_linklist.o ../mtc_client.o

LIBS = 

all: wdk-bin

wdk-bin: $(WDK_OBJS)
	@echo "Building $< ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(INCLUDES) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(image)


