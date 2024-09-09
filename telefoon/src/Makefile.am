LIBPATH=/opt/rootfs-s5p4418/lib/arm-linux-gnueabihf
USERLIBPATH=/opt/rootfs-s5p4418/usr/lib/arm-linux-gnueabihf

SYSROOTPATH=/home/dig/nanoPiFire2A/tools/4.9.3/arm-cortexa9-linux-gnueabihf/sys-root/
SYSROOTLIB=$(SYSROOTPATH)/lib/arm-linux-gnueabihf
SYSROOTUSERLIB=$(SYSROOTPATH)/lib/arm-linux-gnueabihf

PROGNAME=telefoon

bin_PROGRAMS=$(PROGNAME)
telefoon_SOURCES= main.c videoThread.c audioTransmitThread.c audioReceiveThread.c connections.c updateThread.c io.c testThread.c timerThread.c ringThread.c keys.c
telefoon_LDADD = -lpthread  -lgstreamer-1.0  -lglib-2.0 -lgobject-2.0
telefoon_CPPFLAGS=-I$(SYSROOTPATH)usr/include/gstreamer-1.0 -I$(SYSROOTPATH)usr/include/glib-2.0 -I$(SYSROOTPATH)usr/lib/glib-2.0/include



 
