LIBPATH=/opt/rootfs-s5p4418/lib/arm-linux-gnueabihf
USERLIBPATH=/opt/rootfs-s5p4418/usr/lib/arm-linux-gnueabihf

SYSROOTPATH=/home/dig/nanoPiFire2A/tools/4.9.3/arm-cortexa9-linux-gnueabihf/sys-root/
SYSROOTLIB=$(SYSROOTPATH)/lib/arm-linux-gnueabihf
SYSROOTUSERLIB=$(SYSROOTPATH)/lib/arm-linux-gnueabihf

PROGNAME=camera

bin_PROGRAMS=$(PROGNAME)
camera_SOURCES= main.c videoThread.c audioTransmitThread.c ringThread.c \
 audioReceiveThread.c updateThread.c connections.c i2cThread.c mcp23017.c i2c_funcs.c io.c timerThread.c keys.c testThread.c  


camera_LDADD = -lpthread  -lgstreamer-1.0  -lglib-2.0 -lgobject-2.0 -lssl -lcrypto
camera_CPPFLAGS= -std=c99 -I$(SYSROOTPATH)usr/include/gstreamer-1.0 -I$(SYSROOTPATH)usr/include/glib-2.0 -I$(SYSROOTPATH)usr/lib/glib-2.0/include



 
