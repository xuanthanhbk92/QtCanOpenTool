import qbs;

Project {
    name: "SocketCAN";

    Product {
        name: (project.namePrefixDrivers + "socketcan-raw");
        type: (project.driversAsPlugins ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixDrivers + "SocketCAN_RAW");
        condition: qbs.targetOS.contains ("linux");
        cpp.defines: {
            var ret = [];
            if (project.driversAsPlugins) {
                ret.push ("QTCAN_DRIVER_EXPORT=Q_DECL_EXPORT");
            }
            else {
                ret.push ("QTCAN_DRIVER_EXPORT=");
            }
            return ret;
        }

        Export {
            cpp.includePaths: ".";
            cpp.defines: {
                var ret = [];
                if (project.driversAsPlugins) {
                    ret.push ("QTCAN_DRIVER_EXPORT=Q_DECL_IMPORT");
                }
                else {
                    ret.push ("QTCAN_DRIVER_EXPORT=");
                }
                return ret;
            }

            Depends { name: "cpp"; }
        }
        Depends { name: "cpp"; }
        Depends { name: "Qt.core"; }
        Depends { name: "libqtcan-base"; }
        Group {
            name: "C++ files";
            files: [
                "CanDriver_socketCanCommon.h",
                "CanDriver_socketCanCommon.cpp",
                "CanDriver_socketCanRaw.h",
                "CanDriver_socketCanRaw.cpp",
            ]
        }
        Group {
            qbs.install: project.driversAsPlugins;
            qbs.installDir: project.installDirDrivers;
            fileTagsFilter: product.type;
        }
    }
    Product {
        name: (project.namePrefixDrivers + "socketcan-bcm");
        type: (project.driversAsPlugins ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixDrivers + "SocketCAN_BCM");
        condition: qbs.targetOS.contains ("linux");
        cpp.defines: {
            var ret = [];
            if (project.driversAsPlugins) {
                ret.push ("QTCAN_DRIVER_EXPORT=Q_DECL_EXPORT");
            }
            else {
                ret.push ("QTCAN_DRIVER_EXPORT=");
            }
            return ret;
        }

        Export {
            cpp.includePaths: ".";
            cpp.defines: {
                var ret = [];
                if (project.driversAsPlugins) {
                    ret.push ("QTCAN_DRIVER_EXPORT=Q_DECL_IMPORT");
                }
                else {
                    ret.push ("QTCAN_DRIVER_EXPORT=");
                }
                return ret;
            }

            Depends { name: "cpp"; }
        }
        Depends { name: "cpp"; }
        Depends { name: "Qt.core"; }
        Depends { name: "libqtcan-base"; }
        Group {
            name: "C++ files";
            files: [
                "CanDriver_socketCanCommon.h",
                "CanDriver_socketCanCommon.cpp",
                "CanDriver_socketCanBcm.h",
                "CanDriver_socketCanBcm.cpp",
            ]
        }
        Group {
            qbs.install: project.driversAsPlugins;
            qbs.installDir: project.installDirDrivers;
            fileTagsFilter: product.type;
        }
    }
}
