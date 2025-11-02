/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.17)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.17. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[22];
    char stringdata0[286];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 15), // "onButtonClicked"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 11), // "onDebugInfo"
QT_MOC_LITERAL(4, 40, 10), // "onOpenFile"
QT_MOC_LITERAL(5, 51, 12), // "onFileOpened"
QT_MOC_LITERAL(6, 64, 8), // "fileName"
QT_MOC_LITERAL(7, 73, 12), // "onFileClosed"
QT_MOC_LITERAL(8, 86, 12), // "onAudioError"
QT_MOC_LITERAL(9, 99, 5), // "error"
QT_MOC_LITERAL(10, 105, 11), // "onPlayPause"
QT_MOC_LITERAL(11, 117, 6), // "onStop"
QT_MOC_LITERAL(12, 124, 15), // "onVolumeChanged"
QT_MOC_LITERAL(13, 140, 5), // "value"
QT_MOC_LITERAL(14, 146, 21), // "onSeekPositionChanged"
QT_MOC_LITERAL(15, 168, 19), // "onAudioStateChanged"
QT_MOC_LITERAL(16, 188, 27), // "AudioManager::PlaybackState"
QT_MOC_LITERAL(17, 216, 5), // "state"
QT_MOC_LITERAL(18, 222, 22), // "onAudioPositionChanged"
QT_MOC_LITERAL(19, 245, 8), // "position"
QT_MOC_LITERAL(20, 254, 22), // "onAudioDurationChanged"
QT_MOC_LITERAL(21, 277, 8) // "duration"

    },
    "MainWindow\0onButtonClicked\0\0onDebugInfo\0"
    "onOpenFile\0onFileOpened\0fileName\0"
    "onFileClosed\0onAudioError\0error\0"
    "onPlayPause\0onStop\0onVolumeChanged\0"
    "value\0onSeekPositionChanged\0"
    "onAudioStateChanged\0AudioManager::PlaybackState\0"
    "state\0onAudioPositionChanged\0position\0"
    "onAudioDurationChanged\0duration"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x08 /* Private */,
       3,    0,   80,    2, 0x08 /* Private */,
       4,    0,   81,    2, 0x08 /* Private */,
       5,    1,   82,    2, 0x08 /* Private */,
       7,    0,   85,    2, 0x08 /* Private */,
       8,    1,   86,    2, 0x08 /* Private */,
      10,    0,   89,    2, 0x08 /* Private */,
      11,    0,   90,    2, 0x08 /* Private */,
      12,    1,   91,    2, 0x08 /* Private */,
      14,    1,   94,    2, 0x08 /* Private */,
      15,    1,   97,    2, 0x08 /* Private */,
      18,    1,  100,    2, 0x08 /* Private */,
      20,    1,  103,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, 0x80000000 | 16,   17,
    QMetaType::Void, QMetaType::LongLong,   19,
    QMetaType::Void, QMetaType::LongLong,   21,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onButtonClicked(); break;
        case 1: _t->onDebugInfo(); break;
        case 2: _t->onOpenFile(); break;
        case 3: _t->onFileOpened((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->onFileClosed(); break;
        case 5: _t->onAudioError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onPlayPause(); break;
        case 7: _t->onStop(); break;
        case 8: _t->onVolumeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->onSeekPositionChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->onAudioStateChanged((*reinterpret_cast< AudioManager::PlaybackState(*)>(_a[1]))); break;
        case 11: _t->onAudioPositionChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 12: _t->onAudioDurationChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
