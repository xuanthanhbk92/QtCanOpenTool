import qbs;

Project {
    name: "J1939";

    Product {
        name: (project.namePrefixProtocols + "j1939");
        type: "dynamiclibrary";
        targetName: (project.targetPrefixProtocols + "J1939");


    }
}
