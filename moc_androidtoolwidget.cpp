/****************************************************************************
** Meta object code from reading C++ file 'androidtoolwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "androidtoolwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'androidtoolwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AndroidToolWidget_t {
    QByteArrayData data[14];
    char stringdata0[259];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AndroidToolWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AndroidToolWidget_t qt_meta_stringdata_AndroidToolWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "AndroidToolWidget"
QT_MOC_LITERAL(1, 18, 23), // "on_detectButton_clicked"
QT_MOC_LITERAL(2, 42, 0), // ""
QT_MOC_LITERAL(3, 43, 30), // "on_runAdbDevicesButton_clicked"
QT_MOC_LITERAL(4, 74, 30), // "processReadyReadStandardOutput"
QT_MOC_LITERAL(5, 105, 29), // "processReadyReadStandardError"
QT_MOC_LITERAL(6, 135, 15), // "processFinished"
QT_MOC_LITERAL(7, 151, 8), // "exitCode"
QT_MOC_LITERAL(8, 160, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(9, 181, 10), // "exitStatus"
QT_MOC_LITERAL(10, 192, 20), // "processErrorOccurred"
QT_MOC_LITERAL(11, 213, 22), // "QProcess::ProcessError"
QT_MOC_LITERAL(12, 236, 5), // "error"
QT_MOC_LITERAL(13, 242, 16) // "checkDeviceState"

    },
    "AndroidToolWidget\0on_detectButton_clicked\0"
    "\0on_runAdbDevicesButton_clicked\0"
    "processReadyReadStandardOutput\0"
    "processReadyReadStandardError\0"
    "processFinished\0exitCode\0QProcess::ExitStatus\0"
    "exitStatus\0processErrorOccurred\0"
    "QProcess::ProcessError\0error\0"
    "checkDeviceState"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AndroidToolWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x08 /* Private */,
       3,    0,   50,    2, 0x08 /* Private */,
       4,    0,   51,    2, 0x08 /* Private */,
       5,    0,   52,    2, 0x08 /* Private */,
       6,    2,   53,    2, 0x08 /* Private */,
      10,    1,   58,    2, 0x08 /* Private */,
      13,    0,   61,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 8,    7,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,

       0        // eod
};

void AndroidToolWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AndroidToolWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_detectButton_clicked(); break;
        case 1: _t->on_runAdbDevicesButton_clicked(); break;
        case 2: _t->processReadyReadStandardOutput(); break;
        case 3: _t->processReadyReadStandardError(); break;
        case 4: _t->processFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 5: _t->processErrorOccurred((*reinterpret_cast< QProcess::ProcessError(*)>(_a[1]))); break;
        case 6: _t->checkDeviceState(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AndroidToolWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_AndroidToolWidget.data,
    qt_meta_data_AndroidToolWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AndroidToolWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AndroidToolWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AndroidToolWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int AndroidToolWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
