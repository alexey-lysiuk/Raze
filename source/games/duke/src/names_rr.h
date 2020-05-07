//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------


#pragma once

BEGIN_DUKE_NS

enum
{
	SECTOREFFECTOR = 1,
	ACTIVATOR = 2,
	TOUCHPLATE = 3,
	ACTIVATORLOCKED = 4,
	MUSICANDSFX = 5,
	LOCATORS = 6,
	CYCLER = 7,
	MASTERSWITCH = 8,
	RESPAWN = 9,
	GPSPEED = 10,
	RRTILE11 = 11,
	PLEASEWAIT = 12,
	FOF = 13,
	//WEATHERWARN = 14,
	RPG2SPRITE = 14,
	DUKETAG = 15,
	SIGN1 = 16,
	SIGN2 = 17,
	RRTILE18 = 18,
	RRTILE19 = 19,
	ARROW = 20,
	FIRSTGUNSPRITE = 21,
	CHAINGUNSPRITE = 22,
	RPGSPRITE = 23,
	FREEZESPRITE = 24,
	SHRINKERSPRITE = 25,
	HEAVYHBOMB = 26,
	TRIPBOMBSPRITE = 27,
	SHOTGUNSPRITE = 28,
	DEVISTATORSPRITE = 29,
	HEALTHBOX = 30,
	AMMOBOX = 31,
	GROWSPRITEICON = 32,
	INVENTORYBOX = 33,
	RRTILE34 = 34,
	RRTILE35 = 35,
	DESTRUCTO = 36,
	FREEZEAMMO = 37,
	RRTILE38 = 38,
	AMMO = 40,
	BATTERYAMMO = 41,
	DEVISTATORAMMO = 42,
	RRTILE43 = 43,
	RPGAMMO = 44,
	GROWAMMO = 45,
	CRYSTALAMMO = 46,
	HBOMBAMMO = 47,
	AMMOLOTS = 48,
	SHOTGUNAMMO = 49,
	COLA = 51,
	SIXPAK = 52,
	FIRSTAID = 53,
	SHIELD = 54,
	STEROIDS = 55,
	AIRTANK = 56,
	JETPACK = 57,
	HEATSENSOR = 59,
	ACCESSCARD = 60,
	BOOTS = 61,
	GUTMETER = 62,
	RRTILE63 = 63,
	RRTILE64 = 64,
	RRTILE65 = 65,
	RRTILE66 = 66,
	RRTILE67 = 67,
	RRTILE68 = 68,
	MIRRORBROKE = 70,
	SOUNDFX = 71,
	TECHLIGHT2 = 72,
	TECHLIGHTBUST2 = 73,
	TECHLIGHT4 = 74,
	TECHLIGHTBUST4 = 75,
	WALLLIGHT4 = 76,
	WALLLIGHTBUST4 = 77,
	MOTOAMMO = 78,
	BUTTON1 = 80,
	ACCESSSWITCH = 82,
	SLOTDOOR = 84,
	LIGHTSWITCH = 86,
	SPACEDOORSWITCH = 88,
	SPACELIGHTSWITCH = 90,
	FRANKENSTINESWITCH = 92,
	NUKEBUTTON = 94,
	MULTISWITCH = 98,
	DOORTILE1 = 102,
	DOORTILE2 = 103,
	DOORTILE3 = 104,
	DOORTILE4 = 105,
	DOORTILE5 = 106,
	DOORTILE6 = 107,
	DOORTILE7 = 108,
	DOORTILE8 = 109,
	DOORTILE9 = 110,
	DOORTILE10 = 111,
	DOORTILE14 = 115,
	DOORTILE15 = 116,
	DOORTILE16 = 117,
	DOORTILE18 = 119,
	DOORSHOCK = 120,
	DIPSWITCH = 121,
	DIPSWITCH2 = 123,
	TECHSWITCH = 125,
	DIPSWITCH3 = 127,
	ACCESSSWITCH2 = 129,
	REFLECTWATERTILE = 131,
	FLOORSLIME = 132,
	BIGFORCE = 135,
	EPISODE = 137,
	MASKWALL1 = 138,
	MASKWALL2 = 140,
	MASKWALL3 = 141,
	MASKWALL4 = 142,
	MASKWALL5 = 143,
	MASKWALL6 = 144,
	MASKWALL8 = 146,
	MASKWALL9 = 147,
	MASKWALL10 = 148,
	MASKWALL11 = 149,
	MASKWALL12 = 150,
	MASKWALL13 = 151,
	MASKWALL14 = 152,
	MASKWALL15 = 153,
	FIREEXT = 155,
	W_LIGHT = 156,
	SCREENBREAK1 = 159,
	SCREENBREAK2 = 160,
	SCREENBREAK3 = 161,
	SCREENBREAK4 = 162,
	SCREENBREAK5 = 163,
	SCREENBREAK6 = 164,
	SCREENBREAK7 = 165,
	SCREENBREAK8 = 166,
	SCREENBREAK9 = 167,
	SCREENBREAK10 = 168,
	SCREENBREAK11 = 169,
	SCREENBREAK12 = 170,
	SCREENBREAK13 = 171,
	W_TECHWALL1 = 185,
	W_TECHWALL2 = 186,
	W_TECHWALL3 = 187,
	W_TECHWALL4 = 188,
	W_TECHWALL10 = 189,
	W_TECHWALL15 = 191,
	W_TECHWALL16 = 192,
	STATIC = 195,
	W_SCREENBREAK = 199,
	W_HITTECHWALL3 = 205,
	W_HITTECHWALL4 = 206,
	W_HITTECHWALL2 = 207,
	W_HITTECHWALL1 = 208,
	FANSPRITE = 210,
	FANSPRITEBROKE = 215,
	FANSHADOW = 216,
	FANSHADOWBROKE = 219,
	DOORTILE19 = 229,
	DOORTILE20 = 230,
	DOORTILE22 = 232,
	GRATE1 = 234,
	BGRATE1 = 235,
	SPLINTERWOOD = 237,
	WATERDRIP = 239,
	WATERBUBBLE = 240,
	WATERBUBBLEMAKER = 241,
	W_FORCEFIELD = 242,
	WALLLIGHT3 = 244,
	WALLLIGHTBUST3 = 245,
	WALLLIGHT1 = 246,
	WALLLIGHTBUST1 = 247,
	WALLLIGHT2 = 248,
	WALLLIGHTBUST2 = 249,
	LIGHTSWITCH2 = 250,
	UFOBEAM = 252,
	RRTILE280 = 280,
	RRTILE281 = 281,
	RRTILE282 = 282,
	RRTILE283 = 283,
	RRTILE285 = 285,
	RRTILE286 = 286,
	RRTILE287 = 287,
	RRTILE288 = 288,
	RRTILE289 = 289,
	RRTILE290 = 290,
	RRTILE291 = 291,
	RRTILE292 = 292,
	RRTILE293 = 293,
	RRTILE295 = 295,
	RRTILE296 = 296,
	RRTILE297 = 297,
	CDPLAYER = 370,
	RRTILE380 = 380,
	RRTILE403 = 403,
	RRTILE409 = 409,
	BIGFNTCURSOR = 512,
	SMALLFNTCURSOR = 513,
	STARTALPHANUM = 514,
	ENDALPHANUM = 607,
	BIGALPHANUM = 632,
	BIGPERIOD = 694,
	BIGCOMMA = 695,
	BIGX = 696,
	BIGQ = 697,
	BIGSEMI = 698,
	BIGCOLIN = 699,
	THREEBYFIVE = 702,
	BIGAPPOS = 714,
	MINIFONT = 718,
	W_NUMBERS = 810,
	BLANK = 820,
	RESPAWNMARKERRED = 866,
	RESPAWNMARKERYELLOW = 876,
	RESPAWNMARKERGREEN = 886,
	SPINNINGNUKEICON = 896,
	GUTMETER_LIGHT1 = 920,
	GUTMETER_LIGHT2 = 921,
	GUTMETER_LIGHT3 = 922,
	GUTMETER_LIGHT4 = 923,
	AMMO_ICON = 930,
	CLOUDYSKIES = 1021,
	MOONSKY1 = 1022,
	MOONSKY2 = 1023,
	MOONSKY3 = 1024,
	MOONSKY4 = 1025,
	BIGORBIT1 = 1026,
	BIGORBIT2 = 1027,
	BIGORBIT3 = 1028,
	BIGORBIT4 = 1029,
	BIGORBIT5 = 1030,
	LA = 1031,
	REDSKY1 = 1040,
	REDSKY2 = 1041,
	WATERTILE = 1044,
	WATERTILE2 = 1045,
	SATELLITE = 1049,
	VIEWSCREEN2 = 1052,
	VIEWSCREENBROKE = 1054,
	VIEWSCREEN = 1055,
	GLASS = 1056,
	GLASS2 = 1057,
	STAINGLASS1 = 1063,
	SATELITE = 1066,
	FUELPOD = 1067,
	SLIMEPIPE = 1070,
	CRACK1 = 1075,
	CRACK2 = 1076,
	CRACK3 = 1077,
	CRACK4 = 1078,
	FOOTPRINTS = 1079,
	DOMELITE = 1080,
	CAMERAPOLE = 1083,
	CHAIR1 = 1085,
	CHAIR2 = 1086,
	BROKENCHAIR = 1088,
	MIRROR = 1089,
	WATERFOUNTAIN = 1092,
	WATERFOUNTAINBROKE = 1096,
	FEMMAG1 = 1097,
	TOILET = 1098,
	STALL = 1100,
	STALLBROKE = 1102,
	FEMMAG2 = 1106,
	REACTOR2 = 1107,
	REACTOR2BURNT = 1108,
	REACTOR2SPARK = 1109,
	SOLARPANNEL = 1114,
	NAKED1 = 1115,
	ANTENNA = 1117,
	TOILETBROKE = 1120,
	PIPE2 = 1121,
	PIPE1B = 1122,
	PIPE3 = 1123,
	PIPE1 = 1124,
	PIPE2B = 1126,
	BOLT1 = 1127,
	PIPE3B = 1132,
	CAMERA1 = 1134,
	BRICK = 1139,
	VACUUM = 1141,
	JURYGUY = 1142,
	FOOTPRINTS2 = 1144,
	FOOTPRINTS3 = 1145,
	FOOTPRINTS4 = 1146,
	EGG = 1147,
	SCALE = 1150,
	CHAIR3 = 1152,
	CAMERALIGHT = 1157,
	MOVIECAMERA = 1158,
	FOOTPRINT = 1160,
	IVUNIT = 1163,
	POT1 = 1164,
	POT2 = 1165,
	POT3 = 1166,
	STATUE = 1168,
	MIKE = 1170,
	VASE = 1172,
	SUSHIPLATE1 = 1174,
	SUSHIPLATE2 = 1175,
	SUSHIPLATE3 = 1176,
	SUSHIPLATE4 = 1178,
	SUSHIPLATE5 = 1180,
	BIGHOLE2 = 1182,
	STRIPEBALL = 1184,
	QUEBALL = 1185,
	POCKET = 1186,
	WOODENHORSE = 1187,
	TREE1 = 1191,
	TREE2 = 1193,
	CACTUS = 1194,
	TOILETWATER = 1196,
	NEON1 = 1200,
	NEON2 = 1201,
	CACTUSBROKE = 1203,
	BOUNCEMINE = 1204,
	BROKEFIREHYDRENT = 1210,
	BOX = 1211,
	BULLETHOLE = 1212,
	BOTTLE1 = 1215,
	BOTTLE2 = 1216,
	BOTTLE3 = 1217,
	BOTTLE4 = 1218,
	BOTTLE5 = 1219,
	BOTTLE6 = 1220,
	BOTTLE7 = 1221,
	BOTTLE8 = 1222,
	SNAKEP = 1224,
	DOLPHIN1 = 1225,
	DOLPHIN2 = 1226,
	HYDRENT = 1228,
	TIRE = 1230,
	PIPE5 = 1232,
	PIPE6 = 1233,
	PIPE4 = 1234,
	PIPE4B = 1235,
	BROKEHYDROPLANT = 1237,
	PIPE5B = 1239,
	NEON3 = 1241,
	NEON4 = 1242,
	NEON5 = 1243,
	SPOTLITE = 1247,
	HANGOOZ = 1249,
	HORSEONSIDE = 1251,
	GLASSPIECES = 1256,
	HORSELITE = 1259,
	DONUTS = 1263,
	NEON6 = 1264,
	CLOCK = 1266,
	RUBBERCAN = 1268,
	BROKENCLOCK = 1270,
	PLUG = 1272,
	OOZFILTER = 1273,
	FLOORPLASMA = 1276,
	HANDPRINTSWITCH = 1278,
	BOTTLE10 = 1280,
	BOTTLE11 = 1281,
	BOTTLE12 = 1282,
	BOTTLE13 = 1283,
	BOTTLE14 = 1284,
	BOTTLE15 = 1285,
	BOTTLE16 = 1286,
	BOTTLE17 = 1287,
	BOTTLE18 = 1288,
	BOTTLE19 = 1289,
	VENDMACHINE = 1291,
	VENDMACHINEBROKE = 1293,
	COLAMACHINE = 1294,
	COLAMACHINEBROKE = 1296,
	CRANEPOLE = 1298,
	CRANE = 1299,
	BARBROKE = 1302,
	BLOODPOOL = 1303,
	NUKEBARREL = 1304,
	NUKEBARRELDENTED = 1305,
	NUKEBARRELLEAKED = 1306,
	CANWITHSOMETHING = 1309,
	MONEY = 1310,
	BANNER = 1313,
	EXPLODINGBARREL = 1315,
	EXPLODINGBARREL2 = 1316,
	FIREBARREL = 1317,
	SEENINE = 1324,
	SEENINEDEAD = 1325,
	STEAM = 1327,
	CEILINGSTEAM = 1332,
	PIPE6B = 1337,
	TRANSPORTERBEAM = 1338,
	RAT = 1344,
	TRASH = 1346,
	HELECOPT = 1348,
	FETUSJIB = 1349,
	HOLODUKE = 1350,
	MONK = 1354,
	SPACEMARINE = 1355,
	LUKE = 1356,
	INDY = 1357,
	FETUS = 1360,
	FETUSBROKE = 1361,
	WATERSPLASH2 = 1383,
	FIREVASE = 1388,
	SCRATCH = 1389,
	BLOOD = 1391,
	TRANSPORTERSTAR = 1398,
	LOOGIE = 1405,
	FIST = 1408,
	FREEZEBLAST = 1409,
	DEVISTATORBLAST = 1410,
	TONGUE = 1414,
	MORTER = 1416,
	MUD = 1420,
	SHRINKEREXPLOSION = 1421,
	RADIUSEXPLOSION = 1426,
	FORCERIPPLE = 1427,
	CANNONBALL = 1437,
	INNERJAW = 1439,
	EXPLOSION2 = 1441,
	EXPLOSION3 = 1442,
	JIBS1 = 1463,
	JIBS2 = 1468,
	JIBS3 = 1473,
	JIBS4 = 1478,
	JIBS5 = 1483,
	CRACKKNUCKLES = 1489,
	HEADERBAR = 1493,
	BURNING = 1494,
	FIRE = 1495,
	USERWEAPON = 1510,
	JIBS6 = 1515,
	BLOODSPLAT1 = 1525,
	BLOODSPLAT3 = 1526,
	BLOODSPLAT2 = 1527,
	BLOODSPLAT4 = 1528,
	OOZ = 1529,
	WALLBLOOD1 = 1530,
	WALLBLOOD2 = 1531,
	WALLBLOOD3 = 1532,
	WALLBLOOD4 = 1533,
	WALLBLOOD5 = 1534,
	WALLBLOOD6 = 1535,
	WALLBLOOD7 = 1536,
	WALLBLOOD8 = 1537,
	OOZ2 = 1538,
	BURNING2 = 1539,
	FIRE2 = 1540,
	SMALLSMOKE = 1554,
	SMALLSMOKEMAKER = 1555,
	FLOORFLAME = 1558,
	GREENSLIME = 1575,
	WATERDRIPSPLASH = 1585,
	SCRAP6 = 1595,
	SCRAP1 = 1605,
	SCRAP2 = 1609,
	SCRAP3 = 1613,
	SCRAP4 = 1617,
	SCRAP5 = 1621,
	ROTATEGUN = 1624,
	BETAVERSION = 1629,
	PLAYERISHERE = 1630,
	PLAYERWASHERE = 1631,
	SELECTDIR = 1632,
	F1HELP = 1633,
	NOTCHON = 1634,
	NOTCHOFF = 1635,
	RRTILE1636 = 1636,
	DUKEICON = 1637,
	BADGUYICON = 1638,
	FOODICON = 1639,
	GETICON = 1640,
	MENUSCREEN = 1641,
	MENUBAR = 1642,
	KILLSICON = 1643,
	FIRSTAID_ICON = 1645,
	HEAT_ICON = 1646,
	BOTTOMSTATUSBAR = 1647,
	BOOT_ICON = 1648,
	WEAPONBAR = 1649,
	FRAGBAR = 1650,
	JETPACK_ICON = 1652,
	AIRTANK_ICON = 1653,
	STEROIDS_ICON = 1654,
	HOLODUKE_ICON = 1655,
	ACCESS_ICON = 1656,
	DIGITALNUM = 1657,
	CAMCORNER = 1667,
	CAMLIGHT = 1669,
	LOGO = 1670,
	TITLE = 1671,
	NUKEWARNINGICON = 1672,
	MOUSECURSOR = 1673,
	SLIDEBAR = 1674,
	DUKECAR = 1676,
	DREALMS = 1677,
	BETASCREEN = 1678,
	WINDOWBORDER1 = 1679,
	TEXTBOX = 1680,
	WINDOWBORDER2 = 1681,
	DUKENUKEM = 1682,
	THREEDEE = 1683,
	INGAMEDUKETHREEDEE = 1684,
	TENSCREEN = 1685,
	PLUTOPAKSPRITE = 1686,
	CROSSHAIR = 1689,
	FALLINGCLIP = 1699,
	CLIPINHAND = 1700,
	HAND = 1701,
	SHELL = 1702,
	SHOTGUNSHELL = 1704,
	RPGMUZZLEFLASH = 1714,
	CATLITE = 1721,
	HANDHOLDINGLASER = 1732,
	TRIPBOMB = 1735,
	LASERLINE = 1736,
	HANDHOLDINGACCESS = 1737,
	HANDREMOTE = 1739,
	TIP = 1745,
	GLAIR = 1747,
	SPACEMASK = 1753,
	RRTILE1752 = 1752,
	FORCESPHERE = 1759,
	SHOTSPARK1 = 1764,
	RPG = 1774,
	RPG2 = 1781,
	// = LASERSITE = 1781,
	RRTILE1790 = 1790,
	RRTILE1792 = 1792,
	RRTILE1801 = 1801,
	RRTILE1805 = 1805,
	RRTILE1807 = 1807,
	RRTILE1808 = 1808,
	RRTILE1812 = 1812,
	RRTILE1814 = 1814,
	RRTILE1817 = 1817,
	RRTILE1821 = 1821,
	RRTILE1824 = 1824,
	RRTILE1826 = 1826,
	RRTILE1850 = 1850,
	RRTILE1851 = 1851,
	RRTILE1856 = 1856,
	RRTILE1877 = 1877,
	RRTILE1878 = 1878,
	RRTILE1938 = 1938,
	RRTILE1939 = 1939,
	RRTILE1942 = 1942,
	RRTILE1944 = 1944,
	RRTILE1945 = 1945,
	RRTILE1947 = 1947,
	RRTILE1951 = 1951,
	RRTILE1952 = 1952,
	RRTILE1953 = 1953,
	RRTILE1961 = 1961,
	RRTILE1964 = 1964,
	RRTILE1973 = 1973,
	RRTILE1985 = 1985,
	RRTILE1986 = 1986,
	RRTILE1987 = 1987,
	RRTILE1988 = 1988,
	RRTILE1990 = 1990,
	RRTILE1995 = 1995,
	RRTILE1996 = 1996,
	RRTILE2004 = 2004,
	RRTILE2005 = 2005,
	POPCORN = 2021,
	RRTILE2022 = 2022,
	LANEPICS = 2023,
	RRTILE2025 = 2025,
	RRTILE2026 = 2026,
	RRTILE2027 = 2027,
	RRTILE2028 = 2028,
	RRTILE2034 = 2034,
	RRTILE2050 = 2050,
	RRTILE2052 = 2052,
	RRTILE2053 = 2053,
	RRTILE2056 = 2056,
	RRTILE2060 = 2060,
	RRTILE2072 = 2072,
	RRTILE2074 = 2074,
	RRTILE2075 = 2075,
	RRTILE2083 = 2083,
	COOLEXPLOSION1 = 2095,
	RRTILE2097 = 2097,
	RRTILE2121 = 2121,
	RRTILE2122 = 2122,
	RRTILE2123 = 2123,
	RRTILE2124 = 2124,
	RRTILE2125 = 2125,
	RRTILE2126 = 2126,
	RRTILE2137 = 2137,
	RRTILE2132 = 2132,
	RRTILE2136 = 2136,
	RRTILE2139 = 2139,
	RRTILE2150 = 2150,
	RRTILE2151 = 2151,
	RRTILE2152 = 2152,
	RRTILE2156 = 2156,
	RRTILE2157 = 2157,
	RRTILE2158 = 2158,
	RRTILE2159 = 2159,
	RRTILE2160 = 2160,
	RRTILE2161 = 2161,
	RRTILE2175 = 2175,
	RRTILE2176 = 2176,
	RRTILE2178 = 2178,
	RRTILE2186 = 2186,
	RRTILE2214 = 2214,
	WAITTOBESEATED = 2215,
	OJ = 2217,
	HURTRAIL = 2221,
	POWERSWITCH1 = 2222,
	LOCKSWITCH1 = 2224,
	POWERSWITCH2 = 2226,
	ATM = 2229,
	STATUEFLASH = 2231,
	ATMBROKE = 2233,
	FEMPIC5 = 2235,
	FEMPIC6 = 2236,
	FEMPIC7 = 2237,
	REACTOR = 2239,
	REACTORSPARK = 2243,
	REACTORBURNT = 2247,
	HANDSWITCH = 2249,
	CIRCLEPANNEL = 2251,
	CIRCLEPANNELBROKE = 2252,
	PULLSWITCH = 2254,
	ALIENSWITCH = 2259,
	DOORTILE21 = 2261,
	DOORTILE17 = 2263,
	MASKWALL7 = 2264,
	JAILBARBREAK = 2265,
	DOORTILE11 = 2267,
	DOORTILE12 = 2268,
	EXPLOSION2BOT = 2272,
	RRTILE2319 = 2319,
	RRTILE2321 = 2321,
	RRTILE2326 = 2326,
	RRTILE2329 = 2329,
	RRTILE2357 = 2357,
	RRTILE2382 = 2382,
	RRTILE2430 = 2430,
	RRTILE2431 = 2431,
	RRTILE2432 = 2432,
	RRTILE2437 = 2437,
	RRTILE2443 = 2443,
	RRTILE2445 = 2445,
	RRTILE2446 = 2446,
	RRTILE2450 = 2450,
	RRTILE2451 = 2451,
	RRTILE2455 = 2455,
	RRTILE2460 = 2460,
	RRTILE2465 = 2465,
	BONUSSCREEN = 2510,
	VIEWBORDER = 2520,
	VICTORY1 = 2530,
	ORDERING = 2531,
	TEXTSTORY = 2541,
	LOADSCREEN = 2542,
	RRTILE2560 = 2560,
	RRTILE2562 = 2562,
	RRTILE2564 = 2564,
	RRTILE2573 = 2573,
	RRTILE2574 = 2574,
	RRTILE2577 = 2577,
	RRTILE2578 = 2578,
	RRTILE2581 = 2581,
	RRTILE2583 = 2583,
	RRTILE2604 = 2604,
	RRTILE2610 = 2610,
	RRTILE2613 = 2613,
	RRTILE2621 = 2621,
	RRTILE2622 = 2622,
	RRTILE2636 = 2636,
	RRTILE2637 = 2637,
	RRTILE2654 = 2654,
	RRTILE2656 = 2656,
	RRTILE2676 = 2676,
	RRTILE2689 = 2689,
	RRTILE2697 = 2697,
	RRTILE2702 = 2702,
	RRTILE2707 = 2707,
	RRTILE2732 = 2732,
	HATRACK = 2717,
	DESKLAMP = 2719,
	COFFEEMACHINE = 2721,
	CUPS = 2722,
	GAVALS = 2723,
	GAVALS2 = 2724,
	POLICELIGHTPOLE = 2726,
	FLOORBASKET = 2728,
	PUKE = 2729,
	DOORTILE23 = 2731,
	TOPSECRET = 2733,
	SPEAKER = 2734,
	TEDDYBEAR = 2735,
	ROBOTDOG = 2737,
	ROBOTPIRATE = 2739,
	ROBOTMOUSE = 2740,
	MAIL = 2741,
	MAILBAG = 2742,
	HOTMEAT = 2744,
	COFFEEMUG = 2745,
	DONUTS2 = 2746,
	TRIPODCAMERA = 2747,
	METER = 2748,
	DESKPHONE = 2749,
	GUMBALLMACHINE = 2750,
	GUMBALLMACHINEBROKE = 2751,
	PAPER = 2752,
	MACE = 2753,
	GENERICPOLE2 = 2754,
	XXXSTACY = 2755,
	WETFLOOR = 2756,
	BROOM = 2757,
	MOP = 2758,
	PIRATE1A = 2759,
	PIRATE4A = 2760,
	PIRATE2A = 2761,
	PIRATE5A = 2762,
	PIRATE3A = 2763,
	PIRATE6A = 2764,
	PIRATEHALF = 2765,
	CHESTOFGOLD = 2767,
	SIDEBOLT1 = 2768,
	FOODOBJECT1 = 2773,
	FOODOBJECT2 = 2774,
	FOODOBJECT3 = 2775,
	FOODOBJECT4 = 2776,
	FOODOBJECT5 = 2777,
	FOODOBJECT6 = 2778,
	FOODOBJECT7 = 2779,
	FOODOBJECT8 = 2780,
	FOODOBJECT9 = 2781,
	FOODOBJECT10 = 2782,
	FOODOBJECT11 = 2783,
	FOODOBJECT12 = 2784,
	FOODOBJECT13 = 2785,
	FOODOBJECT14 = 2786,
	FOODOBJECT15 = 2787,
	FOODOBJECT16 = 2788,
	FOODOBJECT17 = 2789,
	FOODOBJECT18 = 2790,
	FOODOBJECT19 = 2791,
	FOODOBJECT20 = 2792,
	HEADLAMP = 2793,
	SKINNEDCHICKEN = 2794,
	FEATHEREDCHICKEN = 2795,
	TAMPON = 2796,
	ROBOTDOG2 = 2797,
	JOLLYMEAL = 2800,
	DUKEBURGER = 2801,
	SHOPPINGCART = 2806,
	CANWITHSOMETHING2 = 2807,
	CANWITHSOMETHING3 = 2808,
	CANWITHSOMETHING4 = 2809,
	RRTILE2030 = 2030,
	RRTILE2831 = 2831,
	RRTILE2832 = 2832,
	RRTILE2842 = 2842,
	RRTILE2859 = 2859,
	RRTILE2876 = 2876,
	RRTILE2878 = 2878,
	RRTILE2879 = 2879,
	RRTILE2893 = 2893,
	RRTILE2894 = 2894,
	RRTILE2898 = 2898,
	RRTILE2899 = 2899,
	RRTILE2915 = 2915,
	RRTILE2940 = 2940,
	RRTILE2944 = 2944,
	RRTILE2945 = 2945,
	RRTILE2946 = 2946,
	RRTILE2947 = 2947,
	RRTILE2948 = 2948,
	RRTILE2949 = 2949,
	RRTILE2961 = 2961,
	RRTILE2970 = 2970,
	RRTILE2977 = 2977,
	RRTILE2978 = 2978,
	GLASS3 = 2983,
	BORNTOBEWILDSCREEN = 2985,
	BLIMP = 2989,
	RRTILE2990 = 2990,
	FEM9 = 2991,
	POOP = 2998,
	FRAMEEFFECT1 = 2999,
	PANNEL1 = 3003,
	PANNEL2 = 3004,
	PANNEL3 = 3005,
	BPANNEL1 = 3006,
	BPANNEL3 = 3007,
	SCREENBREAK14 = 3008,
	SCREENBREAK15 = 3009,
	SCREENBREAK19 = 3011,
	SCREENBREAK16 = 3013,
	SCREENBREAK17 = 3014,
	SCREENBREAK18 = 3015,
	W_TECHWALL11 = 3016,
	W_TECHWALL12 = 3017,
	W_TECHWALL13 = 3018,
	W_TECHWALL14 = 3019,
	W_TECHWALL5 = 3020,
	W_TECHWALL6 = 3022,
	W_TECHWALL7 = 3024,
	W_TECHWALL8 = 3026,
	W_TECHWALL9 = 3028,
	W_HITTECHWALL16 = 3030,
	W_HITTECHWALL10 = 3031,
	W_HITTECHWALL15 = 3033,
	W_MILKSHELF = 3035,
	W_MILKSHELFBROKE = 3036,
	PURPLELAVA = 3038,
	LAVABUBBLE = 3040,
	DUKECUTOUT = 3047,
	TARGET = 3049,
	GUNPOWDERBARREL = 3050,
	DUCK = 3051,
	HYDROPLANT = 3053,
	OCEANSPRITE1 = 3055,
	OCEANSPRITE2 = 3056,
	OCEANSPRITE3 = 3057,
	OCEANSPRITE4 = 3058,
	OCEANSPRITE5 = 3059,
	GENERICPOLE = 3061,
	CONE = 3062,
	HANGLIGHT = 3063,
	RRTILE3073 = 3073,
	RRTILE3083 = 3083,
	RRTILE3100 = 3100,
	RRTILE3114 = 3114,
	RRTILE3115 = 3115,
	RRTILE3116 = 3116,
	RRTILE3117 = 3117,
	RRTILE3120 = 3120,
	RRTILE3121 = 3121,
	RRTILE3122 = 3122,
	RRTILE3123 = 3123,
	RRTILE3124 = 3124,
	RRTILE3132 = 3132,
	RRTILE3139 = 3139,
	RRTILE3144 = 3144,
	RRTILE3152 = 3152,
	RRTILE3153 = 3153,
	RRTILE3155 = 3155,
	RRTILE3171 = 3171,
	RRTILE3172 = 3172,
	RRTILE3190 = 3190,
	RRTILE3191 = 3191,
	RRTILE3192 = 3192,
	RRTILE3195 = 3195,
	RRTILE3200 = 3200,
	RRTILE3201 = 3201,
	RRTILE3202 = 3202,
	RRTILE3203 = 3203,
	RRTILE3204 = 3204,
	RRTILE3205 = 3205,
	RRTILE3206 = 3206,
	RRTILE3207 = 3207,
	RRTILE3208 = 3208,
	RRTILE3209 = 3209,
	RRTILE3216 = 3216,
	RRTILE3218 = 3218,
	RRTILE3219 = 3219,
	RRTILE3232 = 3232,
	FEMPIC1 = 3239,
	FEMPIC2 = 3248,
	BLANKSCREEN = 3252,
	PODFEM1 = 3253,
	FEMPIC3 = 3257,
	FEMPIC4 = 3265,
	FEM1 = 3271,
	FEM2 = 3276,
	FEM3 = 3280,
	FEM5 = 3282,
	BLOODYPOLE = 3283,
	FEM4 = 3284,
	FEM6 = 3293,
	FEM6PAD = 3294,
	FEM8 = 3295,
	FEM7 = 3298,
	ORGANTIC = 3308,
	FIRSTGUN = 3328,
	FIRSTGUNRELOAD = 3336,
	KNEE = 3340,
	SHOTGUN = 3350,
	HANDTHROW = 3360,
	SHOTGUNSHELLS = 3372,
	SCUBAMASK = 3374,
	CHAINGUN = 3380,
	SHRINKER = 3384,
	CIRCLESTUCK = 3388,
	SPIT = 3390,
	GROWSPARK = 3395,
	SHRINKSPARK = 3400,
	RRTILE3410 = 3410,
	LUMBERBLADE = 3411,
	FREEZE = 3415,
	FIRELASER = 3420,
	BOWLINGBALLH = 3428,
	BOWLINGBALL = 3430,
	BOWLINGBALLSPRITE = 3437,
	POWDERH = 3438,
	RRTILE3440 = 3440,
	DEVISTATOR = 3445,
	RPGGUN = 3452,
	RRTILE3462 = 3462,
	OWHIP = 3471,
	UWHIP = 3475,
	RPGGUN2 = 3482,
	RRTILE3497 = 3497,
	RRTILE3498 = 3498,
	RRTILE3499 = 3499,
	RRTILE3500 = 3500,
	SLINGBLADE = 3510,
	RRTILE3584 = 3584,
	RRTILE3586 = 3586,
	RRTILE3587 = 3587,
	RRTILE3600 = 3600,
	RRTILE3631 = 3631,
	RRTILE3635 = 3635,
	RRTILE3637 = 3637,
	RRTILE3643 = 3643,
	RRTILE3647 = 3647,
	RRTILE3652 = 3652,
	RRTILE3653 = 3653,
	RRTILE3668 = 3668,
	RRTILE3671 = 3671,
	RRTILE3673 = 3673,
	RRTILE3684 = 3684,
	RRTILE3708 = 3708,
	RRTILE3714 = 3714,
	RRTILE3716 = 3716,
	RRTILE3720 = 3720,
	RRTILE3723 = 3723,
	RRTILE3725 = 3725,
	RRTILE3737 = 3737,
	RRTILE3754 = 3754,
	RRTILE3762 = 3762,
	RRTILE3763 = 3763,
	RRTILE3764 = 3764,
	RRTILE3765 = 3765,
	RRTILE3767 = 3767,
	RRTILE3773 = 3773,
	RRTILE3774 = 3774,
	RRTILE3793 = 3793,
	RRTILE3795 = 3795,
	RRTILE3804 = 3804,
	RRTILE3814 = 3814,
	RRTILE3815 = 3815,
	RRTILE3819 = 3819,
	BIGHOLE = 3822,
	RRTILE3827 = 3827,
	RRTILE3837 = 3837,
	APLAYERTOP = 3840,
	APLAYER = 3845,
	PLAYERONWATER = 3860,
	DUKELYINGDEAD = 3998,
	DUKEGUN = 4041,
	DUKETORSO = 4046,
	DUKELEG = 4055,
	FECES = 4802,
	DRONE = 4916,
	//RRTILE4956 = 4956,
	RECON = 4989,
	RRTILE5014 = 5014,
	RRTILE5016 = 5016,
	RRTILE5017 = 5017,
	RRTILE5018 = 5018,
	RRTILE5019 = 5019,
	RRTILE5020 = 5020,
	RRTILE5021 = 5021,
	RRTILE5022 = 5022,
	RRTILE5023 = 5023,
	RRTILE5024 = 5024,
	RRTILE5025 = 5025,
	RRTILE5026 = 5026,
	RRTILE5027 = 5027,
	RRTILE5029 = 5029,
	RRTILE5030 = 5030,
	RRTILE5031 = 5031,
	RRTILE5032 = 5032,
	RRTILE5033 = 5033,
	RRTILE5034 = 5034,
	RRTILE5035 = 5035,
	RRTILE5036 = 5036,
	RRTILE5037 = 5037,
	RRTILE5038 = 5038,
	RRTILE5039 = 5039,
	RRTILE5040 = 5040,
	RRTILE5041 = 5041,
	RRTILE5043 = 5043,
	RRTILE5044 = 5044,
	RRTILE5045 = 5045,
	RRTILE5046 = 5046,
	RRTILE5047 = 5047,
	RRTILE5048 = 5048,
	RRTILE5049 = 5049,
	RRTILE5050 = 5050,
	RRTILE5051 = 5051,
	RRTILE5052 = 5052,
	RRTILE5053 = 5053,
	RRTILE5054 = 5054,
	RRTILE5055 = 5055,
	RRTILE5056 = 5056,
	RRTILE5057 = 5057,
	RRTILE5058 = 5058,
	RRTILE5059 = 5059,
	RRTILE5061 = 5061,
	RRTILE5062 = 5062,
	RRTILE5063 = 5063,
	RRTILE5064 = 5064,
	RRTILE5065 = 5065,
	RRTILE5066 = 5066,
	RRTILE5067 = 5067,
	RRTILE5068 = 5068,
	RRTILE5069 = 5069,
	RRTILE5070 = 5070,
	RRTILE5071 = 5071,
	RRTILE5072 = 5072,
	RRTILE5073 = 5073,
	RRTILE5074 = 5074,
	RRTILE5075 = 5075,
	RRTILE5076 = 5076,
	RRTILE5077 = 5077,
	RRTILE5078 = 5078,
	RRTILE5079 = 5079,
	RRTILE5080 = 5080,
	RRTILE5081 = 5081,
	RRTILE5082 = 5082,
	RRTILE5083 = 5083,
	RRTILE5084 = 5084,
	RRTILE5085 = 5085,
	RRTILE5086 = 5086,
	RRTILE5087 = 5087,
	RRTILE5088 = 5088,
	RRTILE5090 = 5090,
	SHARK = 5501,
	FEM10 = 5581,
	TOUGHGAL = 5583,
	MAN = 5588,
	MAN2 = 5589,
	WOMAN = 5591,
	ATOMICHEALTH = 5595,
	RRTILE6144 = 6144,
	MOTOGUN = 7168,
	RRTILE7169 = 7169,
	MOTOHIT = 7170,
	BOATHIT = 7175,
	RRTILE7184 = 7184,
	RRTILE7190 = 7190,
	RRTILE7191 = 7191,
	RRTILE7213 = 7213,
	RRTILE7219 = 7219,
	EMPTYBIKE = 7220,
	EMPTYBOAT = 7233,
	RRTILE7424 = 7424,
	RRTILE7430 = 7430,
	RRTILE7433 = 7433,
	RRTILE7441 = 7441,
	RRTILE7547 = 7547,
	RRTILE7467 = 7467,
	RRTILE7469 = 7469,
	RRTILE7470 = 7470,
	RRTILE7475 = 7475,
	RRTILE7478 = 7478,
	RRTILE7505 = 7505,
	RRTILE7506 = 7506,
	RRTILE7534 = 7534,
	RRTILE7540 = 7540,
	RRTILE7533 = 7533,
	RRTILE7545 = 7545,
	RRTILE7552 = 7552,
	RRTILE7553 = 7553,
	RRTILE7554 = 7554,
	RRTILE7555 = 7555,
	RRTILE7557 = 7557,
	RRTILE7558 = 7558,
	RRTILE7559 = 7559,
	RRTILE7561 = 7561,
	RRTILE7566 = 7566,
	RRTILE7568 = 7568,
	RRTILE7574 = 7574,
	RRTILE7575 = 7575,
	RRTILE7576 = 7576,
	RRTILE7578 = 7578,
	RRTILE7579 = 7579,
	RRTILE7580 = 7580,
	RRTILE7595 = 7595,
	RRTILE7629 = 7629,
	RRTILE7636 = 7636,
	RRTILE7638 = 7638,
	RRTILE7640 = 7640,
	RRTILE7644 = 7644,
	RRTILE7646 = 7646,
	RRTILE7648 = 7648,
	RRTILE7650 = 7650,
	RRTILE7653 = 7653,
	RRTILE7655 = 7655,
	RRTILE7657 = 7657,
	RRTILE7659 = 7659,
	RRTILE7691 = 7691,
	RRTILE7694 = 7694,
	RRTILE7696 = 7696,
	RRTILE7697 = 7697,
	RRTILE7700 = 7700,
	RRTILE7702 = 7702,
	RRTILE7704 = 7704,
	RRTILE7705 = 7705,
	RRTILE7711 = 7711,
	RRTILE7716 = 7716,
	RRTILE7756 = 7756,
	RRTILE7768 = 7768,
	RRTILE7806 = 7806,
	RRTILE7820 = 7820,
	RRTILE7859 = 7859,
	RRTILE7870 = 7870,
	RRTILE7873 = 7873,
	RRTILE7875 = 7875,
	RRTILE7876 = 7876,
	RRTILE7879 = 7879,
	RRTILE7881 = 7881,
	RRTILE7883 = 7883,
	RRTILE7885 = 7885,
	RRTILE7886 = 7886,
	RRTILE7887 = 7887,
	RRTILE7888 = 7888,
	RRTILE7889 = 7889,
	RRTILE7890 = 7890,
	RRTILE7900 = 7900,
	RRTILE7901 = 7901,
	RRTILE7906 = 7906,
	RRTILE7912 = 7912,
	RRTILE7913 = 7913,
	RRTILE7936 = 7936,
	RRTILE8047 = 8047,
	MULTISWITCH2 = 8048,
	RRTILE8059 = 8059,
	RRTILE8060 = 8060,
	RRTILE8063 = 8063,
	RRTILE8067 = 8067,
	RRTILE8076 = 8076,
	RRTILE8094 = 8094,
	RRTILE8096 = 8096,
	RRTILE8099 = 8099,
	RRTILE8106 = 8106,
	RRTILE8162 = 8162,
	RRTILE8163 = 8163,
	RRTILE8164 = 8164,
	RRTILE8165 = 8165,
	RRTILE8166 = 8166,
	RRTILE8167 = 8167,
	RRTILE8168 = 8168,
	RRTILE8192 = 8192,
	RRTILE8193 = 8193,
	RRTILE8215 = 8215,
	RRTILE8216 = 8216,
	RRTILE8217 = 8217,
	RRTILE8218 = 8218,
	RRTILE8220 = 8220,
	RRTILE8221 = 8221,
	RRTILE8222 = 8222,
	RRTILE8223 = 8223,
	RRTILE8224 = 8224,
	RRTILE8227 = 8227,
	RRTILE8312 = 8312,
	RRTILE8370 = 8370,
	RRTILE8371 = 8371,
	RRTILE8372 = 8372,
	RRTILE8373 = 8373,
	RRTILE8379 = 8379,
	RRTILE8380 = 8380,
	RRTILE8385 = 8385,
	RRTILE8386 = 8386,
	RRTILE8387 = 8387,
	RRTILE8388 = 8388,
	RRTILE8389 = 8389,
	RRTILE8390 = 8390,
	RRTILE8391 = 8391,
	RRTILE8392 = 8392,
	RRTILE8394 = 8394,
	RRTILE8395 = 8395,
	RRTILE8396 = 8396,
	RRTILE8397 = 8397,
	RRTILE8398 = 8398,
	RRTILE8399 = 8399,
	RRTILE8423 = 8423,
	RRTILE8448 = 8448,
	RRTILE8450 = 8450,
	BOATAMMO = 8460,
	RRTILE8461 = 8461,
	RRTILE8462 = 8462,
	RRTILE8464 = 8464,
	RRTILE8475 = 8475,
	RRTILE8487 = 8487,
	RRTILE8488 = 8488,
	RRTILE8489 = 8489,
	RRTILE8490 = 8490,
	RRTILE8496 = 8496,
	RRTILE8497 = 8497,
	RRTILE8498 = 8498,
	RRTILE8499 = 8499,
	RRTILE8503 = 8503,
	RRTILE8525 = 8525,
	RRTILE8537 = 8537,
	RRTILE8565 = 8565,
	RRTILE8567 = 8567,
	RRTILE8568 = 8568,
	RRTILE8569 = 8569,
	RRTILE8570 = 8570,
	RRTILE8571 = 8571,
	RRTILE8579 = 8579,
	RRTILE8588 = 8588,
	RRTILE8589 = 8589,
	RRTILE8590 = 8590,
	RRTILE8591 = 8591,
	RRTILE8592 = 8592,
	RRTILE8593 = 8593,
	RRTILE8594 = 8594,
	RRTILE8595 = 8595,
	RRTILE8596 = 8596,
	RRTILE8598 = 8598 = ,
	RRTILE8605 = 8605,
	RRTILE8608 = 8608,
	RRTILE8609 = 8609,
	RRTILE8611 = 8611,
	RRTILE8617 = 8617,
	RRTILE8618 = 8618,
	RRTILE8620 = 8620,
	RRTILE8621 = 8621,
	RRTILE8622 = 8622,
	RRTILE8623 = 8623,
	RRTILE8640 = 8640,
	RRTILE8651 = 8651,
	RRTILE8660 = 8660,
	RRTILE8677 = 8677,
	RRTILE8679 = 8679,
	RRTILE8680 = 8680,
	RRTILE8681 = 8681,
	RRTILE8682 = 8682,
	RRTILE8683 = 8683,
	RRTILE8704 = 8704,
	,
	,
	// = RR = bad = guys,
	BOSS1 = 4477,
	BOSS2 = 4557,
	BOSS3 = 4607,
	BOSS4 = 4221,
	,
	,
	BOULDER = 256,
	BOULDER1 = 264,
	TORNADO = 1930,
	CHEERBOMB = 3464,
	CHEERBLADE = 3460,
	DOGATTACK = 4060,
	BILLYWALK = 4096,
	BILLYDIE = 4137,
	BILLYCOCK = 4147,
	BILLYRAY = 4162,
	BILLYRAYSTAYPUT = 4163,
	BILLYBUT = 4188,
	BILLYSCRATCH = 4191,
	BILLYSNIFF = 4195,
	BILLYWOUND = 4202,
	BILLYGORE = 4228,
	BILLYJIBA = 4235,
	BILLYJIBB = 4244,
	BRAYSNIPER = 4249,
	DOGRUN = 4260,
	DOGDIE = 4295,
	DOGDEAD = 4303,
	DOGBARK = 4305,
	LTH = 4352,
	LTHSTRAFE = 4395,
	HULKHANG = 4409,
	HULKHANGDEAD = 4410,
	HULKJUMP = 4429,
	LTHLOAD = 4430,
	LTHDIE = 4456,
	BUBBASCRATCH = 4464,
	BUBBANOSE = 4476,
	BUBBAPISS = 4487,
	BUBBASTAND = 4504,
	BUBBAOUCH = 4506,
	BUBBADIE = 4513,
	BUBBADEAD = 4523,
	HULK = 4649,
	HULKSTAYPUT = 4650,
	HULKA = 4651,
	HULKB = 4652,
	HULKC = 4653,
	HULKJIBA = 4748,
	HULKJIBB = 4753,
	HULKJIBC = 4758,
	SBSWIPE = 4770,
	SBPAIN = 4810,
	SBDIE = 4820,
	HEN = 4861,
	HENSTAYPUT = 4862,
	HENSTAND = 4897,
	PIG = 4945,
	PIGSTAYPUT = 4946,
	PIGEAT = 4983,
	SBMOVE = 5015,
	SBSPIT = 5050,
	SBDIP = 5085,
	MINION = 5120,
	MINIONSTAYPUT = 5121,
	,
	#ifdef = RRRA,
	# = define = UFO1 = 5260,
	#else,
	# = define = UFO1 = 5270,
	#endif,
	,
	UFO2 = 5274,
	UFO3 = 5278,
	UFO4 = 5282,
	UFO5 = 5286,
	MINJIBA = 5290,
	MINJIBB = 5295,
	MINJIBC = 5300,
	COW = 5317,
	COOT = 5376,
	COOTSTAYPUT = 5377,
	COOTSHOOT = 5411,
	COOTDIE = 5437,
	COOTDUCK = 5481,
	COOTPAIN = 5548,
	COOTTRANS = 5568,
	COOTGETUP = 5579,
	COOTJIBA = 5602,
	COOTJIBB = 5607,
	COOTJIBC = 5616,
	VIXEN = 5635,
	VIXENPAIN = 5675,
	VIXENDIE = 5710,
	VIXENSHOOT = 5720,
	VIXENWDN = 5740,
	VIXENWUP = 5775,
	VIXENKICK = 5805,
	VIXENTELE = 5845,
	VIXENTEAT = 5851,
	BIKEJIBA = 5872,
	BIKEJIBB = 5877,
	BIKEJIBC = 5882,
	BIKERB = 5890,
	BIKERBV2 = 5891,
	BIKER = 5995,
	BIKERJIBA = 6112,
	BIKERJIBB = 6117,
	BIKERJIBC = 6121,
	BIKERJIBD = 6127,
	MAKEOUT = 6225,
	CHEERB = 6401,
	CHEER = 6658,
	CHEERSTAYPUT = 6659,
	CHEERJIBA = 7000,
	CHEERJIBB = 7005,
	CHEERJIBC = 7010,
	CHEERJIBD = 7015,
	FBOATJIBA = 7020,
	FBOATJIBB = 7025,
	COOTPLAY = 7030,
	BILLYPLAY = 7035,
	MINIONBOAT = 7192,
	HULKBOAT = 7199,
	CHEERBOAT = 7206,
	RRTILE7274 = 7274,
	RABBIT = 7280,
	RABBITJIBA = 7387,
	RABBITJIBB = 7392,
	RABBITJIBC = 7397,
	ROCK = 8035,
	ROCK2 = 8036,
	MAMACLOUD = 8663,
	MAMA = 8705,
	MAMAJIBA = 8890,
	MAMAJIBB = 8895,
};