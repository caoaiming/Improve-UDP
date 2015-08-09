# The makefile for .o files.

all : $(OBJ)

$(COM)/%.o : $(SRC)/%.c
	$(CC) -c $< -o $@ -I$(HEAD) -fPIC

.PHONY : clean
clean:
	-@$(RM) log.o mainServ.o send.o udp_rtt.o wrapper.o recv.o \
		    threadpool.o unpack.o crc32.o API.o daemon_init.o
