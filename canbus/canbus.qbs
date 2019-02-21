import qbs;

Project {
    name: "base";

    Product {
        name: (project.namePrefixBase + "base");
        type: (project.baseAsSharedLib ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixBase + "base");
        cpp.defines: {
            var ret = [];
            // if base is shared, use export/import, else use nothing (static/inline)
            if (project.baseAsSharedLib) {
                ret.push ("QTCAN_BASE_EXPORT=Q_DECL_EXPORT");
            }
            else {
                ret.push ("QTCAN_BASE_EXPORT=");
            }
            // if drivers are plugins, inform DriverLoader, else inform the drivers to bulld statically
            if (project.driversAsPlugins) {
                ret.push ("QTCAN_PLUGIN_DRIVERS=1");
            }
            else {
                ret.push ("QTCAN_STATIC_DRIVERS=1");
            }
            return ret;
        }

        Export {
            cpp.includePaths: ".";
            cpp.defines: {
                var ret = [];
                // if base is shared, use export/import, else use nothing (static/inline)
                if (project.baseAsSharedLib) {
                    ret.push ("QTCAN_BASE_EXPORT=Q_DECL_IMPORT");
                }
                else {
                    ret.push ("QTCAN_BASE_EXPORT=");
                }
                // if drivers are plugins, inform DriverLoader, else inform the drivers to bulld statically
                if (project.driversAsPlugins) {
                    ret.push ("QTCAN_PLUGIN_DRIVERS=1");
                }
                else {
                    ret.push ("QTCAN_STATIC_DRIVERS=1");
                }
                return ret;
            }

            Depends { name: "cpp"; }
            Depends { name: "Qt.core"; }
        }
        Depends { name: "cpp"; }
        Depends { name: "Qt.core"; }
        Group {
            name: "C++ sources";
            files: [
                "CanDriverLoader.cpp",
                "CanMessage.cpp",
            ]
        }
        Group {
            name: "C++ headers";
            files: [
                "CanDriver.h",
                "CanDriverLoader.h",
                "CanMessage.h",
                "QtCAN.h",
            ]
        }
        Group {
            qbs.install: true;
            qbs.installDir: project.installDirLibraries;
            fileTagsFilter: product.type;
        }
    }
}


