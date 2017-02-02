QT += network widgets printsupport
QT += core

SUBDIRS += source
INCLUDEPATH += DspFilter

LIBS += -L/usr/local/lib/ -lDSPFilters

HEADERS       = networkcontroller.h \
				networkgui.h \
				mainwindow.h \
				qcustomplot.h
SOURCES       = networkcontroller.cpp \
				networkgui.cpp \
				qcustomplot.cpp \
				mainwindow.cpp \
                main.cpp

# install
#target.path = $$[QT_INSTALL_EXAMPLES]/network/multicastreceiver
#INSTALLS += target

DESTDIR=bin
OBJECTS_DIR=aux_files
MOC_DIR=aux_files
