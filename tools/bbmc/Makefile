INSTALL= ../bin

LDLIBS = -L/usr/local/Cellar/libftdi/1.4/lib -lftdi1

TARGET = bbmc

CFLAGS = -Wall -Wextra #-Werror

all: $(TARGET)

clean:
	-rm -f $(TARGET)

install: all $(INSTALL)
	cp $(TARGET) $(INSTALL)

$(INSTALL):
	mkdir $(INSTALL)

