    0x0028, 0x8002,

// /* **************************************************/
// /* Pattern Name ==> ./pattern/pcimaster_lcd.cmds    */
// /* **************************************************/

    0x1100, 0x2000,
    0x1102, 0x0e10,
    0x1104, 0x4822,
    0x1106, 0x0000,
    0x110c, 0x02d0,
    0x110e, 0x0240,
    0x1110, 0x05a0,
    0x1112, 0x0000,
    0x1114, 0x0c00,
    0x1116, 0x1800,

    0x1170, 0xa000,
    0x1172, 0x0042,// //Enable RGB2YUV, UYVY
    0x1174, 0x0081,
    0x1176, 0x0019,
    0x1178, 0x0010,
    0x117a, 0x03da,
    0x117c, 0x03b6,
    0x117e, 0x0070,
    0x1180, 0x0080,
    0x1182, 0x0070,
    0x1184, 0x03a2,
    0x1186, 0x03ee,
    0x1188, 0x0080,

  //0x1234,
// /* **************************************************/
// /* CTG Setting/
// /* **************************************************/

    0x1190, 0x0037,
    0x1192, 0x06c0,
    0x1194, 0x0271,
    0x1196, 0x0000,
// // HSync
    0x11a0, 0x1096,
    0x11a2, 0x0001,
    0x11a4, 0x2018,
    0x11a6, 0x0001,
    0x11a8, 0x1096,
    0x11aa, 0x0001,
    0x11ac, 0x2018,
    0x11ae, 0x0001,
// // VSync
    0x11b0, 0x5018,
    0x11b2, 0x0001,
    0x11b4, 0x2018,
    0x11b6, 0x0004,
    0x11b8, 0x1360,
    0x11ba, 0x0139,
    0x11bc, 0x2360,
    0x11be, 0x013c,
// // DE
    0x11c0, 0x1120,
    0x11c2, 0x0017,
    0x11c4, 0x26c0,
    0x11c6, 0x0137,
    0x11c8, 0x1120,
    0x11ca, 0x0150,
    0x11cc, 0x26c0,
    0x11ce, 0x0270,

// // TVEHSync
    0x11e0, 0x1120,
    0x11e2, 0x0001,
    0x11e4, 0x26c0,
    0x11e6, 0x0001,
    0x11e8, 0x1120,
    0x11ea, 0x0001,
    0x11ec, 0x26c0,
    0x11ee, 0x0001,

// // TVEVSync
    0x11f0, 0x5018,
    0x11f2, 0x0001,
    0x11f4, 0x2018,
    0x11f6, 0x0004,
    0x11f8, 0x13f0,
    0x11fa, 0x0139,
    0x11fc, 0x23f0,
    0x11fe, 0x013c,

    0x1258, 0x8100, // IO output

    // for software
    0x1108, 1,      // Display mode, 0: LCD
                    //               1: 576i
                    //               2: 576p
                    //               3: 480i
                    //               4: 480p
                    //               6: VGA 800x600

    0x111e, 0x0001,
    0xFFFF, 100,
    0x111e, 0x0003,
