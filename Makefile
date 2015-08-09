# Makfile for improve UDP
#
WORKPATH = $(shell pwd)

export HEAD = $(WORKPATH)/head
export BIN  = $(WORKPATH)/bin
export COM  = $(WORKPATH)/com
export SRC  = $(WORKPATH)/src
export API  = $(WORKPATH)/api
export LIB  = $(WORKPATH)/lib

export RM = `which rm` -fr
export CC = $(shell which gcc)

export OBJ = $(COM)/log.o        \
	         $(COM)/mainServ.o   \
	         $(COM)/send.o       \
	         $(COM)/udp_rtt.o    \
	         $(COM)/wrapper.o    \
	         $(COM)/recv.o       \
			 $(COM)/threadpool.o \
			 $(COM)/unpack.o     \
			 $(COM)/crc32.o      \
			 $(COM)/API.o        \
			 $(COM)/daemon_init.o

TARGET1 = $(BIN)/udp_serv
TARGET2 = $(LIB)/libprovudp.so

all : $(TARGET1) $(TARGET2)

.PHONY : all

$(TARGET1) : 
	@cd $(COM) && make
	@$(CC) -o $(TARGET1) $(OBJ) -lpthread -g

$(TARGET2) :
	@$(CC) $(OBJ) -o $(TARGET2) -shared 
	@printf "\033[4m%70s\033[0m\n" ""
	@echo -e "    \033[34mMakefile.\033[0m"
	@printf "    \033[34m%-65s\033[0m[ \033[1;32mOK \033[0m]\n" "Complete."
	@echo -e "    \033[1;30;5m`date`\033[0m"

clean:
	-@$(RM) $(BIN)/udp_serv
	-@$(RM) $(LIB)/libprovudp.so
	@cd $(COM) && make clean
	@printf "    \033[34m%-65s\033[0m[ \033[1;32mOK \033[0m]\n" "Clean up."
	@echo -e "    \033[1;30;5m`date`\033[0m"    
