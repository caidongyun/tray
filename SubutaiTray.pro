#-------------------------------------------------
#
# Project created by QtCreator 2016-02-17T12:15:31
#
#-------------------------------------------------

QT       += core gui network websockets
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4) : QT += widgets

TARGET = SubutaiTray
TEMPLATE = app

INCLUDEPATH += commons/include
INCLUDEPATH += hub/include
INCLUDEPATH += vbox/include

INCLUDEPATH += vbox/glue/include
INCLUDEPATH += vbox/sdk_includes
INCLUDEPATH += vbox/sdk_includes/xpcom
INCLUDEPATH += vbox/sdk_includes/nsprpub
INCLUDEPATH += vbox/sdk_includes/string
INCLUDEPATH += vbox/sdk_includes/ipcd
INCLUDEPATH += sapp/include

SOURCES += \
    main.cpp \
    hub/src/RestWorker.cpp \
    hub/src/DlgLogin.cpp \
    hub/src/SettingsManager.cpp \
    hub/src/DlgSettings.cpp \
    hub/src/TrayControlWindow.cpp \
    vbox/src/IVBoxManager.cpp \
    vbox/glue/src/xpcom/helpers.cpp \
    vbox/glue/src/AutoLock.cpp \
    vbox/glue/src/com.cpp \
    vbox/glue/src/ErrorInfo.cpp \
    vbox/glue/src/EventQueue.cpp \
    vbox/glue/src/initterm.cpp \
    vbox/glue/src/NativeEventQueue.cpp \
    vbox/glue/src/string.cpp \
    hub/src/SystemCallWrapper.cpp \
    hub/src/TrayWebSocketServer.cpp \
    hub/src/HubController.cpp \
    commons/src/EventLoop.cpp \
    commons/src/IFunctor.cpp \
    commons/src/ApplicationLog.cpp \
    commons/src/InternalCriticalSection.cpp \
    commons/src/MRE_Linux.cpp \
    commons/src/Commons.cpp \
    hub/src/DlgAbout.cpp \
    hub/src/DownloadFileManager.cpp \
    hub/src/ExecutableUpdater.cpp \
    hub/src/DlgGenerateSshKey.cpp

HEADERS  += \
    hub/include/RestWorker.h \
    hub/include/DlgLogin.h \
    hub/include/SettingsManager.h \
    hub/include/DlgSettings.h \
    hub/include/TrayControlWindow.h \
    vbox/include/VBoxCommons.h \
    vbox/include/VirtualBoxHdr.h \
    vbox/include/VBoxCommonsPlatform.h \
    vbox/include/IVBoxManager.h \
    vbox/include/IVirtualMachine.h \
    hub/include/SystemCallWrapper.h \
    hub/include/NotifiactionObserver.h \
    libssh2/UpdateErrors.h \
    hub/include/TrayWebSocketServer.h \
    hub/include/HubController.h \
    commons/include/ApplicationLog.h \
    commons/include/EventLoop.h \
    commons/include/EventLoopException.h \
    commons/include/EventLoopExceptionInfo.h \
    commons/include/FileWrapper.h \
    commons/include/FunctorWithoutResult.h \
    commons/include/FunctorWithResult.h \
    commons/include/IFunctor.h \
    commons/include/InternalCriticalSection.h \
    commons/include/IRunnable.h \
    commons/include/Locker.h \
    commons/include/MRE_Linux.h \
    commons/include/MRE_Wrapper.h \
    commons/include/ThreadWrapper.h \
    commons/include/Commons.h \
    hub/include/DlgAbout.h \
    hub/include/RestContainers.h \
    hub/include/DownloadFileManager.h \
    hub/include/ExecutableUpdater.h \
    commons/include/MRE_Windows.h \
    hub/include/DlgGenerateSshKey.h

FORMS    += \
    hub/forms/DlgLogin.ui \
    hub/forms/DlgSettings.ui \
    hub/forms/TrayControlWindow.ui \
    hub/forms/DlgAbout.ui \
    hub/forms/DlgGenerateSshKey.ui

RESOURCES += \
    resources/resources.qrc

GIT_VERSION = $$system(git describe)
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"
#////////////////////////////////////////////////////////////////////////////

unix:!macx {
  QMAKE_CXXFLAGS += -fshort-wchar
  DEFINES += VBOX_WITH_XPCOM_NAMESPACE_CLEANUP RT_OS_LINUX
  DEFINES += VBOX_WITH_XPCOM IN_RING3

  HEADERS +=  vbox/include/VBoxManagerLinux.h \
              vbox/include/VirtualMachineLinux.h

  SOURCES +=  vbox/src/VBoxManagerLinux.cpp \
              vbox/src/VirtualMachineLinux.cpp

  LIBS += /usr/lib/virtualbox/VBoxXPCOM.so
  LIBS += /usr/lib/virtualbox/VBoxRT.so
  QMAKE_RPATHDIR += /usr/lib/virtualbox/
  LIBS += -ldl -lpthread
  QMAKE_RPATHDIR += /opt/subutai/tray/bin
}
#////////////////////////////////////////////////////////////////////////////

macx: {
  QMAKE_CXXFLAGS += -fshort-wchar
#  seems like clang compiler can't resolve some functions presented in nsXPCOM.h
#  but it uses VBoxXPCOM.dylib
#  DEFINES += VBOX_WITH_XPCOM_NAMESPACE_CLEANUP

  DEFINES += RT_OS_DARWIN
  DEFINES += VBOX_WITH_XPCOM
  DEFINES += IN_RING3

  HEADERS +=  vbox/include/VBoxManagerLinux.h \
              vbox/include/VirtualMachineLinux.h
  SOURCES +=  vbox/src/VBoxManagerLinux.cpp \
              vbox/src/VirtualMachineLinux.cpp

  QMAKE_LFLAGS += -F /System/Library/Frameworks/CoreFoundation.framework/
  LIBS += /Applications/VirtualBox.app/Contents/MacOS/VBoxXPCOM.dylib
  LIBS += /Applications/VirtualBox.app/Contents/MacOS/VBoxRT.dylib
  LIBS += -framework CoreFoundation
  LIBS += -ldl -lpthread
  ICON = $$PWD/resources/tray_logo.icns
  QMAKE_INFO_PLIST = $$PWD/Info.plist
}
#////////////////////////////////////////////////////////////////////////////

win32: {
  DEFINES += RT_OS_WINDOWS IN_RING3
  LIBS += -L$$PWD/vbox/lib/ -lVBoxRT_64 VBoxRT_64.exp
  LIBS += Ole32.lib Rpcrt4.lib

  INCLUDEPATH += vbox/mscom/include

  SOURCES +=  vbox/mscom/lib/VirtualBox_i.c \
              vbox/src/VirtualMachineWin.cpp \
              vbox/src/VBoxManagerWin.cpp

  HEADERS +=  vbox/include/VirtualMachineWin.h \
              vbox/include/VBoxManagerWin.h
}
#////////////////////////////////////////////////////////////////////////////

DISTFILES += \
    resources/Tray_icon_set-07.png
