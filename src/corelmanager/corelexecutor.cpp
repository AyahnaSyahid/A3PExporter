#include "incl/corelexecutor.h"
#include "combaseapi.h"
#include <QAxObject>

CorelExecutor::CorelExecutor()
 :  _ax(nullptr), QObject() {}
 
CorelExecutor::~CorelExecutor() {
    if(_ax)
        _ax.deleteLater();
}

QAxObject* initialize() {
    if(_ax) {
        return _ax;
    }
    CoInitializeEx(0, COINIT_MULTITHREADED);
    _ax = new QAxObject();
    return _ax;
}

