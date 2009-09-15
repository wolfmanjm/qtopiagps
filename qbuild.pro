TEMPLATE=app
TARGET = qtgps
# main.cpp uses the QTOPIA_ADD_APPLICATION/QTOPIA_MAIN macros
CONFIG+=qtopia

# Despite using the QTOPIA_MAIN macro, do not build this app as a
# quicklaunch plugin unless -force-quicklaunch was passed to configure
#CONFIG+=no_quicklaunch
# Do not build this app into a singleexec binary
#CONFIG+=no_singleexec

# Specify the languages that make lupdate should produce .ts files for
AVAILABLE_LANGUAGES=en_US
# Specify the langauges we want to install translations for
LANGUAGES=$$AVAILABLE_LANGUAGES

DEPENDPATH += .
INCLUDEPATH += .


# Input
HEADERS += gps.h libgpsmm.h qtgps.h skyView.h
FORMS += qtgps.ui
SOURCES += main.cpp qtgps.cpp skyView.cpp
LIBS += -lgps # -lpthread -lnsl -lm
#depends(gps)


# Install the launcher item. The metadata comes from the .desktop file
# and the path here.
desktop.files=qtgps.desktop
desktop.path=/apps/Applications
desktop.trtarget=qtgps-nct
desktop.hint=nct desktop
INSTALLS+=desktop

# Install some pictures.
#pics.files=pics/*
#pics.path=/pics/example
#pics.hint=pics
#INSTALLS+=pics

# Install help files
#help.source=help
#help.files=qtgps.html
#help.hint=help
#INSTALLS+=help

# SXE information
target.hint=sxe
target.domain=trusted

# Package information (used for make packages)
pkg.name=qtgps
pkg.desc=GPS Test Application
pkg.version=1.0.1
pkg.maintainer=Jim Morris (blog.wolfman.com)
pkg.license=LGPL
