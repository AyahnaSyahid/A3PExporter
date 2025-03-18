from PyQt5.QAxContainer import QAxObject
from PyQt5.QtCore import QSettings, QTimer
from PyQt5.QtWidgets import QApplication

if __name__ == "__main__":
    st = QSettings(r"HKEY_CLASSES_ROOT\CorelDRAW.App", QSettings.NativeFormat)
    print(st.value("CLSID/."), "== None")
    st = QSettings(r"HKEY_CLASSES_ROOT\CorelDRAW.Application", QSettings.NativeFormat)
    print(st.value("CLSID/."), "!= None (DFT)")
    st = QSettings(r"HKEY_CLASSES_ROOT\CorelDRAW.Application.17", QSettings.NativeFormat)
    print(st.value("CLSID/."), "!= None (17)")
    st = QSettings(r"HKEY_CLASSES_ROOT\CorelDRAW.Application.22", QSettings.NativeFormat)
    print(st.value("CLSID/."), "!= None (22)")
    app = QApplication([])
    ax = QAxObject()
    ax.setControl(st.value("CLSID/."))
    print(ax.property("VersionMajor"))
    print("Control :", ax.control())
    QTimer.singleShot(0, app.quit)
    app.exec()