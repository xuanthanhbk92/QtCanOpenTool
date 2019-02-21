import qbs;
import qbs.File;

Project {
    name: "3rdparty";
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
        "QtQmlTricks-Models/QtQmlModels.qbs",
        "QtQmlTricks-SuperMacros/QtSuperMacros.qbs",
        "QtQmlTricks-UiElements/QtQuickUiElements.qbs",
    ];
}
