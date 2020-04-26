#include "macdockiconhandler.h"

#include <QImageWriter>
#include <QMenu>
#include <QTemporaryFile>
#include <QWidget>

#undef slots
#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>
#include <objc/runtime.h>

#if QT_VERSION < 0x050000
extern void qt_mac_set_dock_menu(QMenu *);
#endif

static MacDockIconHandler *s_instance = NULL;

bool dockClickHandler(id self,SEL _cmd,...) {
    Q_UNUSED(self)
    Q_UNUSED(_cmd)

    s_instance->handleDockIconClickEvent();

    // Return NO (false) to suppress the default OS X actions
    return false;
}

void setupDockClickHandler() {
    Class delClass = (Class)[[[NSApplication sharedApplication] delegate] class];
    SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
    if (class_getInstanceMethod(delClass, shouldHandle))
        class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:");
    else
        class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:");
}


MacDockIconHandler::MacDockIconHandler() : QObject()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    setupDockClickHandler();
    this->m_dummyWidget = new QWidget();
    this->m_dockMenu = new QMenu(this->m_dummyWidget);
    this->setMainWindow(NULL);
#if QT_VERSION < 0x050000
    qt_mac_set_dock_menu(this->m_dockMenu);
#elif QT_VERSION >= 0x050200
    this->m_dockMenu->setAsDockMenu();
#endif
    [pool release];
}

void MacDockIconHandler::setMainWindow(QMainWindow *window) {
    this->mainWindow = window;
}

MacDockIconHandler::~MacDockIconHandler()
{
    delete this->m_dummyWidget;
    this->setMainWindow(NULL);
}

QMenu *MacDockIconHandler::dockMenu()
{
    return this->m_dockMenu;
}

void MacDockIconHandler::setIcon(const QIcon &icon)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSImage *image = nil;
    if (icon.isNull())
        image = [[NSImage imageNamed:@"NSApplicationIcon"] retain];
    else {
        // generate NSImage from QIcon and use this as dock icon.
        QSize size = icon.actualSize(QSize(128, 128));
        QPixmap pixmap = icon.pixmap(size);

        // write temp file hack (could also be done through QIODevice [memory])
        QTemporaryFile notificationIconFile;
        if (!pixmap.isNull() && notificationIconFile.open()) {
            QImageWriter writer(&notificationIconFile, "PNG");
            if (writer.write(pixmap.toImage())) {
                const char *cString = notificationIconFile.fileName().toUtf8().data();
                NSString *macString = [NSString stringWithCString:cString encoding:NSUTF8StringEncoding];
                image =  [[NSImage alloc] initWithContentsOfFile:macString];
            }
        }

        if(!image) {
            // if testnet image could not be created, load std. app icon
            image = [[NSImage imageNamed:@"NSApplicationIcon"] retain];
        }
    }

    [NSApp setApplicationIconImage:image];
    [image release];
    [pool release];
}

MacDockIconHandler *MacDockIconHandler::instance()
{
    if (!s_instance)
        s_instance = new MacDockIconHandler();
    return s_instance;
}

void MacDockIconHandler::cleanup()
{
    delete s_instance;
}

void MacDockIconHandler::handleDockIconClickEvent()
{
    if (this->mainWindow)
    {
        this->mainWindow->activateWindow();
        this->mainWindow->show();
    }

    emit this->dockIconClicked();
}