MTC = mtc
MTC_CLI = mtc_cli
TARGET = $(MTC) $(MTC_CLI)

#LIBS        = `pkg-config --libs --cflags libugui`

MTC_CLI_OBJS = mtc_cli.o mtc_client.o
MTC_OBJS  = $(subst $(MTC_CLI_OBJS),,$(patsubst %.c,%.o,$(wildcard *.c)))
MTC_OBJS += $(patsubst %.c,%.o,$(wildcard sys/*.c))
MTC_OBJS += $(patsubst %.c,%.o,$(wildcard serv/*.c))

all: $(TARGET) WDK

WDK:
	@echo "Building WDK ..."
	make -C wdk

$(MTC): $(MTC_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(MTC_CLI): $(MTC_CLI_OBJS)
	@echo "Building $@ ..."
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c -Wall $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o $(TARGET) 


