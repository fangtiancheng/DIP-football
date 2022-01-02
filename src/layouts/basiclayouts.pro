QT += widgets
QT += network widgets multimedia multimediawidgets
requires(qtConfig(combobox))

HEADERS     = dialog.h \
              common.h \
              config.h \
              connection.h \
              fileDownloader.h \
              fileUploader.h \
              imageViewer.h \
              mainWidget.h \
              player/histogramwidget.h \
              player/player.h \
              player/playercontrols.h \
              player/playlistmodel.h \
              player/qmediaplaylist.h \
              player/qmediaplaylist_p.h \
              player/qplaylistfileparser_p.h \
              player/videowidget.h
SOURCES     = dialog.cpp \
              common.cpp \
              config.cpp \
              connection.cpp \
              fileDownloader.cpp \
              fileUploader.cpp \
              imageViewer.cpp \
              main.cpp \
              mainWidget.cpp \
              player/histogramwidget.cpp \
              player/player.cpp \
              player/playercontrols.cpp \
              player/playlistmodel.cpp \
              player/qmediaplaylist.cpp \
              player/qplaylistfileparser.cpp \
              player/videowidget.cpp

# install
#target.path = $$[QT_INSTALL_EXAMPLES]/widgets/layouts/basiclayouts
#INSTALLS += target

FORMS += \
    config.ui \
    fileUploader.ui

DISTFILES += \
    player/CMakeLists.txt

SUBDIRS += \
    player/player.pro
