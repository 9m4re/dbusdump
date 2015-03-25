CFLAGS = -g3 -O2 -Wall -Wunused
DBUS_FLAGS = $(shell pkg-config --cflags --libs dbus-1)
GIO_FLAGS := $(shell pkg-config --cflags --libs 'glib-2.0 >= 2.26' gio-2.0 gio-unix-2.0)
PCAP_FLAGS := $(shell pcap-config --cflags pcap-config --libs)
DESTDIR =
PREFIX = /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
DATADIR = $(DESTDIR)$(PREFIX)/share
MAN1DIR = $(DATADIR)/man/man1

BINARIES = \
	dbusdump \
	$(NULL)

all: $(BINARIES) 

DBUSDUMP_SOURCES = src/dbusdump.c src/dbus_pcap.c src/dbus_svc_info.c src/sysfs.c
DBUSDUMP_HEADERS = src/version.h src/dbus_pcap.h src/dbus_svc_info.h src/dbus_hf_ext.h src/sysfs.h

dbusdump: $(DBUSDUMP_SOURCES) $(DBUSDUMP_HEADERS)
#	@mkdir -p dist/build
	$(CC)  $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) \
		-o $@ $(DBUSDUMP_SOURCES) \
		$(GIO_FLAGS) $(PCAP_FLAGS)

memcheck: 
	G_SLICE=always-malloc valgrind --suppressions=test/glib.supp --leak-check=full --show-reachable=yes --log-file=test/vgdump ./dbusdump  --system -v /tmp/abc

clean:
	rm -f $(BINARIES) 
#	if test -d ./$(TARBALL_DIR); then rm -r ./$(TARBALL_DIR); fi
#	rm -f ./$(TARBALL)

