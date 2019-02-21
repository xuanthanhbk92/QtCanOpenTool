import qbs;
import qbs.File;

Project {
    name: "drivers";
    references: {
        var ret = [];
        for (var idx = 0; idx < subProjects.length; idx++) {
            var subProjectFilePath = subProjects [idx];
            if (File.exists (sourceDirectory + "/" + name + "/" + subProjectFilePath)) {
                ret.push (subProjectFilePath);
            }
        }
        return ret;
    }

    readonly property stringList subProjects : [
        "SocketCAN/SocketCAN.qbs",
    ];
}
