import qbs;

Project {
    name: "CANopen";

    Product {
        name: (project.namePrefixProtocols + "canopen");
        type: (project.protocolsAsSharedLibs ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixProtocols + "CANopen");
        cpp.includePaths: [".", "QML/"];
        cpp.defines: {
            var ret = [];
            if (project.protocolsAsSharedLibs) {
                ret.push ("QTCAN_CANOPEN_EXPORT=Q_DECL_EXPORT");
            }
            else {
                ret.push ("QTCAN_CANOPEN_EXPORT=");
            }
            return ret;
        }

        Export {
            cpp.includePaths: [".", "QML/"];
            cpp.defines: {
                var ret = [];
                if (project.protocolsAsSharedLibs) {
                    ret.push ("QTCAN_CANOPEN_EXPORT=Q_DECL_IMPORT");
                }
                else {
                    ret.push ("QTCAN_CANOPEN_EXPORT=");
                }
                return ret;
            }

            Depends { name: "cpp"; }
            Depends { name: "Qt"; submodules: ["core", "qml"]; }
        }
        Depends { name: "cpp"; }
        Depends { name: "Qt"; submodules: ["core", "qml"]; }
        Depends { name: (project.namePrefixBase + "base"); }
        Group {
            name: "C++ headers";
            files: [
                "CanOpenDefs.h",
                "CanOpenEntry.h",
                "CanOpenSubEntry.h",
                "CanOpenObjDict.h",
                "CanOpenProtocolManager.h",
                "QML/AbstractObdPreparator.h",
                "QML/QmlCanOpenArrayEntry.h",
                "QML/QmlCanOpenDataLayer.h",
                "QML/QmlCanOpenEntryRepeater.h",
                "QML/QmlCanOpenPdoMappedVar.h",
                "QML/QmlCanOpenPdoReceive.h",
                "QML/QmlCanOpenPdoTransmit.h",
                "QML/QmlCanOpenRecordEntry.h",
                "QML/QmlCanOpenSdoClientConfig.h",
                "QML/QmlCanOpenSdoServerConfig.h",
                "QML/QmlCanOpenSubEntry.h",
                "QML/QmlCanOpenValueModifier.h",
                "QML/QmlCanOpenVarEntry.h",
                "SparseArray.h",
            ]
        }
        Group {
            name: "C++ sources";
            files: [
                "CanOpenDefs.cpp",
                "CanOpenEntry.cpp",
                "CanOpenSubEntry.cpp",
                "CanOpenObjDict.cpp",
                "CanOpenProtocolManager.cpp",
                "QML/AbstractObdPreparator.cpp",
                "QML/QmlCanOpenArrayEntry.cpp",
                "QML/QmlCanOpenDataLayer.cpp",
                "QML/QmlCanOpenEntryRepeater.cpp",
                "QML/QmlCanOpenPdoMappedVar.cpp",
                "QML/QmlCanOpenPdoReceive.cpp",
                "QML/QmlCanOpenPdoTransmit.cpp",
                "QML/QmlCanOpenRecordEntry.cpp",
                "QML/QmlCanOpenSdoClientConfig.cpp",
                "QML/QmlCanOpenSdoServerConfig.cpp",
                "QML/QmlCanOpenSubEntry.cpp",
                "QML/QmlCanOpenValueModifier.cpp",
                "QML/QmlCanOpenVarEntry.cpp",
            ]
        }
        Group {
            qbs.install: true;
            qbs.installDir: project.installDirLibraries;
            fileTagsFilter: product.type;
        }
    }
    Product {
        name: (project.namePrefixTests + "canopen");

        Group {
            name: "QML data layers";
            prefix: "tests/";
            files: [
                "example_master.qml",
                "example_slave.qml",
            ]
        }
    }
}
