# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Add-in.
# ------------------------------------------------------

TEMPLATE = lib
TARGET = ModelSimGenerator
DESTDIR = ../../executable/Plugins
QT += core xml widgets gui
CONFIG += plugin release
DEFINES += QT_DLL QT_XML_LIB MODELSIMGENERATOR_LIB QT_WIDGETS_LIB
INCLUDEPATH += ./../.. \
    ./GeneratedFiles \
    . \
    $(QTDIR)/../qttools/include 
LIBS += -L"./../../executable" \
    -lIPXACTmodels
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(ModelSimGenerator.pri)

target.path = /usr/share/kactus2/plugins
INSTALLS += target

