import qbs;

Project {
    name: "JsonTCP";

    Product {
        name: (project.namePrefixDrivers + "jsontcp");
        type: (project.driversAsPlugins ? "dynamiclibrary" : "staticlibrary");
        targetName: (project.targetPrefixDrivers + "JsonTCP");
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
        Depends { name: "Qt"; submodules:  ["core", "network"]; }
        Depends { name: "libqtcan-base"; }
        Group {
            name: "C++ files";
            files: [
                "CanDriver_jsonTcp.h",
                "CanDriver_jsonTcp.cpp",
            ]
        }
        Group {
            qbs.install: project.driversAsPlugins;
            qbs.installDir: project.installDirDrivers;
            fileTagsFilter: product.type;
        }
    }
}
