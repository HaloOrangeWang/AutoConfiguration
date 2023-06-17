QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

vcpkg_root = D:\Software\vcpkg-master\installed\x64-windows
INCLUDEPATH += $${vcpkg_root}\include

CONFIG(debug, debug|release){
    LIBS += -L"$${vcpkg_root}\debug\lib" boost_random-vc140-mt-gd.lib boost_filesystem-vc140-mt-gd.lib

    dll_files.files = $${vcpkg_root}\debug\bin\boost_random-vc143-mt-gd-x64-1_81.dll
    dll_files.path = $${OUT_PWD}
    COPIES += dll_files

    dll_files2.files = $${vcpkg_root}\debug\bin\boost_filesystem-vc143-mt-gd-x64-1_81.dll
    dll_files2.path = $${OUT_PWD}
    COPIES += dll_files2
}

CONFIG(release, debug|release){
    LIBS += -L"$${vcpkg_root}\lib" boost_random-vc140-mt.lib boost_filesystem-vc140-mt.lib

    dll_files.files = $${vcpkg_root}\bin\boost_random-vc143-mt-x64-1_81.dll
    dll_files.path = $${OUT_PWD}
    COPIES += dll_files

    dll_files2.files = $${vcpkg_root}\bin\boost_filesystem-vc143-mt-x64-1_81.dll
    dll_files2.path = $${OUT_PWD}
    COPIES += dll_files2
}

SOURCES += \
    answer_parser.cpp \
    cmdexec.cpp \
    constants.cpp \
    frontend/cinstallwindow.cpp \
    frontend/cmdlistview.cpp \
    frontend/cpathwindow.cpp \
    frontend/creplacewindow.cpp \
    frontend/mainwindow.cpp \
    gpt_comm.cpp \
    main.cpp \
    question.cpp

HEADERS += \
    answer_parser.h \
    cmdexec.h \
    constants.h \
    frontend/cinstallwindow.h \
    frontend/cmdlistview.h \
    frontend/cpathwindow.h \
    frontend/creplacewindow.h \
    frontend/mainwindow.h \
    funcs.h \
    gpt_comm.h \
    question.h

FORMS += \
    frontend/cinstallwindow.ui \
    frontend/cpathwindow.ui \
    frontend/creplacewindow.ui \
    frontend/mainwindow.ui



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
