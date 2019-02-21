import qbs;
import qbs.File;

Project {
    name: "protocols";
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
        "CANopen/CANopen.qbs",
        "J1939/J1939.qbs",
    ];
}
