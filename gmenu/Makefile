#set PREFIX to gnome's location:
PREFIX = /usr

include $(PREFIX)/lib/gnomeConf.sh

#Parse out those nasty quotes.
GNOME_INCLUDEDIR_PARSED = $(subst ",,$(GNOME_INCLUDEDIR))
GNOME_LIBS_PARSED = $(subst ",,$(GNOME_LIBS))
GNOMEUI_LIBS_PARSED = $(subst ",,$(GNOMEUI_LIBS))

GTK_INCLUDE = `gtk-config --cflags`
GTK_LIB = `gtk-config --libs`
CFLAGS = $(GTK_INCLUDE) $(GNOME_INCLUDEDIR_PARSED)
LDFLAGS = $(GTK_LIB) $(GNOMEUI_LIBS_PARSED)
CC = gcc -g -Wall $(CFLAGS)
CCPLUS = c++ -Wall

OBJS = gmenu.o fileops.o iconsel.o

gmenu: $(OBJS)
	$(CCPLUS) $(OBJS) -o gmenu $(LDFLAGS)

gmenu.o : gmenu.c gmenu.h top.xpm unknown.xpm folder.xpm
	$(CC) -c gmenu.c
fileops.o : fileops.c gmenu.h iconsel.h 
	$(CC) -c fileops.c
iconsel.o : iconsel.c iconsel.h gmenu.h 
	$(CC) -c iconsel.c


install:
	install -c gmenu $(PREFIX)/bin/gmenu
	install -c gmenu.desktop $(PREFIX)/share/apps/System/gmenu.desktop
	install -c gnome-gmenu.png $(PREFIX)/share/pixmaps/gnome-gmenu.png

uninstall:
	rm -f $(PREFIX)/bin/gmenu
	rm -f $(PREFIX)/share/apps/System/gmenu.desktop
	rm -f $(PREFIX)/share/pixmaps/gnome-gmenu.png

clean:
	rm -f *.o *~ gmenu core
