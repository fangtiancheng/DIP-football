#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>
#include <QMainWindow>
#include <QImage>
#include <QDialogButtonBox>
#if defined(QT_PRINTSUPPORT_LIB)
#  include <QtPrintSupport/qtprintsupportglobal.h>

#  if QT_CONFIG(printer)
#    include <QPrinter>
#  endif
#endif

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE

//! [0]
class ImageViewer : public QDialog
{
    Q_OBJECT

public:
    ImageViewer(const QString&);
    bool loadFile(const QString &);

private slots:
    void open();
    void saveAs();
    void print();
    void copy();
    void paste();
    void zoomIn();
    void zoomOut();
    void normalSize();
//    void fitToWindow();
    void about();

private:
    void createActions();
    void createMenus();
//    void updateActions();
    bool saveFile(const QString &fileName);
    void setImage(const QImage &newImage);
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QImage image;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QDialogButtonBox* buttonBox;
    double scaleFactor = 1;

#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printer)
    QPrinter printer;
#endif

    QAction *saveAsAct;
    QAction *printAct;
    QAction *copyAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;
};
//! [0]



#endif // IMAGEVIEWER_H
