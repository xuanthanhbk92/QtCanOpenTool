import QtCAN.CANopen 2.0;

DataLayer {
    name: "Test Master OBD";

    ValueModifier { index: 0x1008; subIndex: 0; data: "MS-1"; }

    VarEntry { index: 0x1009; dataType: DataTypes.VisibleStr; dataLen: 4; data: "v1.0"; }
    VarEntry { index: 0x100A; dataType: DataTypes.VisibleStr; dataLen: 4; data: "v1.0"; }

    SdoServer {
        srvNum: 1;
        cobIdCliRequest: 0x604;
        cobIdSrvReply: 0x584;
    }

    SdoClient {
        cliNum: 1;
        cobIdCliRequest: 0x601;
        cobIdSrvReply: 0x581;
        nodeId: 0x01;
    }

    PdoTransmit {
        pdoNum: 1;
        cobId: 0x181;

        PdoMappedVar { index: 0x2345; subIndex: 1; bitsCount: 16; }
        PdoMappedVar { index: 0x2345; subIndex: 2; bitsCount: 16; }
        PdoMappedVar { index: 0x2345; subIndex: 3; bitsCount: 16; }
        PdoMappedVar { index: 0x2345; subIndex: 4; bitsCount: 16; }
    }

    EntryRepeater {
        indexFirst: 0x2000;
        indexLast: 0x2010;
        entryTemplate: VarEntry {
            dataType: DataTypes.Int32;
            metaData: ({ "uid" : (num % 2 ? "ODD_TEST_OBJECT_%1" : "EVEN_TEST_OBJECT_%1").arg (num) });
            data: 0xABCD1234;
        }
    }

    ArrayEntry {
        index: 0x2345;
        count: 20;
        dataType: DataTypes.UInt16;
        metaData: ({ "uid" : "ARRAY_SUBENTRY_%1".arg (subIdx) });
    }

    EntryRepeater {
        indexes: [0x3002, 0x3004, 0x3008, 0x300F];
        entryTemplate: VarEntry {
            dataType: DataTypes.VisibleStr;
            metaData: ({ "uid" : "CUSTOM_OBJECT_%1".arg (num) });
            data: "Hello world !";
            dataLen: 20;
        }
    }

    RecordEntry {
        index: 0x3456;

        SubEntry { dataType: DataTypes.Int16;  data: 0x1234;     metaData: ({ "uid" : "RECORD_VAR1" }); }
        SubEntry { dataType: DataTypes.Bool;   data: true;       metaData: ({ "uid" : "RECORD_VAR2" }); }
        SubEntry { dataType: DataTypes.Bool;   data: false;      metaData: ({ "uid" : "RECORD_VAR3" }); }
        SubEntry { dataType: DataTypes.UInt8;  data: 0xAB;       metaData: ({ "uid" : "RECORD_VAR4" }); }
        SubEntry { dataType: DataTypes.UInt32; data: 0xDEADBEEF; metaData: ({ "uid" : "RECORD_VAR5" }); }
    }


    VarEntry {
        index: 0x9876;
        dataType: DataTypes.Domain;
        dataLen: 512;
        metaData: ({ "uid" : "DOMAIN_OBJECT" });
    }

}
