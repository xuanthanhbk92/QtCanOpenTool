import qbs;

Project {
    name: "utils";

    Product {
        name: (project.namePrefixBase + "utils");
        type: (project.utilsAsSharedLib ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixBase + "Utils");
        cpp.includePaths: ["."];
        cpp.defines: {
            var ret = [];
            if (project.utilsAsSharedLib) {
                ret.push ("QTCAN_UTILS_EXPORT=Q_DECL_EXPORT");
            }
            else {
                ret.push ("QTCAN_UTILS_EXPORT=");
            }
            return ret;
        }

        readonly property stringList qmlImportPaths : [sourceDirectory + "/imports"]; // equivalent to QML_IMPORT_PATH += $$PWD/imports

        Export {
            cpp.includePaths: ["."];
            cpp.defines: {
                var ret = [];
                if (project.utilsAsSharedLib) {
                    ret.push ("QTCAN_UTILS_EXPORT=Q_DECL_IMPORT");
                }
                else {
                    ret.push ("QTCAN_UTILS_EXPORT=");
                }
                return ret;
            }

            Depends { name: "cpp"; }
            Depends { name: "Qt"; submodules: ["core", "gui", "qml", "quick"]; }
        }
        Depends { name: "cpp"; }
        Depends { name: "Qt"; submodules: ["core", "gui", "qml", "quick"]; }
        Depends { name: (project.namePrefixBase + "base"); }
        Depends { name: "libqtqmltricks-qtsupermacros"; }
        Depends { name: "libqtqmltricks-qtqmlmodels"; }
        Depends { name: "libqtqmltricks-qtquickuielements"; }
        Group {
            name: "C++ headers";
            files: [
                "ByteArrayQmlWrapper.h",
                "CanDataQmlWrapper.h",
                "CanDriverQmlWrapper.h",
                "TypesQmlWrapper.h",
            ]
        }
        Group {
            name: "C++ sources";
            files: [
                "ByteArrayQmlWrapper.cpp",
                "CanDataQmlWrapper.cpp",
                "CanDriverQmlWrapper.cpp",
            ]
        }
        Group {
            name: "QML components";
            prefix: "imports/QtCAN/Utils.2/";
            files: [
                "DriverOptionsForm.qml",
                "DriverSelector.qml",
                "qmldir",
            ]
        }
        Group {
            name: "Qt resources bundle";
            files: [
                "data_utils.qrc",
            ]
        }
        Group {
            qbs.install: project.utilsAsSharedLib;
            qbs.installDir: project.installDirLibraries;
            fileTagsFilter: product.type;
        }
    }
}
