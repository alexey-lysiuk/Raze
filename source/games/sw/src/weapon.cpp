//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "misc.h"
#include "interpso.h"
#include "tags.h"
#include "break.h"
#include "network.h"
#include "pal.h"

#include "ai.h"
#include "weapon.h"
#include "sector.h"
#include "sprite.h"
#include "misc.h"
#include "player.h"
#include "gstrings.h"

BEGIN_SW_NS

struct MISSILE_PLACEMENT
{
    int dist_over, dist_out;
    short ang;
};


void SpawnZombie2(DSWActor*);
Collision move_ground_missile(DSWActor* actor, int xchange, int ychange, int ceildist, int flordist, uint32_t cliptype, int numtics);
void DoPlayerBeginDie(PLAYER*);
void VehicleSetSmoke(SECTOR_OBJECT* sop, ANIMATOR* animator);
ANIMATOR DoBettyBeginDeath;
ANIMATOR DoSkullBeginDeath;
ANIMATOR DoRipperGrow;

//
// Damage Amounts defined in damage.h
//
#define DAMAGE_TABLE
DAMAGE_DATA DamageData[] =
{
#include "damage.h"
};
#undef  DAMAGE_TABLE

FOOT_TYPE FootMode=WATER_FOOT;
bool left_foot = false;
int FinishTimer = 0;

// This is how many bullet shells have been spawned since the beginning of the game.
int ShellCount = 0;

//int Zombies = 0;
int StarQueueHead=0;
TObjPtr<DSWActor*> StarQueue[MAX_STAR_QUEUE];
int HoleQueueHead=0;
TObjPtr<DSWActor*> HoleQueue[MAX_HOLE_QUEUE];
int WallBloodQueueHead=0;
TObjPtr<DSWActor*> WallBloodQueue[MAX_WALLBLOOD_QUEUE];
int FloorBloodQueueHead=0;
TObjPtr<DSWActor*> FloorBloodQueue[MAX_FLOORBLOOD_QUEUE];
int GenericQueueHead=0;
TObjPtr<DSWActor*> GenericQueue[MAX_GENERIC_QUEUE];
int LoWangsQueueHead=0;
TObjPtr<DSWActor*> LoWangsQueue[MAX_LOWANGS_QUEUE];
void SpawnBreakStaticFlames(DSWActor* actor);

bool GlobalSkipZrange = false;

int WeaponIsAmmo = BIT(WPN_STAR) | BIT(WPN_SWORD) | BIT(WPN_MINE) | BIT(WPN_FIST);

short target_ang;

ANIMATOR DoStar;
ANIMATOR DoCrossBolt;
ANIMATOR DoSuicide, DoUziSmoke;
ANIMATOR DoShrapJumpFall;
ANIMATOR DoFastShrapJumpFall;

int SpawnSmokePuff(DSWActor* actor);
bool WarpToUnderwater(sectortype** sect, int *x, int *y, int *z);
bool WarpToSurface(sectortype** sect, int *x, int *y, int *z);

inline bool WarpToSurface(DVector3& pos, sectortype** sect)
{
    vec3_t vv = { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint) };
    auto act = WarpToSurface(sect, &vv.X, &vv.Y, &vv.Z);
    pos = { vv.X * inttoworld, vv.Y * inttoworld, vv.Z * zinttoworld };
    return act;
}

inline bool WarpToUnderwater(DVector3& pos, sectortype** sect)
{
    vec3_t vv = { int(pos.X * worldtoint), int(pos.Y * worldtoint), int(pos.Z * zworldtoint) };
    auto act = WarpToUnderwater(sect, &vv.X, &vv.Y, &vv.Z);
    pos = { vv.X * inttoworld, vv.Y * inttoworld, vv.Z * zinttoworld };
    return act;
}

bool TestDontStickSector(sectortype* hit_sect);
ANIMATOR SpawnShrapX;
bool WeaponMoveHit(DSWActor* actor);
void SpawnMidSplash(DSWActor* actor);

int SopDamage(SECTOR_OBJECT* sop,short amt);
int SopCheckKill(SECTOR_OBJECT* sop);
int QueueStar(DSWActor*);
int DoBlurExtend(DSWActor* actor, int16_t interval, int16_t blur_num);
int SpawnDemonFist(DSWActor*);
void SpawnTankShellExp(DSWActor*);
void SpawnMicroExp(DSWActor*);
void SpawnExpZadjust(DSWActor* actor, DSWActor* expActor, int upper_zsize, int lower_zsize);
int BulletHitSprite(DSWActor* actor, DSWActor* hitActor, const DVector3& pos, short ID);
int SpawnSplashXY(int hit_x,int hit_y,int hit_z,sectortype*);
DSWActor* SpawnBoatSparks(PLAYER* pp, sectortype* hit_sect, walltype* hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang);

short StatDamageList[STAT_DAMAGE_LIST_SIZE] =
{
    STAT_SO_SP_CHILD,
    STAT_ENEMY,
    STAT_ENEMY_SKIP4,
    STAT_PLAYER0,
    STAT_PLAYER1,
    STAT_PLAYER2,
    STAT_PLAYER3,
    STAT_PLAYER4,
    STAT_PLAYER5,
    STAT_PLAYER6,
    STAT_PLAYER7,
    STAT_PLAYER_UNDER0,
    STAT_PLAYER_UNDER1,
    STAT_PLAYER_UNDER2,
    STAT_PLAYER_UNDER3,
    STAT_PLAYER_UNDER4,
    STAT_PLAYER_UNDER5,
    STAT_PLAYER_UNDER6,
    STAT_PLAYER_UNDER7,
    // MINE MUST BE LAST
    STAT_MINE_STUCK
};

//////////////////////
//
// SPECIAL STATES
//
//////////////////////

// state for sprites that are not restored
STATE s_NotRestored[] =
{
    {2323, 100, NullAnimator, &s_NotRestored[0]}
};

STATE s_Suicide[] =
{
    {1, 100, DoSuicide, &s_Suicide[0]}
};

STATE s_DeadLoWang[] =
{
    {1160, 100, NullAnimator, &s_DeadLoWang[0]},
};

//////////////////////
//
// BREAKABLE STATES
//
//////////////////////

ANIMATOR DoDefaultStat;

#define BREAK_LIGHT_RATE 18
STATE s_BreakLight[] =
{
    {BREAK_LIGHT_ANIM + 0, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[1]},
    {BREAK_LIGHT_ANIM + 1, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[2]},
    {BREAK_LIGHT_ANIM + 2, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[3]},
    {BREAK_LIGHT_ANIM + 3, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[4]},
    {BREAK_LIGHT_ANIM + 4, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[5]},
    {BREAK_LIGHT_ANIM + 5, BREAK_LIGHT_RATE, NullAnimator, &s_BreakLight[5]}
};

#define BREAK_BARREL_RATE 18
STATE s_BreakBarrel[] =
{
    {BREAK_BARREL + 4, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[1]},
    {BREAK_BARREL + 5, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[2]},
    {BREAK_BARREL + 6, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[3]},
    {BREAK_BARREL + 7, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[4]},
    {BREAK_BARREL + 8, BREAK_BARREL_RATE, NullAnimator, &s_BreakBarrel[5]},
    {BREAK_BARREL + 9, BREAK_BARREL_RATE, DoDefaultStat, &s_BreakBarrel[5]},
};

#define BREAK_PEDISTAL_RATE 28
STATE s_BreakPedistal[] =
{
    {BREAK_PEDISTAL + 1, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[1]},
    {BREAK_PEDISTAL + 2, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[2]},
    {BREAK_PEDISTAL + 3, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[3]},
    {BREAK_PEDISTAL + 4, BREAK_PEDISTAL_RATE, NullAnimator, &s_BreakPedistal[3]},
};

#define BREAK_BOTTLE1_RATE 18
STATE s_BreakBottle1[] =
{
    {BREAK_BOTTLE1 + 1, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[1]},
    {BREAK_BOTTLE1 + 2, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[2]},
    {BREAK_BOTTLE1 + 3, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[3]},
    {BREAK_BOTTLE1 + 4, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[4]},
    {BREAK_BOTTLE1 + 5, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[5]},
    {BREAK_BOTTLE1 + 6, BREAK_BOTTLE1_RATE, NullAnimator, &s_BreakBottle1[5]},
};

#define BREAK_BOTTLE2_RATE 18
STATE s_BreakBottle2[] =
{
    {BREAK_BOTTLE2 + 1, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[1]},
    {BREAK_BOTTLE2 + 2, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[2]},
    {BREAK_BOTTLE2 + 3, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[3]},
    {BREAK_BOTTLE2 + 4, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[4]},
    {BREAK_BOTTLE2 + 5, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[5]},
    {BREAK_BOTTLE2 + 6, BREAK_BOTTLE2_RATE, NullAnimator, &s_BreakBottle2[5]},
};

#define PUFF_RATE 8
ANIMATOR DoPuff;
STATE s_Puff[] =
{
    {PUFF + 0, PUFF_RATE, DoPuff, &s_Puff[1]},
    {PUFF + 1, PUFF_RATE, DoPuff, &s_Puff[2]},
    {PUFF + 2, PUFF_RATE, DoPuff, &s_Puff[3]},
    {PUFF + 3, PUFF_RATE, DoPuff, &s_Puff[4]},
    {PUFF + 4, PUFF_RATE, DoPuff, &s_Puff[5]},
    {PUFF + 5, PUFF_RATE, DoPuff, &s_Puff[6]},
    {PUFF + 5, 100, DoSuicide, &s_Puff[0]}
};

#define RAIL_PUFF_R0 3969
#define RAIL_PUFF_R1 3985
#define RAIL_PUFF_R2 4001
#define RAIL_PUFF_RATE 6

ANIMATOR DoRailPuff;

STATE s_RailPuff[3][17] =
{
    {
        {RAIL_PUFF_R0 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][1]},
        {RAIL_PUFF_R0 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][2]},
        {RAIL_PUFF_R0 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][3]},
        {RAIL_PUFF_R0 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][4]},
        {RAIL_PUFF_R0 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][5]},
        {RAIL_PUFF_R0 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][6]},
        {RAIL_PUFF_R0 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][7]},
        {RAIL_PUFF_R0 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][8]},
        {RAIL_PUFF_R0 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][9]},
        {RAIL_PUFF_R0 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][10]},
        {RAIL_PUFF_R0 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][11]},
        {RAIL_PUFF_R0 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][12]},
        {RAIL_PUFF_R0 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][13]},
        {RAIL_PUFF_R0 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][14]},
        {RAIL_PUFF_R0 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][15]},
        {RAIL_PUFF_R0 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[0][16]},
        {RAIL_PUFF_R0 + 15, 100, DoSuicide, &s_RailPuff[0][0]},
    },
    {
        {RAIL_PUFF_R1 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][1]},
        {RAIL_PUFF_R1 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][2]},
        {RAIL_PUFF_R1 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][3]},
        {RAIL_PUFF_R1 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][4]},
        {RAIL_PUFF_R1 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][5]},
        {RAIL_PUFF_R1 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][6]},
        {RAIL_PUFF_R1 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][7]},
        {RAIL_PUFF_R1 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][8]},
        {RAIL_PUFF_R1 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][9]},
        {RAIL_PUFF_R1 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][10]},
        {RAIL_PUFF_R1 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][11]},
        {RAIL_PUFF_R1 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][12]},
        {RAIL_PUFF_R1 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][13]},
        {RAIL_PUFF_R1 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][14]},
        {RAIL_PUFF_R1 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][15]},
        {RAIL_PUFF_R1 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[1][16]},
        {RAIL_PUFF_R1 + 15, 100, DoSuicide, &s_RailPuff[1][0]},
    },
    {
        {RAIL_PUFF_R2 + 0, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][1]},
        {RAIL_PUFF_R2 + 1, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][2]},
        {RAIL_PUFF_R2 + 2, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][3]},
        {RAIL_PUFF_R2 + 3, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][4]},
        {RAIL_PUFF_R2 + 4, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][5]},
        {RAIL_PUFF_R2 + 5, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][6]},
        {RAIL_PUFF_R2 + 6, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][7]},
        {RAIL_PUFF_R2 + 7, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][8]},
        {RAIL_PUFF_R2 + 8, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][9]},
        {RAIL_PUFF_R2 + 9, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][10]},
        {RAIL_PUFF_R2 + 10, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][11]},
        {RAIL_PUFF_R2 + 11, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][12]},
        {RAIL_PUFF_R2 + 12, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][13]},
        {RAIL_PUFF_R2 + 13, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][14]},
        {RAIL_PUFF_R2 + 14, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][15]},
        {RAIL_PUFF_R2 + 15, RAIL_PUFF_RATE, DoRailPuff, &s_RailPuff[2][16]},
        {RAIL_PUFF_R2 + 15, 100, DoSuicide, &s_RailPuff[2][0]},
    }
};


STATE* sg_RailPuff[] =
{
    &s_RailPuff[0][0],
    &s_RailPuff[1][0],
    &s_RailPuff[2][0],
    &s_RailPuff[1][0],
    &s_RailPuff[0][0]
};

#define LASER_PUFF 3201
#define LASER_PUFF_RATE 8
STATE s_LaserPuff[] =
{
    {LASER_PUFF + 0, LASER_PUFF_RATE, nullptr, &s_LaserPuff[1]},
    //{LASER_PUFF + 1, LASER_PUFF_RATE, nullptr, &s_LaserPuff[2]},
    //{LASER_PUFF + 2, LASER_PUFF_RATE, nullptr, &s_LaserPuff[3]},
    {LASER_PUFF + 0, 100, DoSuicide, &s_LaserPuff[0]}
};

#define TRACER 3201
#define TRACER_RATE 6
ANIMATOR DoTracer;
STATE s_Tracer[] =
{
    {TRACER + 0, TRACER_RATE, DoTracer, &s_Tracer[1]},
    {TRACER + 1, TRACER_RATE, DoTracer, &s_Tracer[2]},
    {TRACER + 2, TRACER_RATE, DoTracer, &s_Tracer[3]},
    {TRACER + 3, TRACER_RATE, DoTracer, &s_Tracer[4]},
    {TRACER + 4, TRACER_RATE, DoTracer, &s_Tracer[5]},
    {TRACER + 5, TRACER_RATE, DoTracer, &s_Tracer[0]}
};

#define EMP 2058
#define EMP_RATE 6
ANIMATOR DoEMP;
STATE s_EMP[] =
{
    {EMP + 0, EMP_RATE, DoEMP, &s_EMP[1]},
    {EMP + 1, EMP_RATE, DoEMP, &s_EMP[2]},
    {EMP + 2, EMP_RATE, DoEMP, &s_EMP[0]}
};

ANIMATOR DoEMPBurst;
STATE s_EMPBurst[] =
{
    {EMP + 0, EMP_RATE, DoEMPBurst, &s_EMPBurst[1]},
    {EMP + 1, EMP_RATE, DoEMPBurst, &s_EMPBurst[2]},
    {EMP + 2, EMP_RATE, DoEMPBurst, &s_EMPBurst[0]}
};

STATE s_EMPShrap[] =
{
    {EMP + 0, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[1]},
    {EMP + 1, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[2]},
    {EMP + 2, EMP_RATE, DoFastShrapJumpFall, &s_EMPShrap[0]},
};


#define TANK_SHELL 3201
#define TANK_SHELL_RATE 6
ANIMATOR DoTankShell;
STATE s_TankShell[] =
{
    {TRACER + 0, 200, DoTankShell, &s_TankShell[0]}
};

ANIMATOR DoVehicleSmoke;
#define VEHICLE_SMOKE_RATE 18
STATE s_VehicleSmoke[] =
{
    {PUFF + 0, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[1]},
    {PUFF + 1, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[2]},
    {PUFF + 2, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[3]},
    {PUFF + 3, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[4]},
    {PUFF + 4, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[5]},
    {PUFF + 5, VEHICLE_SMOKE_RATE, DoVehicleSmoke, &s_VehicleSmoke[6]},
    {PUFF + 5, 100, DoSuicide, &s_VehicleSmoke[6]}
};

ANIMATOR DoWaterSmoke, SpawnWaterSmoke;
#define WATER_SMOKE_RATE 18
STATE s_WaterSmoke[] =
{
    {PUFF + 0, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[1]},
    {PUFF + 1, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[2]},
    {PUFF + 2, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[3]},
    {PUFF + 3, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[4]},
    {PUFF + 4, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[5]},
    {PUFF + 5, WATER_SMOKE_RATE, DoWaterSmoke, &s_WaterSmoke[6]},
    {PUFF + 5, 100, DoSuicide, &s_WaterSmoke[6]}
};

#define UZI_SPARK_REPEAT 24
#define UZI_SMOKE_REPEAT 24 // Was 32
#define UZI_SMOKE_RATE 16 // Was 9
STATE s_UziSmoke[] =
{
    {UZI_SMOKE + 0, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[1]},
    {UZI_SMOKE + 1, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[2]},
    {UZI_SMOKE + 2, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[3]},
    {UZI_SMOKE + 3, UZI_SMOKE_RATE, DoUziSmoke, &s_UziSmoke[4]},
    {UZI_SMOKE + 3, 100, DoSuicide, &s_UziSmoke[0]},
};

#define SHOTGUN_SMOKE_RATE 16
#define SHOTGUN_SMOKE_REPEAT 18 // Was 32
#define SHOTGUN_SMOKE UZI_SMOKE+1
ANIMATOR DoShotgunSmoke;
STATE s_ShotgunSmoke[] =
{
    {UZI_SMOKE + 0, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[1]},
    {UZI_SMOKE + 1, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[2]},
    {UZI_SMOKE + 2, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[3]},
    {UZI_SMOKE + 3, SHOTGUN_SMOKE_RATE, DoShotgunSmoke, &s_ShotgunSmoke[4]},
    {UZI_SMOKE + 3, 100, DoSuicide, &s_ShotgunSmoke[0]},
};

#define UZI_BULLET_RATE 100
#define UZI_BULLET 717 // actually a bubble
ANIMATOR DoUziBullet;

STATE s_UziBullet[] =
{
    {UZI_BULLET + 0, UZI_BULLET_RATE, DoUziBullet, &s_UziBullet[0]},
};

#define UZI_SPARK_RATE 8

STATE s_UziSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, NullAnimator, &s_UziSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_UziSpark[0]},
};

STATE s_UziPowerSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, DoUziSmoke, &s_UziSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_UziSpark[0]},
};

#define BUBBLE 716
#define BUBBLE_RATE 100
ANIMATOR DoBubble;

STATE s_Bubble[] =
{
    {BUBBLE + 0, BUBBLE_RATE, DoBubble, &s_Bubble[0]}
};


//#define SPLASH 2190
#define SPLASH 772
#define SPLASH_RATE 10

STATE s_Splash[] =
{
    {SPLASH + 0, SPLASH_RATE, NullAnimator, &s_Splash[1]},
    {SPLASH + 1, SPLASH_RATE, NullAnimator, &s_Splash[2]},
    {SPLASH + 2, SPLASH_RATE, NullAnimator, &s_Splash[3]},
    {SPLASH + 3, SPLASH_RATE, NullAnimator, &s_Splash[4]},
    {SPLASH + 4, SPLASH_RATE, NullAnimator, &s_Splash[5]},
    {SPLASH + 4, 100, DoSuicide, &s_Splash[0]}
};

#define CROSSBOLT 2230
#define CROSSBOLT_RATE 100
STATE s_CrossBolt[5][1] =
{
    {
        {CROSSBOLT + 0, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[0][0]},
    },
    {
        {CROSSBOLT + 2, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[1][0]},
    },
    {
        {CROSSBOLT + 3, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[2][0]},
    },
    {
        {CROSSBOLT + 4, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[3][0]},
    },
    {
        {CROSSBOLT + 1, CROSSBOLT_RATE, DoCrossBolt, &s_CrossBolt[4][0]},
    }
};

STATE* sg_CrossBolt[] =
{
    &s_CrossBolt[0][0],
    &s_CrossBolt[1][0],
    &s_CrossBolt[2][0],
    &s_CrossBolt[3][0],
    &s_CrossBolt[4][0]
};

#undef STAR
#define STAR 2102
#define STAR_RATE 6
STATE s_Star[] =
{
    {STAR + 0, STAR_RATE, DoStar, &s_Star[1]},
    {STAR + 1, STAR_RATE, DoStar, &s_Star[2]},
    {STAR + 2, STAR_RATE, DoStar, &s_Star[3]},
    {STAR + 3, STAR_RATE, DoStar, &s_Star[0]}
};

STATE s_StarStuck[] =
{
    {STAR + 0, STAR_RATE, NullAnimator, &s_StarStuck[0]},
};

#define STAR_DOWN 2066
STATE s_StarDown[] =
{
    {STAR_DOWN + 0, STAR_RATE, DoStar, &s_StarDown[1]},
    {STAR_DOWN + 1, STAR_RATE, DoStar, &s_StarDown[2]},
    {STAR_DOWN + 2, STAR_RATE, DoStar, &s_StarDown[3]},
    {STAR_DOWN + 3, STAR_RATE, DoStar, &s_StarDown[0]}
};

STATE s_StarDownStuck[] =
{
    {STAR + 0, STAR_RATE, NullAnimator, &s_StarDownStuck[0]},
};

//////////////////////
//
// LAVA BOSS
//
//////////////////////

#define LAVA_BOULDER_RATE 6
ANIMATOR DoLavaBoulder,DoShrapDamage,DoVulcanBoulder;

STATE s_LavaBoulder[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoLavaBoulder, &s_LavaBoulder[0]},
};

#define LAVA_SHARD (LAVA_BOULDER+1)

STATE s_LavaShard[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoShrapDamage, &s_LavaShard[0]},
};

STATE s_VulcanBoulder[] =
{
    {LAVA_BOULDER + 1, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[1]},
    {LAVA_BOULDER + 2, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[2]},
    {LAVA_BOULDER + 3, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[3]},
    {LAVA_BOULDER + 4, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[4]},
    {LAVA_BOULDER + 5, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[5]},
    {LAVA_BOULDER + 6, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[6]},
    {LAVA_BOULDER + 7, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[7]},
    {LAVA_BOULDER + 8, LAVA_BOULDER_RATE, DoVulcanBoulder, &s_VulcanBoulder[0]},
};

//////////////////////
//
// GRENADE
//
//////////////////////

#if 0
ANIMATOR DoGrenade;
#undef GRENADE
#define GRENADE 5000
#define GRENADE_RATE 8

STATE s_Grenade[] =
{
    {FIREBALL + 0, GRENADE_RATE, DoGrenade, &s_Grenade[1]},
    {FIREBALL + 1, GRENADE_RATE, DoGrenade, &s_Grenade[2]},
    {FIREBALL + 2, GRENADE_RATE, DoGrenade, &s_Grenade[3]},
    {FIREBALL + 3, GRENADE_RATE, DoGrenade, &s_Grenade[0]}
};
#else
#define GRENADE_FRAMES 1
#define GRENADE_R0 2110
#define GRENADE_R1 GRENADE_R0 + (GRENADE_FRAMES * 1)
#define GRENADE_R2 GRENADE_R0 + (GRENADE_FRAMES * 2)
#define GRENADE_R3 GRENADE_R0 + (GRENADE_FRAMES * 3)
#define GRENADE_R4 GRENADE_R0 + (GRENADE_FRAMES * 4)

#undef GRENADE
#define GRENADE GRENADE_R0
#define GRENADE_RATE 8
ANIMATOR DoGrenade;

STATE s_Grenade[5][1] =
{
    {
        {GRENADE_R0 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[0][0]},
    },
    {
        {GRENADE_R1 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[1][0]},
    },
    {
        {GRENADE_R2 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[2][0]},
    },
    {
        {GRENADE_R3 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[3][0]},
    },
    {
        {GRENADE_R4 + 0, GRENADE_RATE, DoGrenade, &s_Grenade[4][0]},
    }
};


STATE* sg_Grenade[] =
{
    &s_Grenade[0][0],
    &s_Grenade[1][0],
    &s_Grenade[2][0],
    &s_Grenade[3][0],
    &s_Grenade[4][0]
};
#endif

//////////////////////
//
// MINE
//
//////////////////////

ANIMATOR DoMine,DoMineStuck;
#undef MINE
#define MINE 2223
#define MINE_SHRAP 5011
#define MINE_RATE 16

STATE s_MineStuck[] =
{
    {MINE + 0, MINE_RATE, DoMineStuck, &s_MineStuck[0]},
};

STATE s_Mine[] =
{
    {MINE + 0, MINE_RATE, DoMine, &s_Mine[1]},
    {MINE + 1, MINE_RATE, DoMine, &s_Mine[0]},
};

ANIMATOR DoMineSpark;
STATE s_MineSpark[] =
{
    {UZI_SPARK + 0, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[1]},
    {UZI_SPARK + 1, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[2]},
    {UZI_SPARK + 2, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[3]},
    {UZI_SPARK + 3, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[4]},
    {UZI_SPARK + 4, UZI_SPARK_RATE, DoMineSpark, &s_MineSpark[5]},
    {UZI_SPARK + 4, 100, DoSuicide, &s_MineSpark[0]},
};

//////////////////////
//
// METEOR
//
//////////////////////

#define METEOR_R0 2098
#define METEOR_R1 2090
#define METEOR_R2 2094
#define METEOR_R3 2090
#define METEOR_R4 2098


#define METEOR STAR
#define METEOR_RATE 8
ANIMATOR DoMeteor;

STATE s_Meteor[5][4] =
{
    {
        {METEOR_R0 + 0, METEOR_RATE, DoMeteor, &s_Meteor[0][1]},
        {METEOR_R0 + 1, METEOR_RATE, DoMeteor, &s_Meteor[0][2]},
        {METEOR_R0 + 2, METEOR_RATE, DoMeteor, &s_Meteor[0][3]},
        {METEOR_R0 + 3, METEOR_RATE, DoMeteor, &s_Meteor[0][0]},
    },
    {
        {METEOR_R1 + 0, METEOR_RATE, DoMeteor, &s_Meteor[1][1]},
        {METEOR_R1 + 1, METEOR_RATE, DoMeteor, &s_Meteor[1][2]},
        {METEOR_R1 + 2, METEOR_RATE, DoMeteor, &s_Meteor[1][3]},
        {METEOR_R1 + 3, METEOR_RATE, DoMeteor, &s_Meteor[1][0]},
    },
    {
        {METEOR_R2 + 0, METEOR_RATE, DoMeteor, &s_Meteor[2][1]},
        {METEOR_R2 + 1, METEOR_RATE, DoMeteor, &s_Meteor[2][2]},
        {METEOR_R2 + 2, METEOR_RATE, DoMeteor, &s_Meteor[2][3]},
        {METEOR_R2 + 3, METEOR_RATE, DoMeteor, &s_Meteor[2][0]},
    },
    {
        {METEOR_R3 + 0, METEOR_RATE, DoMeteor, &s_Meteor[3][1]},
        {METEOR_R3 + 1, METEOR_RATE, DoMeteor, &s_Meteor[3][2]},
        {METEOR_R3 + 2, METEOR_RATE, DoMeteor, &s_Meteor[3][3]},
        {METEOR_R3 + 3, METEOR_RATE, DoMeteor, &s_Meteor[3][0]},
    },
    {
        {METEOR_R4 + 0, METEOR_RATE, DoMeteor, &s_Meteor[4][1]},
        {METEOR_R4 + 1, METEOR_RATE, DoMeteor, &s_Meteor[4][2]},
        {METEOR_R4 + 2, METEOR_RATE, DoMeteor, &s_Meteor[4][3]},
        {METEOR_R4 + 3, METEOR_RATE, DoMeteor, &s_Meteor[4][0]},
    }
};


STATE* sg_Meteor[] =
{
    &s_Meteor[0][0],
    &s_Meteor[1][0],
    &s_Meteor[2][0],
    &s_Meteor[3][0],
    &s_Meteor[4][0]
};

#define METEOR_EXP 2115
#define METEOR_EXP_RATE 7

STATE s_MeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_MeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_MeteorExp[6]}
};

#define MIRV_METEOR METEOR_R0
ANIMATOR DoMirvMissile;
STATE s_MirvMeteor[5][4] =
{
    {
        {METEOR_R0 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][1]},
        {METEOR_R0 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][2]},
        {METEOR_R0 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][3]},
        {METEOR_R0 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[0][0]},
    },
    {
        {METEOR_R1 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][1]},
        {METEOR_R1 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][2]},
        {METEOR_R1 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][3]},
        {METEOR_R1 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[1][0]},
    },
    {
        {METEOR_R2 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][1]},
        {METEOR_R2 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][2]},
        {METEOR_R2 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][3]},
        {METEOR_R2 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[2][0]},
    },
    {
        {METEOR_R3 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][1]},
        {METEOR_R3 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][2]},
        {METEOR_R3 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][3]},
        {METEOR_R3 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[3][0]},
    },
    {
        {METEOR_R4 + 0, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][1]},
        {METEOR_R4 + 1, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][2]},
        {METEOR_R4 + 2, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][3]},
        {METEOR_R4 + 3, METEOR_RATE, DoMirvMissile, &s_MirvMeteor[4][0]},
    }
};


STATE* sg_MirvMeteor[] =
{
    &s_MirvMeteor[0][0],
    &s_MirvMeteor[1][0],
    &s_MirvMeteor[2][0],
    &s_MirvMeteor[3][0],
    &s_MirvMeteor[4][0]
};

STATE s_MirvMeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_MirvMeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_MirvMeteorExp[6]}
};

#define SERP_METEOR METEOR_R0+1
ANIMATOR DoSerpMeteor;
STATE s_SerpMeteor[5][4] =
{
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[0][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[1][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[2][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[3][0]},
    },
    {
        {2031 + 0, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][1]},
        {2031 + 1, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][2]},
        {2031 + 2, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][3]},
        {2031 + 3, METEOR_RATE, DoSerpMeteor, &s_SerpMeteor[4][0]},
    }
};


STATE* sg_SerpMeteor[] =
{
    &s_SerpMeteor[0][0],
    &s_SerpMeteor[1][0],
    &s_SerpMeteor[2][0],
    &s_SerpMeteor[3][0],
    &s_SerpMeteor[4][0]
};


STATE s_SerpMeteorExp[] =
{
    {METEOR_EXP + 0, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[1]},
    {METEOR_EXP + 1, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[2]},
    {METEOR_EXP + 2, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[3]},
    {METEOR_EXP + 3, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[4]},
    {METEOR_EXP + 4, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[5]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, NullAnimator, &s_SerpMeteorExp[6]},
    {METEOR_EXP + 5, METEOR_EXP_RATE, DoSuicide, &s_SerpMeteorExp[6]}
};

//////////////////////
//
// SPEAR
//
//////////////////////

#define SPEAR_RATE 8
ANIMATOR DoSpear;

STATE s_Spear[5][1] =
{
    {
        {SPEAR_R0 + 0, SPEAR_RATE, DoSpear, s_Spear[0]},
    },
    {
        {SPEAR_R1 + 0, SPEAR_RATE, DoSpear, s_Spear[1]},
    },
    {
        {SPEAR_R2 + 0, SPEAR_RATE, DoSpear, s_Spear[2]},
    },
    {
        {SPEAR_R3 + 0, SPEAR_RATE, DoSpear, s_Spear[3]},
    },
    {
        {SPEAR_R4 + 0, SPEAR_RATE, DoSpear, s_Spear[4]},
    }
};

STATE* sg_Spear[] =
{
    s_Spear[0],
    s_Spear[1],
    s_Spear[2],
    s_Spear[3],
    s_Spear[4]
};

//////////////////////
//
// ROCKET
//
//////////////////////

#define ROCKET_FRAMES 1
#define ROCKET_R0 2206
#define ROCKET_R1 ROCKET_R0 + (ROCKET_FRAMES * 2)
#define ROCKET_R2 ROCKET_R0 + (ROCKET_FRAMES * 3)
#define ROCKET_R3 ROCKET_R0 + (ROCKET_FRAMES * 4)
#define ROCKET_R4 ROCKET_R0 + (ROCKET_FRAMES * 1)

ANIMATOR DoRocket;
#define ROCKET_RATE 8

STATE s_Rocket[5][1] =
{
    {
        {ROCKET_R0 + 0, ROCKET_RATE, DoRocket, &s_Rocket[0][0]},
    },
    {
        {ROCKET_R1 + 0, ROCKET_RATE, DoRocket, &s_Rocket[1][0]},
    },
    {
        {ROCKET_R2 + 0, ROCKET_RATE, DoRocket, &s_Rocket[2][0]},
    },
    {
        {ROCKET_R3 + 0, ROCKET_RATE, DoRocket, &s_Rocket[3][0]},
    },
    {
        {ROCKET_R4 + 0, ROCKET_RATE, DoRocket, &s_Rocket[4][0]},
    }
};

STATE* sg_Rocket[] =
{
    &s_Rocket[0][0],
    &s_Rocket[1][0],
    &s_Rocket[2][0],
    &s_Rocket[3][0],
    &s_Rocket[4][0]
};

//////////////////////
//
// BUNNY ROCKET
//
//////////////////////

#define BUNNYROCKET_FRAMES 5
#define BUNNYROCKET_R0 4550
#define BUNNYROCKET_R1 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 1)
#define BUNNYROCKET_R2 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 2)
#define BUNNYROCKET_R3 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 3)
#define BUNNYROCKET_R4 BUNNYROCKET_R0 + (BUNNYROCKET_FRAMES * 4)

ANIMATOR DoRocket;
#define BUNNYROCKET_RATE 8

STATE s_BunnyRocket[5][1] =
{
    {
        {BUNNYROCKET_R0 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[0][0]},
    },
    {
        {BUNNYROCKET_R1 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[1][0]},
    },
    {
        {BUNNYROCKET_R2 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[2][0]},
    },
    {
        {BUNNYROCKET_R3 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[3][0]},
    },
    {
        {BUNNYROCKET_R4 + 2, BUNNYROCKET_RATE, DoRocket, &s_BunnyRocket[4][0]},
    }
};

STATE* sg_BunnyRocket[] =
{
    &s_BunnyRocket[0][0],
    &s_BunnyRocket[1][0],
    &s_BunnyRocket[2][0],
    &s_BunnyRocket[3][0],
    &s_BunnyRocket[4][0]
};

ANIMATOR DoRail;
#define RAIL_RATE 8

STATE s_Rail[5][1] =
{
    {
        {ROCKET_R0 + 0, RAIL_RATE, DoRail, &s_Rail[0][0]},
    },
    {
        {ROCKET_R1 + 0, RAIL_RATE, DoRail, &s_Rail[1][0]},
    },
    {
        {ROCKET_R2 + 0, RAIL_RATE, DoRail, &s_Rail[2][0]},
    },
    {
        {ROCKET_R3 + 0, RAIL_RATE, DoRail, &s_Rail[3][0]},
    },
    {
        {ROCKET_R4 + 0, RAIL_RATE, DoRail, &s_Rail[4][0]},
    }
};

STATE* sg_Rail[] =
{
    &s_Rail[0][0],
    &s_Rail[1][0],
    &s_Rail[2][0],
    &s_Rail[3][0],
    &s_Rail[4][0]
};

ANIMATOR DoLaser;
#define LASER_RATE 8

STATE s_Laser[] =
{
    {ROCKET_R0 + 0, LASER_RATE, DoLaser, &s_Laser[0]}
};

//////////////////////
//
// MICRO
//
//////////////////////

#define MICRO_FRAMES 1
#define MICRO_R0 2206
#define MICRO_R1 MICRO_R0 + (MICRO_FRAMES * 2)
#define MICRO_R2 MICRO_R0 + (MICRO_FRAMES * 3)
#define MICRO_R3 MICRO_R0 + (MICRO_FRAMES * 4)
#define MICRO_R4 MICRO_R0 + (MICRO_FRAMES * 1)

ANIMATOR DoMicro;
#define MICRO_RATE 8

#if 0
#define PUFF 1748
#define PUFF_RATE 8
ANIMATOR DoMicroPuffSuicide;
STATE s_MicroPuff[] =
{
    {PUFF + 0, PUFF_RATE, DoPuff, &s_MicroPuff[1]},
    {PUFF + 1, PUFF_RATE, DoPuff, &s_MicroPuff[2]},
    {PUFF + 2, PUFF_RATE, DoPuff, &s_MicroPuff[3]},
    {PUFF + 3, PUFF_RATE, DoPuff, &s_MicroPuff[4]},
    {PUFF + 4, PUFF_RATE, DoPuff, &s_MicroPuff[5]},
    {PUFF + 5, PUFF_RATE, DoPuff, &s_MicroPuff[6]},
    {PUFF + 5, 100, DoMicroPuffSuicide, &s_MicroPuff[6]}
};
#endif

STATE s_Micro[5][1] =
{
    {
        {MICRO_R0 + 0, MICRO_RATE, DoMicro, &s_Micro[0][0]},
    },
    {
        {MICRO_R1 + 0, MICRO_RATE, DoMicro, &s_Micro[1][0]},
    },
    {
        {MICRO_R2 + 0, MICRO_RATE, DoMicro, &s_Micro[2][0]},
    },
    {
        {MICRO_R3 + 0, MICRO_RATE, DoMicro, &s_Micro[3][0]},
    },
    {
        {MICRO_R4 + 0, MICRO_RATE, DoMicro, &s_Micro[4][0]},
    }
};

STATE* sg_Micro[] =
{
    &s_Micro[0][0],
    &s_Micro[1][0],
    &s_Micro[2][0],
    &s_Micro[3][0],
    &s_Micro[4][0]
};

ANIMATOR DoMicroMini;
STATE s_MicroMini[5][1] =
{
    {
        {MICRO_R0 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[0][0]},
    },
    {
        {MICRO_R1 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[1][0]},
    },
    {
        {MICRO_R2 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[2][0]},
    },
    {
        {MICRO_R3 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[3][0]},
    },
    {
        {MICRO_R4 + 0, MICRO_RATE, DoMicroMini, &s_MicroMini[4][0]},
    }
};

STATE* sg_MicroMini[] =
{
    &s_MicroMini[0][0],
    &s_MicroMini[1][0],
    &s_MicroMini[2][0],
    &s_MicroMini[3][0],
    &s_MicroMini[4][0]
};

//////////////////////
//
// BOLT THINMAN
//
//////////////////////

#define BOLT_THINMAN_RATE 8
ANIMATOR DoBoltThinMan;

STATE s_BoltThinMan[5][1] =
{
    {
        {BOLT_THINMAN_R0 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[0][0]},
    },
    {
        {BOLT_THINMAN_R1 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[1][0]},
    },
    {
        {BOLT_THINMAN_R2 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[2][0]},
    },
    {
        {BOLT_THINMAN_R3 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[3][0]},
    },
    {
        {BOLT_THINMAN_R4 + 0, BOLT_THINMAN_RATE, DoBoltThinMan, &s_BoltThinMan[4][0]},
    }
};

STATE* sg_BoltThinMan[] =
{
    &s_BoltThinMan[0][0],
    &s_BoltThinMan[1][0],
    &s_BoltThinMan[2][0],
    &s_BoltThinMan[3][0],
    &s_BoltThinMan[4][0]
};

#define BOLT_SEEKER_RATE 8
ANIMATOR DoBoltSeeker;

STATE s_BoltSeeker[5][1] =
{
    {
        {BOLT_THINMAN_R0 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[0][0]},
    },
    {
        {BOLT_THINMAN_R1 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[1][0]},
    },
    {
        {BOLT_THINMAN_R2 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[2][0]},
    },
    {
        {BOLT_THINMAN_R3 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[3][0]},
    },
    {
        {BOLT_THINMAN_R4 + 0, BOLT_SEEKER_RATE, DoBoltSeeker, &s_BoltSeeker[4][0]},
    }
};

STATE* sg_BoltSeeker[] =
{
    &s_BoltSeeker[0][0],
    &s_BoltSeeker[1][0],
    &s_BoltSeeker[2][0],
    &s_BoltSeeker[3][0],
    &s_BoltSeeker[4][0]
};


#define BOLT_FATMAN STAR
#define BOLT_FATMAN_RATE 8
ANIMATOR DoBoltFatMan;

STATE s_BoltFatMan[] =
{
    {BOLT_FATMAN + 0, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[1]},
    {BOLT_FATMAN + 1, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[2]},
    {BOLT_FATMAN + 2, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[3]},
    {BOLT_FATMAN + 3, BOLT_FATMAN_RATE, DoBoltFatMan, &s_BoltFatMan[0]}
};


#define BOLT_SHRAPNEL STAR
#define BOLT_SHRAPNEL_RATE 8
ANIMATOR DoBoltShrapnel;

STATE s_BoltShrapnel[] =
{
    {BOLT_SHRAPNEL + 0, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[1]},
    {BOLT_SHRAPNEL + 1, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[2]},
    {BOLT_SHRAPNEL + 2, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[3]},
    {BOLT_SHRAPNEL + 3, BOLT_SHRAPNEL_RATE, DoBoltShrapnel, &s_BoltShrapnel[0]}
};

#define COOLG_FIRE 2430
//#define COOLG_FIRE 1465
#define COOLG_W_FIRE_RATE 8
ANIMATOR DoCoolgFire;

STATE s_CoolgFire[] =
{
    {2031 + 0, COOLG_W_FIRE_RATE, DoCoolgFire, &s_CoolgFire[1]},
    {2031 + 1, COOLG_W_FIRE_RATE, DoCoolgFire, &s_CoolgFire[2]},
    {2031 + 2, COOLG_W_FIRE_RATE, DoCoolgFire, &s_CoolgFire[3]},
    {2031 + 3, COOLG_W_FIRE_RATE, DoCoolgFire, &s_CoolgFire[0]}
};

#define COOLG_FIRE_DONE 2410
//#define COOLG_FIRE_DONE 1466
//#define COOLG_FIRE_DONE_RATE 8
#define COOLG_FIRE_DONE_RATE 3

STATE s_CoolgFireDone[] =
{
    {COOLG_FIRE_DONE + 0, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[1]},
    {COOLG_FIRE_DONE + 1, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[2]},
    {COOLG_FIRE_DONE + 2, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[3]},
    {COOLG_FIRE_DONE + 3, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[4]},
    {COOLG_FIRE_DONE + 4, COOLG_FIRE_DONE_RATE, NullAnimator, &s_CoolgFireDone[5]},
    {COOLG_FIRE_DONE + 4, COOLG_FIRE_DONE_RATE, DoSuicide, &s_CoolgFireDone[5]}
};

ANIMATOR DoCoolgDrip;
#define COOLG_DRIP 1720
STATE s_CoolgDrip[] =
{
    {COOLG_DRIP + 0, 100, DoCoolgDrip, &s_CoolgDrip[0]}
};

#define GORE_FLOOR_SPLASH_RATE 8
#define GORE_FLOOR_SPLASH 1710
STATE s_GoreFloorSplash[] =
{
    {GORE_FLOOR_SPLASH + 0, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[1]},
    {GORE_FLOOR_SPLASH + 1, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[2]},
    {GORE_FLOOR_SPLASH + 2, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[3]},
    {GORE_FLOOR_SPLASH + 3, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[4]},
    {GORE_FLOOR_SPLASH + 4, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[5]},
    {GORE_FLOOR_SPLASH + 5, GORE_FLOOR_SPLASH_RATE, NullAnimator, &s_GoreFloorSplash[6]},
    {GORE_FLOOR_SPLASH + 5, GORE_FLOOR_SPLASH_RATE, DoSuicide, &s_GoreFloorSplash[6]}
};

#define GORE_SPLASH_RATE 8
#define GORE_SPLASH 2410
STATE s_GoreSplash[] =
{
    {GORE_SPLASH + 0, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[1]},
    {GORE_SPLASH + 1, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[2]},
    {GORE_SPLASH + 2, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[3]},
    {GORE_SPLASH + 3, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[4]},
    {GORE_SPLASH + 4, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[5]},
    {GORE_SPLASH + 5, GORE_SPLASH_RATE, NullAnimator, &s_GoreSplash[6]},
    {GORE_SPLASH + 5, GORE_SPLASH_RATE, DoSuicide, &s_GoreSplash[6]}
};

//////////////////////////////////////////////
//
//  HEART ATTACK & PLASMA
//
//////////////////////////////////////////////

#define PLASMA 1562 //2058
#define PLASMA_FOUNTAIN 2058+1
#define PLASMA_RATE 8
#define PLASMA_FOUNTAIN_TIME (3*120);

ANIMATOR DoPlasma;

// regular bolt from heart
STATE s_Plasma[] =
{
    {PLASMA + 0, PLASMA_RATE, DoPlasma, &s_Plasma[1]},
    {PLASMA + 1, PLASMA_RATE, DoPlasma, &s_Plasma[2]},
    {PLASMA + 2, PLASMA_RATE, DoPlasma, &s_Plasma[0]}
};

ANIMATOR DoPlasmaFountain;

// follows actor spewing blood
#define PLASMA_Drip 1562 //2420
STATE s_PlasmaFountain[] =
{
    {PLASMA_Drip + 0, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[1]},
    {PLASMA_Drip + 1, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[2]},
    {PLASMA_Drip + 2, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[3]},
    {PLASMA_Drip + 3, PLASMA_RATE, DoPlasmaFountain, &s_PlasmaFountain[0]},
};

#define PLASMA_Drip_RATE 12
STATE s_PlasmaDrip[] =
{
    {PLASMA_Drip + 0, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[1]},
    {PLASMA_Drip + 1, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[2]},
    {PLASMA_Drip + 2, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[3]},
    {PLASMA_Drip + 3, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[4]},
    {PLASMA_Drip + 4, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[5]},
    {PLASMA_Drip + 5, PLASMA_Drip_RATE, DoShrapJumpFall, &s_PlasmaDrip[6]},
    {PLASMA_Drip + 7, PLASMA_Drip_RATE, DoSuicide, &s_PlasmaDrip[6]}
};

#define PLASMA_DONE 2061
#define PLASMA_DONE_RATE 15

ANIMATOR DoPlasmaDone;

STATE s_PlasmaDone[] =
{
    {PLASMA + 0, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[1]},
    {PLASMA + 2, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[2]},
    {PLASMA + 1, PLASMA_DONE_RATE, DoPlasmaDone, &s_PlasmaDone[0]}
};

#define TELEPORT_EFFECT 3240
#define TELEPORT_EFFECT_RATE 6
ANIMATOR DoTeleportEffect;
STATE s_TeleportEffect[] =
{
    {TELEPORT_EFFECT + 0, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[1]},
    {TELEPORT_EFFECT + 1, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[2]},
    {TELEPORT_EFFECT + 2, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[3]},
    {TELEPORT_EFFECT + 3, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[4]},
    {TELEPORT_EFFECT + 4, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[5]},
    {TELEPORT_EFFECT + 5, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[6]},
    {TELEPORT_EFFECT + 6, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[7]},
    {TELEPORT_EFFECT + 7, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[8]},
    {TELEPORT_EFFECT + 8, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[9]},
    {TELEPORT_EFFECT + 9, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[10]},
    {TELEPORT_EFFECT + 10, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[11]},
    {TELEPORT_EFFECT + 11, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[12]},
    {TELEPORT_EFFECT + 12, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[13]},
    {TELEPORT_EFFECT + 13, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[14]},
    {TELEPORT_EFFECT + 14, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[15]},
    {TELEPORT_EFFECT + 15, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[16]},
    {TELEPORT_EFFECT + 16, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[17]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect[18]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, DoSuicide, &s_TeleportEffect[18]},
};

// Spawn a RIPPER teleport effect
ANIMATOR DoTeleRipper;
STATE s_TeleportEffect2[] =
{
    {TELEPORT_EFFECT + 0, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[1]},
    {TELEPORT_EFFECT + 1, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[2]},
    {TELEPORT_EFFECT + 2, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[3]},
    {TELEPORT_EFFECT + 3, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[4]},
    {TELEPORT_EFFECT + 4, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[5]},
    {TELEPORT_EFFECT + 5, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[6]},
    {TELEPORT_EFFECT + 6, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[7]},
    {TELEPORT_EFFECT + 7, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[8]},
    {TELEPORT_EFFECT + 8, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[9]},
    {TELEPORT_EFFECT + 9, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[10]},
    {TELEPORT_EFFECT + 10, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[11]},
    {TELEPORT_EFFECT + 11, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[12]},
    {TELEPORT_EFFECT + 12, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[13]},
    {TELEPORT_EFFECT + 13, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[14]},
    {TELEPORT_EFFECT + 14, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[15]},
    {TELEPORT_EFFECT + 15, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[16]},
    {TELEPORT_EFFECT + 16, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[17]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, NullAnimator, &s_TeleportEffect2[18]},
    {TELEPORT_EFFECT + 17, SF_QUICK_CALL, DoTeleRipper, &s_TeleportEffect2[19]},
    {TELEPORT_EFFECT + 17, TELEPORT_EFFECT_RATE, DoSuicide, &s_TeleportEffect2[19]},
};


ANIMATOR DoElectro;
#define ELECTRO_SNAKE 2073
#define ELECTRO_PLAYER (ELECTRO)
#define ELECTRO_ENEMY (ELECTRO + 1)
#define ELECTRO_SHARD (ELECTRO + 2)

STATE s_Electro[] =
{
    {ELECTRO + 0, 12, DoElectro, &s_Electro[1]},
    {ELECTRO + 1, 12, DoElectro, &s_Electro[2]},
    {ELECTRO + 2, 12, DoElectro, &s_Electro[3]},
    {ELECTRO + 3, 12, DoElectro, &s_Electro[0]}
};

STATE s_ElectroShrap[] =
{
    {ELECTRO + 0, 12, DoShrapDamage, &s_ElectroShrap[1]},
    {ELECTRO + 1, 12, DoShrapDamage, &s_ElectroShrap[2]},
    {ELECTRO + 2, 12, DoShrapDamage, &s_ElectroShrap[3]},
    {ELECTRO + 3, 12, DoShrapDamage, &s_ElectroShrap[0]}
};

//////////////////////
//
// EXPS
//
//////////////////////
#define GRENADE_EXP 3121
#define MINE_EXP GRENADE_EXP+1
#define GRENADE_EXP_RATE 6

#if 0
STATE s_GrenadeExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[7]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[8]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[9]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[10]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[11]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[12]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[13]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[14]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[15]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[16]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[17]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[18]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[19]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[20]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[21]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[22]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeExp[22]}
};
#else
STATE s_GrenadeSmallExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[7]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[8]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[9]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[10]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[11]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[12]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[13]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[14]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[15]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[16]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[17]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[18]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[19]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[20]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[21]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeSmallExp[22]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeSmallExp[22]}
};

ANIMATOR SpawnGrenadeSmallExp;
STATE s_GrenadeExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[3]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[4]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[5]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[6]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[7]},
    {GRENADE_EXP + 6, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[8]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[9]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[10]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[11]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[12]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[13]},
    {GRENADE_EXP + 12, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[14]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[15]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[16]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[17]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[18]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[19]},
    {GRENADE_EXP + 17, SF_QUICK_CALL,     SpawnGrenadeSmallExp, &s_GrenadeExp[20]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[21]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[22]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[23]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[24]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_GrenadeExp[25]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_GrenadeExp[25]}
};
#endif

#define MINE_EXP GRENADE_EXP+1
ANIMATOR DoMineExp, DoMineExpMine;
STATE s_MineExp[] =
{
    {GRENADE_EXP + 0, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[1]},
    {GRENADE_EXP + 1, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[2]},
    {GRENADE_EXP + 2, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[3]},
    {GRENADE_EXP + 3, 0|SF_QUICK_CALL, DoMineExp, &s_MineExp[4]},
    {GRENADE_EXP + 3, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[5]},
    {GRENADE_EXP + 4, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[6]},
    {GRENADE_EXP + 5, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[7]},
    {GRENADE_EXP + 6, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[8]},
    {GRENADE_EXP + 7, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[9]},
    {GRENADE_EXP + 8, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[10]},
    {GRENADE_EXP + 9, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[11]},
    {GRENADE_EXP + 10, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[12]},
    {GRENADE_EXP + 11, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[13]},
    {GRENADE_EXP + 12, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[14]},
    {GRENADE_EXP + 13, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[15]},
    {GRENADE_EXP + 14, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[16]},
    {GRENADE_EXP + 15, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[17]},
    {GRENADE_EXP + 16, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[18]},
    {GRENADE_EXP + 17, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[19]},
    {GRENADE_EXP + 17, 0|SF_QUICK_CALL, DoMineExpMine, &s_MineExp[20]},
    {GRENADE_EXP + 18, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[21]},
    {GRENADE_EXP + 19, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[22]},
    {GRENADE_EXP + 20, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[23]},
    {GRENADE_EXP + 21, GRENADE_EXP_RATE, NullAnimator, &s_MineExp[24]},
    {GRENADE_EXP + 21, 100, DoSuicide, &s_MineExp[24]}
};

#define EXP_RATE_W 7
#define BOLT_EXP EXP
#define FIREBALL_EXP EXP+1
#define BASIC_EXP EXP+2
#define SECTOR_EXP EXP+3
#define MICRO_EXP EXP+5
#define TRACER_EXP EXP+6
#define TANK_SHELL_EXP EXP+7

STATE s_BasicExp[] =
{
    {EXP + 0, EXP_RATE_W, NullAnimator, &s_BasicExp[1]},
    {EXP + 1, EXP_RATE_W, NullAnimator, &s_BasicExp[2]},
    {EXP + 2, EXP_RATE_W, NullAnimator, &s_BasicExp[3]},
    {EXP + 3, EXP_RATE_W, NullAnimator, &s_BasicExp[4]},
    {EXP + 4, EXP_RATE_W, NullAnimator, &s_BasicExp[5]},
    {EXP + 5, EXP_RATE_W, NullAnimator, &s_BasicExp[6]},
    {EXP + 6, EXP_RATE_W, NullAnimator, &s_BasicExp[7]},
    {EXP + 7, EXP_RATE_W, NullAnimator, &s_BasicExp[8]},
    {EXP + 8, EXP_RATE_W, NullAnimator, &s_BasicExp[9]},
    {EXP + 9, EXP_RATE_W, NullAnimator, &s_BasicExp[10]},
    {EXP + 10, EXP_RATE_W, NullAnimator, &s_BasicExp[11]},
    {EXP + 11, EXP_RATE_W, NullAnimator, &s_BasicExp[12]},
    {EXP + 12, EXP_RATE_W, NullAnimator, &s_BasicExp[13]},
    {EXP + 13, EXP_RATE_W, NullAnimator, &s_BasicExp[14]},
    {EXP + 14, EXP_RATE_W, NullAnimator, &s_BasicExp[15]},
    {EXP + 15, EXP_RATE_W, NullAnimator, &s_BasicExp[16]},
    {EXP + 16, EXP_RATE_W, NullAnimator, &s_BasicExp[17]},
    {EXP + 17, EXP_RATE_W, NullAnimator, &s_BasicExp[18]},
    {EXP + 18, EXP_RATE_W, NullAnimator, &s_BasicExp[19]},
    {EXP + 19, EXP_RATE_W, NullAnimator, &s_BasicExp[20]},
    {EXP + 20, 100, DoSuicide, &s_BasicExp[0]}
};

#define MICRO_EXP_RATE 3
ANIMATOR DoExpDamageTest;

STATE s_MicroExp[] =
{
    {EXP + 0, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[1]},
    {EXP + 0, SF_QUICK_CALL,  DoExpDamageTest,   &s_MicroExp[2]},
    {EXP + 1, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[3]},
    {EXP + 2, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[4]},
    {EXP + 3, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[5]},
    {EXP + 4, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[6]},
    {EXP + 5, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[7]},
    {EXP + 6, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[8]},
    {EXP + 7, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[9]},
    {EXP + 8, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[10]},
    {EXP + 9, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[11]},
    {EXP + 10, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[12]},
    {EXP + 11, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[13]},
    {EXP + 12, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[14]},
    {EXP + 13, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[15]},
    {EXP + 14, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[16]},
    {EXP + 15, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[17]},
    {EXP + 16, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[18]},
    {EXP + 17, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[19]},
    {EXP + 18, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[20]},
    {EXP + 19, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[21]},
    {EXP + 20, MICRO_EXP_RATE, NullAnimator, &s_MicroExp[22]},
    {EXP + 20, 100, DoSuicide, &s_MicroExp[22]}
};


#define BIG_GUN_FLAME_RATE 15
STATE s_BigGunFlame[] =
{
    // first 3 frames
    {EXP + 0, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[1]},
    {EXP + 1, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[2]},
    {EXP + 2, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[3]},
    // last 4 frames frames
    {EXP + 17, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[4]},
    {EXP + 18, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[5]},
    {EXP + 19, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[6]},
    {EXP + 20, BIG_GUN_FLAME_RATE, NullAnimator, &s_BigGunFlame[7]},
    {EXP + 20, 100, DoSuicide, &s_BigGunFlame[0]}
};

STATE s_BoltExp[] =
{
    {EXP + 0, EXP_RATE_W, NullAnimator, &s_BoltExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_BoltExp[2]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,   &s_BoltExp[3]},
    {EXP + 1, EXP_RATE_W, NullAnimator, &s_BoltExp[4]},
    {EXP + 2, EXP_RATE_W, NullAnimator, &s_BoltExp[5]},
    {EXP + 3, EXP_RATE_W, NullAnimator, &s_BoltExp[6]},
    {EXP + 4, EXP_RATE_W, NullAnimator, &s_BoltExp[7]},
    {EXP + 5, EXP_RATE_W, NullAnimator, &s_BoltExp[8]},
    {EXP + 6, EXP_RATE_W, NullAnimator, &s_BoltExp[9]},
    {EXP + 7, EXP_RATE_W, NullAnimator, &s_BoltExp[10]},
    {EXP + 7, SF_QUICK_CALL,  SpawnShrapX,   &s_BoltExp[11]},
    {EXP + 8, EXP_RATE_W, NullAnimator, &s_BoltExp[12]},
    {EXP + 9, EXP_RATE_W, NullAnimator, &s_BoltExp[13]},
    {EXP + 10, EXP_RATE_W, NullAnimator, &s_BoltExp[14]},
    {EXP + 11, EXP_RATE_W, NullAnimator, &s_BoltExp[15]},
    {EXP + 12, EXP_RATE_W, NullAnimator, &s_BoltExp[16]},
    {EXP + 13, EXP_RATE_W, NullAnimator, &s_BoltExp[17]},
    {EXP + 14, EXP_RATE_W, NullAnimator, &s_BoltExp[18]},
    {EXP + 15, EXP_RATE_W, NullAnimator, &s_BoltExp[19]},
    {EXP + 16, EXP_RATE_W, NullAnimator, &s_BoltExp[20]},
    {EXP + 17, EXP_RATE_W, NullAnimator, &s_BoltExp[21]},
    {EXP + 18, EXP_RATE_W, NullAnimator, &s_BoltExp[22]},
    {EXP + 19, EXP_RATE_W, NullAnimator, &s_BoltExp[23]},
    {EXP + 20, EXP_RATE_W, NullAnimator, &s_BoltExp[24]},
    {EXP + 20, 100, DoSuicide, &s_BoltExp[0]}
};

STATE s_TankShellExp[] =
{
    {EXP + 0, EXP_RATE_W, NullAnimator, &s_TankShellExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_TankShellExp[2]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,   &s_TankShellExp[3]},
    {EXP + 1, EXP_RATE_W, NullAnimator, &s_TankShellExp[4]},
    {EXP + 2, EXP_RATE_W, NullAnimator, &s_TankShellExp[5]},
    {EXP + 3, EXP_RATE_W, NullAnimator, &s_TankShellExp[6]},
    {EXP + 4, EXP_RATE_W, NullAnimator, &s_TankShellExp[7]},
    {EXP + 5, EXP_RATE_W, NullAnimator, &s_TankShellExp[8]},
    {EXP + 6, EXP_RATE_W, NullAnimator, &s_TankShellExp[9]},
    {EXP + 7, EXP_RATE_W, NullAnimator, &s_TankShellExp[10]},
    {EXP + 7, SF_QUICK_CALL,  SpawnShrapX,   &s_TankShellExp[11]},
    {EXP + 8, EXP_RATE_W, NullAnimator, &s_TankShellExp[12]},
    {EXP + 9, EXP_RATE_W, NullAnimator, &s_TankShellExp[13]},
    {EXP + 10, EXP_RATE_W, NullAnimator, &s_TankShellExp[14]},
    {EXP + 11, EXP_RATE_W, NullAnimator, &s_TankShellExp[15]},
    {EXP + 12, EXP_RATE_W, NullAnimator, &s_TankShellExp[16]},
    {EXP + 13, EXP_RATE_W, NullAnimator, &s_TankShellExp[17]},
    {EXP + 14, EXP_RATE_W, NullAnimator, &s_TankShellExp[18]},
    {EXP + 15, EXP_RATE_W, NullAnimator, &s_TankShellExp[19]},
    {EXP + 16, EXP_RATE_W, NullAnimator, &s_TankShellExp[20]},
    {EXP + 17, EXP_RATE_W, NullAnimator, &s_TankShellExp[21]},
    {EXP + 18, EXP_RATE_W, NullAnimator, &s_TankShellExp[22]},
    {EXP + 19, EXP_RATE_W, NullAnimator, &s_TankShellExp[23]},
    {EXP + 20, EXP_RATE_W, NullAnimator, &s_TankShellExp[24]},
    {EXP + 20, 100, DoSuicide, &s_TankShellExp[0]}
};

#define TRACER_EXP_RATE 4
STATE s_TracerExp[] =
{
    {EXP + 0, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[1]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,  &s_TracerExp[2]},
    {EXP + 0, SF_QUICK_CALL,  NullAnimator,   &s_TracerExp[3]},
    {EXP + 1, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[4]},
    {EXP + 2, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[5]},
    {EXP + 3, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[6]},
    {EXP + 4, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[7]},
    {EXP + 5, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[8]},
    {EXP + 6, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[9]},
    {EXP + 7, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[10]},
    {EXP + 7, SF_QUICK_CALL,  NullAnimator,   &s_TracerExp[11]},
    {EXP + 8, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[12]},
    {EXP + 9, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[13]},
    {EXP + 10, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[14]},
    {EXP + 11, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[15]},
    {EXP + 12, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[16]},
    {EXP + 13, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[17]},
    {EXP + 14, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[18]},
    {EXP + 15, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[19]},
    {EXP + 16, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[20]},
    {EXP + 17, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[21]},
    {EXP + 18, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[22]},
    {EXP + 19, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[23]},
    {EXP + 20, TRACER_EXP_RATE, NullAnimator, &s_TracerExp[24]},
    {EXP + 20, 100, DoSuicide, &s_TracerExp[0]}
};

#define EXP_RATE_W 7
ANIMATOR DoSectorExp;

STATE s_SectorExp[] =
{
    {EXP + 0, EXP_RATE_W, DoSectorExp, &s_SectorExp[1]},
    {EXP + 0, SF_QUICK_CALL,  SpawnShrapX,  &s_SectorExp[2]},
    {EXP + 0, SF_QUICK_CALL,  DoSectorExp,   &s_SectorExp[3]},
    {EXP + 1, EXP_RATE_W, DoSectorExp, &s_SectorExp[4]},
    {EXP + 2, EXP_RATE_W, DoSectorExp, &s_SectorExp[5]},
    {EXP + 3, EXP_RATE_W, DoSectorExp, &s_SectorExp[6]},
    {EXP + 4, EXP_RATE_W, DoSectorExp, &s_SectorExp[7]},
    {EXP + 5, EXP_RATE_W, DoSectorExp, &s_SectorExp[8]},
    {EXP + 6, EXP_RATE_W, DoSectorExp, &s_SectorExp[9]},
    {EXP + 7, EXP_RATE_W, DoSectorExp, &s_SectorExp[10]},
    {EXP + 7, SF_QUICK_CALL,  DoSectorExp,  &s_SectorExp[11]},
    {EXP + 8, EXP_RATE_W, DoSectorExp, &s_SectorExp[12]},
    {EXP + 9, EXP_RATE_W, DoSectorExp, &s_SectorExp[13]},
    {EXP + 10, EXP_RATE_W, DoSectorExp, &s_SectorExp[14]},
    {EXP + 11, EXP_RATE_W, DoSectorExp, &s_SectorExp[15]},
    {EXP + 12, EXP_RATE_W, DoSectorExp, &s_SectorExp[16]},
    {EXP + 13, EXP_RATE_W, DoSectorExp, &s_SectorExp[17]},
    {EXP + 14, EXP_RATE_W, DoSectorExp, &s_SectorExp[18]},
    {EXP + 15, EXP_RATE_W, DoSectorExp, &s_SectorExp[19]},
    {EXP + 16, EXP_RATE_W, DoSectorExp, &s_SectorExp[20]},
    {EXP + 17, EXP_RATE_W, DoSectorExp, &s_SectorExp[21]},
    {EXP + 18, EXP_RATE_W, DoSectorExp, &s_SectorExp[22]},
    {EXP + 19, EXP_RATE_W, DoSectorExp, &s_SectorExp[23]},
    {EXP + 20, EXP_RATE_W, DoSectorExp, &s_SectorExp[24]},
    {EXP + 20, 100, DoSuicide, &s_SectorExp[0]}
};


#define FIREBALL_DISS 3196
#define FIREBALL_DISS_RATE 8
STATE s_FireballExp[] =
{
    {FIREBALL_DISS + 0, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[1]},
    {FIREBALL_DISS + 1, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[2]},
    {FIREBALL_DISS + 2, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[3]},
    {FIREBALL_DISS + 3, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[4]},
    {FIREBALL_DISS + 4, FIREBALL_DISS_RATE, NullAnimator, &s_FireballExp[5]},
    {FIREBALL_DISS + 4, 100, DoSuicide, &s_FireballExp[0]}
};

#define NAP_EXP (3072)
#define NAP_EXP_RATE 6

STATE s_NapExp[] =
{
    {NAP_EXP + 0,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[1]},
    {NAP_EXP + 0,     0 | SF_QUICK_CALL,       DoDamageTest,  &s_NapExp[2]},
    {NAP_EXP + 1,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[3]},
    {NAP_EXP + 2,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[4]},
    {NAP_EXP + 3,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[5]},
    {NAP_EXP + 4,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[6]},
    {NAP_EXP + 5,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[7]},
    {NAP_EXP + 6,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[8]},
    {NAP_EXP + 7,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[9]},
    {NAP_EXP + 8,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[10]},
    {NAP_EXP + 9,     NAP_EXP_RATE,      NullAnimator,  &s_NapExp[11]},
    {NAP_EXP + 10,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[12]},
    {NAP_EXP + 11,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[13]},
    {NAP_EXP + 12,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[14]},
    {NAP_EXP + 13,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[15]},
    {NAP_EXP + 14,    NAP_EXP_RATE,      NullAnimator,  &s_NapExp[16]},
    {NAP_EXP + 15,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[17]},
    {NAP_EXP + 16,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[18]},
    {NAP_EXP + 17,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[19]},
    {NAP_EXP + 18,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[20]},
    {NAP_EXP + 19,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[21]},
    {NAP_EXP + 20,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[22]},
    {NAP_EXP + 21,    NAP_EXP_RATE-2,    NullAnimator,  &s_NapExp[23]},
    {NAP_EXP + 21,    NAP_EXP_RATE-2,    DoSuicide,     &s_NapExp[23]}
};

ANIMATOR DoFireballFlames;
#define FLAME_RATE 6

STATE s_FireballFlames[] =
{
    {FIREBALL_FLAMES + 0, FLAME_RATE, DoFireballFlames, &s_FireballFlames[1]},
    {FIREBALL_FLAMES + 1, FLAME_RATE, DoFireballFlames, &s_FireballFlames[2]},
    {FIREBALL_FLAMES + 2, FLAME_RATE, DoFireballFlames, &s_FireballFlames[3]},
    {FIREBALL_FLAMES + 3, FLAME_RATE, DoFireballFlames, &s_FireballFlames[4]},
    {FIREBALL_FLAMES + 4, FLAME_RATE, DoFireballFlames, &s_FireballFlames[5]},
    {FIREBALL_FLAMES + 5, FLAME_RATE, DoFireballFlames, &s_FireballFlames[6]},
    {FIREBALL_FLAMES + 6, FLAME_RATE, DoFireballFlames, &s_FireballFlames[7]},
    {FIREBALL_FLAMES + 7, FLAME_RATE, DoFireballFlames, &s_FireballFlames[8]},
    {FIREBALL_FLAMES + 8, FLAME_RATE, DoFireballFlames, &s_FireballFlames[9]},
    {FIREBALL_FLAMES + 9, FLAME_RATE, DoFireballFlames, &s_FireballFlames[10]},
    {FIREBALL_FLAMES +10, FLAME_RATE, DoFireballFlames, &s_FireballFlames[11]},
    {FIREBALL_FLAMES +11, FLAME_RATE, DoFireballFlames, &s_FireballFlames[12]},
    {FIREBALL_FLAMES +12, FLAME_RATE, DoFireballFlames, &s_FireballFlames[13]},
    {FIREBALL_FLAMES +13, FLAME_RATE, DoFireballFlames, &s_FireballFlames[0]},
};

ANIMATOR DoBreakFlames;
#define FLAME_RATE 6

STATE s_BreakFlames[] =
{
    {FIREBALL_FLAMES + 0, FLAME_RATE, DoBreakFlames, &s_BreakFlames[1]},
    {FIREBALL_FLAMES + 1, FLAME_RATE, DoBreakFlames, &s_BreakFlames[2]},
    {FIREBALL_FLAMES + 2, FLAME_RATE, DoBreakFlames, &s_BreakFlames[3]},
    {FIREBALL_FLAMES + 3, FLAME_RATE, DoBreakFlames, &s_BreakFlames[4]},
    {FIREBALL_FLAMES + 4, FLAME_RATE, DoBreakFlames, &s_BreakFlames[5]},
    {FIREBALL_FLAMES + 5, FLAME_RATE, DoBreakFlames, &s_BreakFlames[6]},
    {FIREBALL_FLAMES + 6, FLAME_RATE, DoBreakFlames, &s_BreakFlames[7]},
    {FIREBALL_FLAMES + 7, FLAME_RATE, DoBreakFlames, &s_BreakFlames[8]},
    {FIREBALL_FLAMES + 8, FLAME_RATE, DoBreakFlames, &s_BreakFlames[9]},
    {FIREBALL_FLAMES + 9, FLAME_RATE, DoBreakFlames, &s_BreakFlames[10]},
    {FIREBALL_FLAMES +10, FLAME_RATE, DoBreakFlames, &s_BreakFlames[11]},
    {FIREBALL_FLAMES +11, FLAME_RATE, DoBreakFlames, &s_BreakFlames[12]},
    {FIREBALL_FLAMES +12, FLAME_RATE, DoBreakFlames, &s_BreakFlames[13]},
    {FIREBALL_FLAMES +13, FLAME_RATE, DoBreakFlames, &s_BreakFlames[0]},
};

//////////////////////
//
// FIREBALL
//
//////////////////////

#if 1
ANIMATOR DoFireball;
#define FIREBALL_RATE 8
#define GORO_FIREBALL FIREBALL+1

STATE s_Fireball[] =
{
    {FIREBALL + 0, 12, DoFireball, &s_Fireball[1]},
    {FIREBALL + 1, 12, DoFireball, &s_Fireball[2]},
    {FIREBALL + 2, 12, DoFireball, &s_Fireball[3]},
    {FIREBALL + 3, 12, DoFireball, &s_Fireball[0]}
};

#else
#define GORO_FIREBALL FIREBALL_R0
#define FIREBALL_RATE 6
ANIMATOR DoFireball;

STATE s_Fireball[5][4] =
{
    {
        {FIREBALL_R0 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[0][1]},
        {FIREBALL_R0 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[0][2]},
        {FIREBALL_R0 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[0][3]},
        {FIREBALL_R0 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[0][0]},
    },
    {
        {FIREBALL_R1 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[1][1]},
        {FIREBALL_R1 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[1][2]},
        {FIREBALL_R1 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[1][3]},
        {FIREBALL_R1 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[1][0]},
    },
    {
        {FIREBALL_R2 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[2][1]},
        {FIREBALL_R2 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[2][2]},
        {FIREBALL_R2 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[2][3]},
        {FIREBALL_R2 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[2][0]},
    },
    {
        {FIREBALL_R3 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[3][1]},
        {FIREBALL_R3 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[3][2]},
        {FIREBALL_R3 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[3][3]},
        {FIREBALL_R3 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[3][0]},
    },
    {
        {FIREBALL_R4 + 0, FIREBALL_RATE, DoFireball, &s_Fireball[4][1]},
        {FIREBALL_R4 + 1, FIREBALL_RATE, DoFireball, &s_Fireball[4][2]},
        {FIREBALL_R4 + 2, FIREBALL_RATE, DoFireball, &s_Fireball[4][3]},
        {FIREBALL_R4 + 3, FIREBALL_RATE, DoFireball, &s_Fireball[4][0]},
    }
};

STATE* sg_Fireball[] =
{
    s_Fireball[0],
    s_Fireball[1],
    s_Fireball[2],
    s_Fireball[3],
    s_Fireball[4]
};
#endif

#if 0
ANIMATOR DoRing;

STATE s_Ring[5][4] =
{
    {
        {FIREBALL_R0 + 0, FIREBALL_RATE, DoRing, &s_Ring[0][1]},
        {FIREBALL_R0 + 1, FIREBALL_RATE, DoRing, &s_Ring[0][2]},
        {FIREBALL_R0 + 2, FIREBALL_RATE, DoRing, &s_Ring[0][3]},
        {FIREBALL_R0 + 3, FIREBALL_RATE, DoRing, &s_Ring[0][0]},
    },
    {
        {FIREBALL_R1 + 0, FIREBALL_RATE, DoRing, &s_Ring[1][1]},
        {FIREBALL_R1 + 1, FIREBALL_RATE, DoRing, &s_Ring[1][2]},
        {FIREBALL_R1 + 2, FIREBALL_RATE, DoRing, &s_Ring[1][3]},
        {FIREBALL_R1 + 3, FIREBALL_RATE, DoRing, &s_Ring[1][0]},
    },
    {
        {FIREBALL_R2 + 0, FIREBALL_RATE, DoRing, &s_Ring[2][1]},
        {FIREBALL_R2 + 1, FIREBALL_RATE, DoRing, &s_Ring[2][2]},
        {FIREBALL_R2 + 2, FIREBALL_RATE, DoRing, &s_Ring[2][3]},
        {FIREBALL_R2 + 3, FIREBALL_RATE, DoRing, &s_Ring[2][0]},
    },
    {
        {FIREBALL_R3 + 0, FIREBALL_RATE, DoRing, &s_Ring[3][1]},
        {FIREBALL_R3 + 1, FIREBALL_RATE, DoRing, &s_Ring[3][2]},
        {FIREBALL_R3 + 2, FIREBALL_RATE, DoRing, &s_Ring[3][3]},
        {FIREBALL_R3 + 3, FIREBALL_RATE, DoRing, &s_Ring[3][0]},
    },
    {
        {FIREBALL_R4 + 0, FIREBALL_RATE, DoRing, &s_Ring[4][1]},
        {FIREBALL_R4 + 1, FIREBALL_RATE, DoRing, &s_Ring[4][2]},
        {FIREBALL_R4 + 2, FIREBALL_RATE, DoRing, &s_Ring[4][3]},
        {FIREBALL_R4 + 3, FIREBALL_RATE, DoRing, &s_Ring[4][0]},
    }
};

STATE* sg_Ring[] =
{
    s_Ring[0],
    s_Ring[1],
    s_Ring[2],
    s_Ring[3],
    s_Ring[4]
};
#else
ANIMATOR DoRing;

STATE s_Ring[] =
{
    {FIREBALL + 0, 12, DoRing, &s_Ring[1]},
    {FIREBALL + 1, 12, DoRing, &s_Ring[2]},
    {FIREBALL + 2, 12, DoRing, &s_Ring[3]},
    {FIREBALL + 3, 12, DoRing, &s_Ring[0]}
};
#endif

STATE s_Ring2[] =
{
    {2031 + 0, 12, DoRing, &s_Ring2[1]},
    {2031 + 1, 12, DoRing, &s_Ring2[2]},
    {2031 + 2, 12, DoRing, &s_Ring2[3]},
    {2031 + 3, 12, DoRing, &s_Ring2[0]}
};

ANIMATOR DoNapalm;

STATE s_Napalm[] =
{
    {FIREBALL + 0, 12, DoNapalm, &s_Napalm[1]},
    {FIREBALL + 1, 12, DoNapalm, &s_Napalm[2]},
    {FIREBALL + 2, 12, DoNapalm, &s_Napalm[3]},
    {FIREBALL + 3, 12, DoNapalm, &s_Napalm[0]}
};


ANIMATOR DoBloodWorm;

#if 1
#define BLOOD_WORM 2106
STATE s_BloodWorm[] =
{
    {BLOOD_WORM + 0, 12, DoBloodWorm, &s_BloodWorm[1]},
    {BLOOD_WORM + 1, 12, DoBloodWorm, &s_BloodWorm[2]},
    {BLOOD_WORM + 2, 12, DoBloodWorm, &s_BloodWorm[3]},
    {BLOOD_WORM + 3, 12, DoBloodWorm, &s_BloodWorm[4]},
    {BLOOD_WORM + 2, 12, DoBloodWorm, &s_BloodWorm[5]},
    {BLOOD_WORM + 1, 12, DoBloodWorm, &s_BloodWorm[0]}
};
#else
#define BLOOD_WORM FIREBALL+5
STATE s_BloodWorm[] =
{
    {FIREBALL + 0, 12, DoBloodWorm, &s_BloodWorm[1]},
    {FIREBALL + 1, 12, DoBloodWorm, &s_BloodWorm[2]},
    {FIREBALL + 2, 12, DoBloodWorm, &s_BloodWorm[3]},
    {FIREBALL + 3, 12, DoBloodWorm, &s_BloodWorm[0]}
};
#endif

#if 1
#define PLASMA_EXP BLOOD_WORM+1
#define PLASMA_EXP_RATE 8

STATE s_PlasmaExp[] =
{
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[1]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[2]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[3]},
    {BLOOD_WORM + 3, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[4]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[5]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[6]},
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[7]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[8]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[9]},
    {BLOOD_WORM + 3, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[10]},
    {BLOOD_WORM + 2, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[11]},
    {BLOOD_WORM + 1, PLASMA_EXP_RATE, NullAnimator, &s_PlasmaExp[12]},
    {BLOOD_WORM + 0, PLASMA_EXP_RATE, DoSuicide, &s_PlasmaExp[12]},
};

#else
#define PLASMA_EXP (NAP_EXP+1)
#define PLASMA_EXP_RATE 4

STATE s_PlasmaExp[] =
{
    {PLASMA_EXP + 0,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[1]},
    {PLASMA_EXP + 0,     0 | SF_QUICK_CALL,    DoDamageTest,  &s_PlasmaExp[2]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[3]},
    {PLASMA_EXP + 2,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[4]},
    {PLASMA_EXP + 3,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[5]},
    {PLASMA_EXP + 4,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[6]},
    {PLASMA_EXP + 5,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[7]},
    {PLASMA_EXP + 6,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[8]},
    {PLASMA_EXP + 7,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[9]},
    {PLASMA_EXP + 8,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[10]},
    {PLASMA_EXP + 9,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[11]},
    {PLASMA_EXP + 10,    PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[12]},
    {PLASMA_EXP + 11,    PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[13]},
    {PLASMA_EXP + 9,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[14]},
    {PLASMA_EXP + 8,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[15]},
    {PLASMA_EXP + 7,     PLASMA_EXP_RATE,      NullAnimator,  &s_PlasmaExp[16]},
    {PLASMA_EXP + 6,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[17]},
    {PLASMA_EXP + 5,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[18]},
    {PLASMA_EXP + 4,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[19]},
    {PLASMA_EXP + 3,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[20]},
    {PLASMA_EXP + 2,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[21]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[22]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    NullAnimator,  &s_PlasmaExp[23]},
    {PLASMA_EXP + 1,     PLASMA_EXP_RATE-2,    DoSuicide,     &s_PlasmaExp[23]}
};
#endif


ANIMATOR DoMirv;

STATE s_Mirv[] =
{
    {FIREBALL + 0, 12, DoMirv, &s_Mirv[1]},
    {FIREBALL + 1, 12, DoMirv, &s_Mirv[2]},
    {FIREBALL + 2, 12, DoMirv, &s_Mirv[3]},
    {FIREBALL + 3, 12, DoMirv, &s_Mirv[0]}
};

ANIMATOR DoMirvMissile;

STATE s_MirvMissile[] =
{
    {FIREBALL + 0, 12, DoMirvMissile, &s_MirvMissile[1]},
    {FIREBALL + 1, 12, DoMirvMissile, &s_MirvMissile[2]},
    {FIREBALL + 2, 12, DoMirvMissile, &s_MirvMissile[3]},
    {FIREBALL + 3, 12, DoMirvMissile, &s_MirvMissile[0]}
};

//#define Vomit1 1740
//#define Vomit2 1741
#define Vomit1 1719
#define Vomit2 1721
//#define VomitSplash 1742
#define VomitSplash 1711
#define Vomit_RATE 16
ANIMATOR DoVomit,DoVomitSplash;

STATE s_Vomit1[] =
{
    {Vomit1 + 0, Vomit_RATE, DoVomit, &s_Vomit1[0]}
};

STATE s_Vomit2[] =
{
    {Vomit2 + 0, Vomit_RATE, DoVomit, &s_Vomit2[0]}
};

STATE s_VomitSplash[] =
{
    {VomitSplash + 0, Vomit_RATE, DoVomitSplash, &s_VomitSplash[0]}
};

#define GORE_Head 1670
#define GORE_Head_RATE 16

STATE s_GoreHead[] =
{
    {GORE_Head + 0, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[1]},
    {GORE_Head + 1, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[2]},
    {GORE_Head + 2, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[3]},
    {GORE_Head + 3, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[4]},
    {GORE_Head + 4, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[5]},
    {GORE_Head + 5, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[6]},
    {GORE_Head + 6, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[7]},
    {GORE_Head + 7, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[8]},
    {GORE_Head + 8, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[9]},
    {GORE_Head + 9, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[10]},
    {GORE_Head + 10, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[11]},
    {GORE_Head + 11, GORE_Head_RATE, DoShrapJumpFall, &s_GoreHead[0]},
};

#define GORE_Leg 1689
#define GORE_Leg_RATE 16

STATE s_GoreLeg[] =
{
    {GORE_Leg + 0, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[1]},
    {GORE_Leg + 1, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[2]},
    {GORE_Leg + 2, GORE_Leg_RATE, DoShrapJumpFall, &s_GoreLeg[0]},
};

#define GORE_Eye 1692
#define GORE_Eye_RATE 16

STATE s_GoreEye[] =
{
    {GORE_Eye + 0, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[1]},
    {GORE_Eye + 1, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[2]},
    {GORE_Eye + 2, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[3]},
    {GORE_Eye + 3, GORE_Eye_RATE, DoShrapJumpFall, &s_GoreEye[0]},
};

#define GORE_Torso 1696
#define GORE_Torso_RATE 16

STATE s_GoreTorso[] =
{
    {GORE_Torso + 0, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[1]},
    {GORE_Torso + 1, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[2]},
    {GORE_Torso + 2, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[3]},
    {GORE_Torso + 3, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[4]},
    {GORE_Torso + 4, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[5]},
    {GORE_Torso + 5, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[6]},
    {GORE_Torso + 6, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[7]},
    {GORE_Torso + 7, GORE_Torso_RATE, DoShrapJumpFall, &s_GoreTorso[0]},
};


#define GORE_Arm 1550
#define GORE_Arm_RATE 16

STATE s_GoreArm[] =
{
    {GORE_Arm + 0, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[1]},
    {GORE_Arm + 1, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[2]},
    {GORE_Arm + 2, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[3]},
    {GORE_Arm + 3, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[4]},
    {GORE_Arm + 4, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[5]},
    {GORE_Arm + 5, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[6]},
    {GORE_Arm + 6, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[7]},
    {GORE_Arm + 7, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[8]},
    {GORE_Arm + 8, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[9]},
    {GORE_Arm + 9, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[10]},
    {GORE_Arm + 10, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[11]},
    {GORE_Arm + 11, GORE_Arm_RATE, DoShrapJumpFall, &s_GoreArm[0]},
};

#define GORE_Lung 903
#define GORE_Lung_RATE 16

STATE s_GoreLung[] =
{
    {GORE_Lung + 0, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[1]},
    {GORE_Lung + 1, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[2]},
    {GORE_Lung + 2, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[3]},
    {GORE_Lung + 3, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[4]},
    {GORE_Lung + 4, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[5]},
    {GORE_Lung + 5, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[6]},
    {GORE_Lung + 6, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[7]},
    {GORE_Lung + 7, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[8]},
    {GORE_Lung + 8, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[9]},
    {GORE_Lung + 9, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[10]},
    {GORE_Lung + 10, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[11]},
    {GORE_Lung + 11, GORE_Lung_RATE, DoShrapJumpFall, &s_GoreLung[0]},
};

#define GORE_Liver 918
#define GORE_Liver_RATE 16

STATE s_GoreLiver[] =
{
    {GORE_Liver + 0, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[1]},
    {GORE_Liver + 1, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[2]},
    {GORE_Liver + 2, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[3]},
    {GORE_Liver + 3, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[4]},
    {GORE_Liver + 4, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[5]},
    {GORE_Liver + 5, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[6]},
    {GORE_Liver + 6, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[7]},
    {GORE_Liver + 7, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[8]},
    {GORE_Liver + 8, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[9]},
    {GORE_Liver + 9, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[10]},
    {GORE_Liver + 10, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[11]},
    {GORE_Liver + 11, GORE_Liver_RATE, DoShrapJumpFall, &s_GoreLiver[0]},
};

#define GORE_SkullCap 933
#define GORE_SkullCap_RATE 16

STATE s_GoreSkullCap[] =
{
    {GORE_SkullCap + 0, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[1]},
    {GORE_SkullCap + 1, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[2]},
    {GORE_SkullCap + 2, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[3]},
    {GORE_SkullCap + 3, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[4]},
    {GORE_SkullCap + 4, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[5]},
    {GORE_SkullCap + 5, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[6]},
    {GORE_SkullCap + 6, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[7]},
    {GORE_SkullCap + 7, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[8]},
    {GORE_SkullCap + 8, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[9]},
    {GORE_SkullCap + 9, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[10]},
    {GORE_SkullCap + 10, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[11]},
    {GORE_SkullCap + 11, GORE_SkullCap_RATE, DoShrapJumpFall, &s_GoreSkullCap[0]},
};


#define GORE_ChunkS 2430
#define GORE_ChunkS_RATE 16

STATE s_GoreChunkS[] =
{
    {GORE_ChunkS + 0, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[1]},
    {GORE_ChunkS + 1, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[2]},
    {GORE_ChunkS + 2, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[3]},
    {GORE_ChunkS + 3, GORE_ChunkS_RATE, DoShrapJumpFall, &s_GoreChunkS[0]},
};

#define GORE_Drip 1562 //2430
#define GORE_Drip_RATE 16

STATE s_GoreDrip[] =
{
    {GORE_Drip + 0, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[1]},
    {GORE_Drip + 1, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[2]},
    {GORE_Drip + 2, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[3]},
    {GORE_Drip + 3, GORE_Drip_RATE, DoShrapJumpFall, &s_GoreDrip[0]},
};

STATE s_FastGoreDrip[] =
{
    {GORE_Drip + 0, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[1]},
    {GORE_Drip + 1, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[2]},
    {GORE_Drip + 2, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[3]},
    {GORE_Drip + 3, GORE_Drip_RATE, DoFastShrapJumpFall, &s_FastGoreDrip[0]},
};

///////////////////////////////////////////////
//
// This GORE mostly for the Accursed Heads
//
///////////////////////////////////////////////

#define GORE_Flame 847
#define GORE_Flame_RATE 8

STATE s_GoreFlame[] =
{
    {GORE_Flame + 0, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[1]},
    {GORE_Flame + 1, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[2]},
    {GORE_Flame + 2, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[3]},
    {GORE_Flame + 3, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[4]},
    {GORE_Flame + 4, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[5]},
    {GORE_Flame + 5, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[6]},
    {GORE_Flame + 6, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[7]},
    {GORE_Flame + 7, GORE_Flame_RATE, DoFastShrapJumpFall, &s_GoreFlame[0]},
};

ANIMATOR DoTracerShrap;
STATE s_TracerShrap[] =
{
    {GORE_Flame + 0, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[1]},
    {GORE_Flame + 1, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[2]},
    {GORE_Flame + 2, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[3]},
    {GORE_Flame + 3, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[4]},
    {GORE_Flame + 4, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[5]},
    {GORE_Flame + 5, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[6]},
    {GORE_Flame + 6, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[7]},
    {GORE_Flame + 7, GORE_Flame_RATE, DoTracerShrap, &s_TracerShrap[0]},
};

#define UZI_SHELL 2152
#define UZISHELL_RATE 8
//ANIMATOR DoShellShrap;
STATE s_UziShellShrap[] =
{
    {UZI_SHELL + 0, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[1]},
    {UZI_SHELL + 1, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[2]},
    {UZI_SHELL + 2, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[3]},
    {UZI_SHELL + 3, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[4]},
    {UZI_SHELL + 4, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[5]},
    {UZI_SHELL + 5, UZISHELL_RATE, DoShrapJumpFall, &s_UziShellShrap[0]},
};

STATE s_UziShellShrapStill1[] =
{
    {UZI_SHELL + 0, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill1[0]}
};
STATE s_UziShellShrapStill2[] =
{
    {UZI_SHELL + 1, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill2[0]}
};
STATE s_UziShellShrapStill3[] =
{
    {UZI_SHELL + 2, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill3[0]}
};
STATE s_UziShellShrapStill4[] =
{
    {UZI_SHELL + 3, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill4[0]}
};
STATE s_UziShellShrapStill5[] =
{
    {UZI_SHELL + 4, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill5[0]}
};
STATE s_UziShellShrapStill6[] =
{
    {UZI_SHELL + 5, UZISHELL_RATE, NullAnimator, &s_UziShellShrapStill6[0]}
};

#define SHOT_SHELL 2180
#define SHOTSHELL_RATE 8
STATE s_ShotgunShellShrap[] =
{
    {SHOT_SHELL + 0, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[1]},
    {SHOT_SHELL + 1, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[2]},
    {SHOT_SHELL + 2, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[3]},
    {SHOT_SHELL + 3, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[4]},
    {SHOT_SHELL + 4, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[5]},
    {SHOT_SHELL + 5, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[6]},
    {SHOT_SHELL + 6, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[7]},
    {SHOT_SHELL + 7, SHOTSHELL_RATE, DoShrapJumpFall, &s_ShotgunShellShrap[0]},
};

STATE s_ShotgunShellShrapStill1[] =
{
    {SHOT_SHELL + 1, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill1[0]}
};
STATE s_ShotgunShellShrapStill2[] =
{
    {SHOT_SHELL + 3, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill2[0]}
};
STATE s_ShotgunShellShrapStill3[] =
{
    {SHOT_SHELL + 7, SHOTSHELL_RATE, NullAnimator, &s_ShotgunShellShrapStill3[0]}
};

#define GORE_FlameChunkA 839
#define GORE_FlameChunkA_RATE 8

STATE s_GoreFlameChunkA[] =
{
    {GORE_FlameChunkA + 0, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[1]},
    {GORE_FlameChunkA + 1, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[2]},
    {GORE_FlameChunkA + 2, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[3]},
    {GORE_FlameChunkA + 3, GORE_FlameChunkA_RATE, DoShrapJumpFall, &s_GoreFlameChunkA[0]},
};

#define GORE_FlameChunkB 843
#define GORE_FlameChunkB_RATE 8

STATE s_GoreFlameChunkB[] =
{
    {GORE_FlameChunkB + 0, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[1]},
    {GORE_FlameChunkB + 1, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[2]},
    {GORE_FlameChunkB + 2, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[3]},
    {GORE_FlameChunkB + 3, GORE_FlameChunkB_RATE, DoShrapJumpFall, &s_GoreFlameChunkB[0]},
};

/////////////////////////////////////////////////////////////////////
//
// General Breaking Shrapnel
//
/////////////////////////////////////////////////////////////////////

#define COIN_SHRAP 2530
#define CoinShrap_RATE 12

STATE s_CoinShrap[] =
{
    {COIN_SHRAP + 0, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[1]},
    {COIN_SHRAP + 1, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[2]},
    {COIN_SHRAP + 2, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[3]},
    {COIN_SHRAP + 3, CoinShrap_RATE, DoShrapJumpFall, &s_CoinShrap[0]},
};

#define MARBEL 5096
#define Marbel_RATE 12

STATE s_Marbel[] =
{
    {MARBEL, Marbel_RATE, DoShrapJumpFall, &s_Marbel[0]},
};

//
// Glass
//

#define GLASS_SHRAP_A 3864
#define GlassShrapA_RATE 12

STATE s_GlassShrapA[] =
{
    {GLASS_SHRAP_A + 0, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[1]},
    {GLASS_SHRAP_A + 1, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[2]},
    {GLASS_SHRAP_A + 2, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[3]},
    {GLASS_SHRAP_A + 3, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[4]},
    {GLASS_SHRAP_A + 4, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[5]},
    {GLASS_SHRAP_A + 5, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[6]},
    {GLASS_SHRAP_A + 6, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[7]},
    {GLASS_SHRAP_A + 7, GlassShrapA_RATE, DoShrapJumpFall, &s_GlassShrapA[0]},
};

#define GLASS_SHRAP_B 3872
#define GlassShrapB_RATE 12

STATE s_GlassShrapB[] =
{
    {GLASS_SHRAP_B + 0, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[1]},
    {GLASS_SHRAP_B + 1, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[2]},
    {GLASS_SHRAP_B + 2, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[3]},
    {GLASS_SHRAP_B + 3, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[4]},
    {GLASS_SHRAP_B + 4, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[5]},
    {GLASS_SHRAP_B + 5, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[6]},
    {GLASS_SHRAP_B + 6, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[7]},
    {GLASS_SHRAP_B + 7, GlassShrapB_RATE, DoShrapJumpFall, &s_GlassShrapB[0]},
};

#define GLASS_SHRAP_C 3880
#define GlassShrapC_RATE 12

STATE s_GlassShrapC[] =
{
    {GLASS_SHRAP_C + 0, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[1]},
    {GLASS_SHRAP_C + 1, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[2]},
    {GLASS_SHRAP_C + 2, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[3]},
    {GLASS_SHRAP_C + 3, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[4]},
    {GLASS_SHRAP_C + 4, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[5]},
    {GLASS_SHRAP_C + 5, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[6]},
    {GLASS_SHRAP_C + 6, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[7]},
    {GLASS_SHRAP_C + 7, GlassShrapC_RATE, DoShrapJumpFall, &s_GlassShrapC[0]},
};

//
// Wood
//

#define WOOD_SHRAP_A 3924
#define WoodShrapA_RATE 12

STATE s_WoodShrapA[] =
{
    {WOOD_SHRAP_A + 0, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[1]},
    {WOOD_SHRAP_A + 1, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[2]},
    {WOOD_SHRAP_A + 2, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[3]},
    {WOOD_SHRAP_A + 3, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[4]},
    {WOOD_SHRAP_A + 4, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[5]},
    {WOOD_SHRAP_A + 5, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[6]},
    {WOOD_SHRAP_A + 6, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[7]},
    {WOOD_SHRAP_A + 7, WoodShrapA_RATE, DoShrapJumpFall, &s_WoodShrapA[0]},
};

#define WOOD_SHRAP_B 3932
#define WoodShrapB_RATE 12

STATE s_WoodShrapB[] =
{
    {WOOD_SHRAP_B + 0, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[1]},
    {WOOD_SHRAP_B + 1, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[2]},
    {WOOD_SHRAP_B + 2, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[3]},
    {WOOD_SHRAP_B + 3, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[4]},
    {WOOD_SHRAP_B + 4, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[5]},
    {WOOD_SHRAP_B + 5, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[6]},
    {WOOD_SHRAP_B + 6, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[7]},
    {WOOD_SHRAP_B + 7, WoodShrapB_RATE, DoShrapJumpFall, &s_WoodShrapB[0]},
};

#define WOOD_SHRAP_C 3941
#define WoodShrapC_RATE 12

STATE s_WoodShrapC[] =
{
    {WOOD_SHRAP_C + 0, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[1]},
    {WOOD_SHRAP_C + 1, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[2]},
    {WOOD_SHRAP_C + 2, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[3]},
    {WOOD_SHRAP_C + 3, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[4]},
    {WOOD_SHRAP_C + 4, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[5]},
    {WOOD_SHRAP_C + 5, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[6]},
    {WOOD_SHRAP_C + 6, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[7]},
    {WOOD_SHRAP_C + 7, WoodShrapC_RATE, DoShrapJumpFall, &s_WoodShrapC[0]},
};

//
// Stone
//

#define STONE_SHRAP_A 3840
#define StoneShrapA_RATE 12

STATE s_StoneShrapA[] =
{
    {STONE_SHRAP_A + 0, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[1]},
    {STONE_SHRAP_A + 1, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[2]},
    {STONE_SHRAP_A + 2, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[3]},
    {STONE_SHRAP_A + 3, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[4]},
    {STONE_SHRAP_A + 4, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[5]},
    {STONE_SHRAP_A + 5, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[6]},
    {STONE_SHRAP_A + 6, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[7]},
    {STONE_SHRAP_A + 7, StoneShrapA_RATE, DoShrapJumpFall, &s_StoneShrapA[0]},
};

#define STONE_SHRAP_B 3848
#define StoneShrapB_RATE 12

STATE s_StoneShrapB[] =
{
    {STONE_SHRAP_B + 0, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[1]},
    {STONE_SHRAP_B + 1, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[2]},
    {STONE_SHRAP_B + 2, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[3]},
    {STONE_SHRAP_B + 3, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[4]},
    {STONE_SHRAP_B + 4, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[5]},
    {STONE_SHRAP_B + 5, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[6]},
    {STONE_SHRAP_B + 6, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[7]},
    {STONE_SHRAP_B + 7, StoneShrapB_RATE, DoShrapJumpFall, &s_StoneShrapB[0]},
};

#define STONE_SHRAP_C 3856
#define StoneShrapC_RATE 12

STATE s_StoneShrapC[] =
{
    {STONE_SHRAP_C + 0, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[1]},
    {STONE_SHRAP_C + 1, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[2]},
    {STONE_SHRAP_C + 2, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[3]},
    {STONE_SHRAP_C + 3, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[4]},
    {STONE_SHRAP_C + 4, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[5]},
    {STONE_SHRAP_C + 5, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[6]},
    {STONE_SHRAP_C + 6, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[7]},
    {STONE_SHRAP_C + 7, StoneShrapC_RATE, DoShrapJumpFall, &s_StoneShrapC[0]},
};

//
// Metal
//

#define METAL_SHRAP_A 3888
#define MetalShrapA_RATE 12

STATE s_MetalShrapA[] =
{
    {METAL_SHRAP_A + 0, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[1]},
    {METAL_SHRAP_A + 1, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[2]},
    {METAL_SHRAP_A + 2, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[3]},
    {METAL_SHRAP_A + 3, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[4]},
    {METAL_SHRAP_A + 4, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[5]},
    {METAL_SHRAP_A + 5, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[6]},
    {METAL_SHRAP_A + 6, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[7]},
    {METAL_SHRAP_A + 7, MetalShrapA_RATE, DoShrapJumpFall, &s_MetalShrapA[0]},
};

#define METAL_SHRAP_B 3896
#define MetalShrapB_RATE 12

STATE s_MetalShrapB[] =
{
    {METAL_SHRAP_B + 0, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[1]},
    {METAL_SHRAP_B + 1, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[2]},
    {METAL_SHRAP_B + 2, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[3]},
    {METAL_SHRAP_B + 3, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[4]},
    {METAL_SHRAP_B + 4, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[5]},
    {METAL_SHRAP_B + 5, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[6]},
    {METAL_SHRAP_B + 6, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[7]},
    {METAL_SHRAP_B + 7, MetalShrapB_RATE, DoShrapJumpFall, &s_MetalShrapB[0]},
};

#define METAL_SHRAP_C 3904
#define MetalShrapC_RATE 12

STATE s_MetalShrapC[] =
{
    {METAL_SHRAP_C + 0, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[1]},
    {METAL_SHRAP_C + 1, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[2]},
    {METAL_SHRAP_C + 2, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[3]},
    {METAL_SHRAP_C + 3, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[4]},
    {METAL_SHRAP_C + 4, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[5]},
    {METAL_SHRAP_C + 5, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[6]},
    {METAL_SHRAP_C + 6, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[7]},
    {METAL_SHRAP_C + 7, MetalShrapC_RATE, DoShrapJumpFall, &s_MetalShrapC[0]},
};

//
// Paper
//

#define PAPER_SHRAP_A 3924
#define PaperShrapA_RATE 12

STATE s_PaperShrapA[] =
{
    {PAPER_SHRAP_A + 0, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[1]},
    {PAPER_SHRAP_A + 1, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[2]},
    {PAPER_SHRAP_A + 2, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[3]},
    {PAPER_SHRAP_A + 3, PaperShrapA_RATE, DoShrapJumpFall, &s_PaperShrapA[0]},
};

#define PAPER_SHRAP_B 3932
#define PaperShrapB_RATE 12

STATE s_PaperShrapB[] =
{
    {PAPER_SHRAP_B + 0, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[1]},
    {PAPER_SHRAP_B + 1, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[2]},
    {PAPER_SHRAP_B + 2, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[3]},
    {PAPER_SHRAP_B + 3, PaperShrapB_RATE, DoShrapJumpFall, &s_PaperShrapB[0]},
};

#define PAPER_SHRAP_C 3941
#define PaperShrapC_RATE 12

STATE s_PaperShrapC[] =
{
    {PAPER_SHRAP_C + 0, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[1]},
    {PAPER_SHRAP_C + 1, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[2]},
    {PAPER_SHRAP_C + 2, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[3]},
    {PAPER_SHRAP_C + 3, PaperShrapC_RATE, DoShrapJumpFall, &s_PaperShrapC[0]},
};

bool MissileHitMatch(DSWActor* weapActor, int WeaponNum, DSWActor* hitActor)
{
    if (WeaponNum <= -1)
    {
        ASSERT(weapActor != nullptr);
        WeaponNum = weapActor->user.WeaponNum;

        // can be hit by SO only
        if (SP_TAG7(hitActor) == 4)
        {
            if ((weapActor->user.Flags2 & SPR2_SO_MISSILE))
            {
                DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    if (SP_TAG7(hitActor) == 0)
    {
        switch (WeaponNum)
        {
        case WPN_RAIL:
        case WPN_MICRO:
        case WPN_NAPALM:
        case WPN_ROCKET:
            DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
            return true;
        }
    }
    else if (SP_TAG7(hitActor) == 1)
    {
        switch (WeaponNum)
        {
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
            DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
            return true;
        }
    }
    else if (SP_TAG7(hitActor) == 2)
    {
        switch (WeaponNum)
        {
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
        case WPN_UZI:
        case WPN_SHOTGUN:
            DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
            return true;
        }
    }
    else if (SP_TAG7(hitActor) == 3)
    {
        switch (WeaponNum)
        {
        case WPN_STAR:
        case WPN_SWORD:
        case WPN_FIST:
        case WPN_MICRO:
        case WPN_RAIL:
        case WPN_HOTHEAD:
        case WPN_NAPALM:
        case WPN_ROCKET:
        case WPN_UZI:
        case WPN_SHOTGUN:
            DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
            return true;
        }
    }

    return false;

#if 0
    WPN_STAR
    WPN_UZI
    WPN_SHOTGUN
    WPN_MICRO
    WPN_GRENADE
    WPN_MINE
    WPN_RAIL
    WPN_HEART
    WPN_HOTHEAD
    WPN_NAPALM
    WPN_RING
    WPN_ROCKET
    WPN_SWORD
        WPN_FIST
#endif
}

int SpawnShrapX(DSWActor* actor)
{
    //For shrap that has no Weapon to send over
    SpawnShrap(actor, nullptr);
    return 0;
}

int DoLavaErupt(DSWActor* actor)
{
    short i,pnum;
    PLAYER* pp;
    bool found = false;

    if (TEST_BOOL1(actor))
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;
            if (pp->insector() && (pp->cursector->extra & SECTFX_TRIGGER))
            {
                SWSectIterator it(pp->cursector);
                while (auto itActor = it.Next())
                {
                    if (itActor->spr.statnum == STAT_TRIGGER && SP_TAG7(itActor) == 0 && SP_TAG5(itActor) == 1)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }

        if (!found)
            return 0;
    }

    if (!(actor->user.Flags & SPR_ACTIVE))
    {
        // inactive
        if ((actor->user.WaitTics -= synctics) <= 0)
        {
            actor->user.Flags |= (SPR_ACTIVE);
            actor->user.Counter = 0;
            actor->user.WaitTics = SP_TAG9(actor) * 120L;
        }
    }
    else
    {
        // active
        if ((actor->user.WaitTics -= synctics) <= 0)
        {
            // Stop for this long
            actor->user.Flags &= ~(SPR_ACTIVE);
            actor->user.Counter = 0;
            actor->user.WaitTics = SP_TAG10(actor) * 120L;
        }

        // Counter controls the volume of lava erupting
        // starts out slow and increases to a max
        actor->user.Counter += synctics;
        if (actor->user.Counter > SP_TAG2(actor))
            actor->user.Counter = SP_TAG2(actor);

        if ((RANDOM_P2(1024<<6)>>6) < actor->user.Counter)
        {
            switch (SP_TAG3(actor))
            {
            case 0:
                SpawnShrapX(actor);
                break;
            case 1:
                InitVulcanBoulder(actor);
                break;
            }
        }
    }

    return 0;
}


int SpawnShrap(DSWActor* parentActor, DSWActor* secondaryActor, int means, BREAK_INFO* breakinfo)
{
    short i;

    /////////////////////////////////////////
    //
    // BREAK shrap types
    //
    /////////////////////////////////////////

    // Individual shraps can be copied to this and then values can be changed
    static SHRAP CustomShrap[20];

    static SHRAP CoinShrap[] =
    {
        {s_CoinShrap, COIN_SHRAP,      5, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP GlassShrap[] =
    {
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP WoodShrap[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP StoneShrap[] =
    {
        {s_StoneShrapA, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_StoneShrapB, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_StoneShrapC, STONE_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PaperShrap[] =
    {
        {s_PaperShrapA, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_PaperShrapB, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_PaperShrapC, PAPER_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP MetalShrap[] =
    {
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP MetalMix[] =
    {
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP WoodMix[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapA, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapB, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_MetalShrapC, METAL_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PaperMix[] =
    {
        {s_WoodShrapA, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapB, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_WoodShrapC, WOOD_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_PaperShrapA, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_PaperShrapB, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_PaperShrapC, PAPER_SHRAP_A, 2, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP Marbels[] =
    {
        {s_Marbel,      MARBEL,        5, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapA, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapB, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {s_GlassShrapC, GLASS_SHRAP_A, 1, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP BloodShrap[] =
    {
        {s_BloodShrap, BLOOD_SHRAP, 8, Z_MID, 200, 600, 100, 500, true, 2048},
        {nullptr},
    };
#endif

    ////
    // END - BREAK shrap types
    ////

    static SHRAP EMPShrap[] =
    {
        {s_EMPShrap, EMP, 1, Z_MID, 500, 1100, 300, 600, false, 128},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP StdShrap[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, true, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, true, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, true, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, true, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 250, 500, true, 2048},
        {s_FastGoreDrip, GORE_Drip, 8, Z_BOT, 600, 800, 50, 70, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP HeartAttackShrap[] = // fewer gibs because of the plasma fountain sprites
    {
        {s_GoreLung,     GORE_Lung,       2, Z_TOP, 500, 1100, 300, 600, true, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 500, 1100, 300, 500, true, 2048},
        {s_GoreArm,     GORE_Arm,       2, Z_MID, 500, 1100, 350, 600, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SkelGore[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, true, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, true, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, true, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 100, 150, true, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, true, 2048},
        {s_GoreLeg, GORE_Leg, 2, Z_BOT, 200, 400, 250, 500, true, 2048},
        {s_GoreChunkS, GORE_ChunkS, 4, Z_BOT, 200, 400, 250, 400, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP UpperGore[] =
    {
        {s_GoreHead, GORE_Head, 1, Z_TOP, 400, 700, 20, 40, true, 2048},
        {s_GoreLung, GORE_Lung, 2, Z_TOP, 500, 800, 100, 300, true, 2048},
        {s_GoreLiver, GORE_Liver, 1, Z_MID, 300, 500, 100, 150, true, 2048},
        {s_GoreSkullCap, GORE_SkullCap, 1, Z_TOP, 300, 500, 100, 150, true, 2048},
        {s_GoreArm, GORE_Arm, 1, Z_MID, 300, 500, 250, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP LowerGore[] =
    {
        {s_GoreLeg, GORE_Leg, 4, Z_BOT, 300, 500, 100, 200, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };
#endif

    static SHRAP SmallGore[] =
    {
        {s_GoreDrip, GORE_Drip, 3, Z_TOP, 600, 800, 50, 70, false, 2048},
        {s_FastGoreDrip, GORE_Drip, 3, Z_BOT, 600, 800, 70, 100, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FlamingGore[] =
    {
        {s_GoreFlame, GORE_Drip, 2, Z_TOP, 600, 800, 100, 200, false, 2048},
        {s_GoreFlameChunkB, GORE_Drip, 4, Z_MID, 300, 500, 100, 200, false, 2048},
        {s_GoreFlame, GORE_Drip, 2, Z_BOT, 100, 200, 100, 200, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP BoltExpShrap[] =
    {
        {s_GoreFlame, GORE_Drip, 4, Z_MID, 300, 700, 300, 600, true, 2048},
        {s_GoreFlame, GORE_Drip, 4, Z_BOT, 300, 700, 300, 600, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP TracerExpShrap[] =
    {
        {s_TracerShrap, GORE_Drip, 3, Z_MID, 300, 700, 300, 600, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FireballExpShrap1[] =
    {
        {s_GoreFlame, GORE_Drip, 1, Z_MID, 100, 300, 100, 200, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP FireballExpShrap2[] =
    {
        {s_GoreFlame, GORE_Drip, 2, Z_MID, 100, 300, 100, 200, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP* FireballExpShrap[] =
    {
        FireballExpShrap1,
        FireballExpShrap2
    };
#endif

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP ElectroShrap[] =
    {
        {s_ElectroShrap, ELECTRO_SHARD, 12, Z_TOP, 200, 600, 100, 500, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP LavaShrap1[] =
    {
        {s_GoreFlame, GORE_Drip, 1, Z_TOP, 400, 1400, 100, 400, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP LavaShrap2[] =
    {
        {s_GoreFlameChunkB, GORE_Drip, 1, Z_TOP, 400, 1400, 100, 400, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP *LavaShrapTable[] =
    {
        LavaShrap1,
        LavaShrap2
    };

    static SHRAP LavaBoulderShrap[] =
    {
        {s_LavaShard, LAVA_SHARD, 16, Z_MID, 400, 900, 200, 600, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP SectorSquishGore[] =
    {
        {s_FastGoreDrip,    GORE_Drip,   24, Z_MID, -400, -200, 600, 800, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };
#endif



    //
    // PLAYER SHRAP
    //

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP PlayerGoreFall[] =
    {
        {s_GoreSkullCap,GORE_SkullCap,  1, Z_TOP, 200, 300, 100, 200, true, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 200, 300, 100, 200, true, 2048},
        {s_GoreLung,   GORE_Lung,       1, Z_MID, 200, 300, 100, 200, true, 2048},
        {s_GoreDrip,    GORE_Drip,      10, Z_MID, 200, 300, 100, 200, false, 2048},
        {s_GoreArm,     GORE_Arm,       1, Z_MID, 200, 300, 100, 200, true, 2048},
        {s_FastGoreDrip,    GORE_Drip,      10, Z_BOT, 200, 300, 100, 200, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlayerGoreFly[] =
    {
        {s_GoreSkullCap,GORE_SkullCap,  1, Z_TOP, 500, 1100, 300, 600, true, 2048},
        {s_GoreTorso,   GORE_Torso,     1, Z_MID, 500, 1100, 300, 500, true, 2048},
        {s_GoreLiver,   GORE_Liver,     1, Z_MID, 200, 300, 100, 200, true, 2048},
        {s_GoreArm,     GORE_Arm,       1, Z_MID, 500, 1100, 350, 600, true, 2048},
        {s_FastGoreDrip,    GORE_Drip,      16, Z_MID, 500, 1100, 350, 600, false, 2048},
        {s_FastGoreDrip,    GORE_Drip,      16, Z_BOT, 500, 1100, 350, 600, false, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlayerDeadHead[] =
    {
        {s_GoreDrip, GORE_Drip, 2, Z_TOP, 150, 400, 40, 80, true, 2048},
        {s_GoreDrip, GORE_Drip, 2, Z_MID, 150, 400, 40, 80, true, 2048},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;
    static SHRAP PlayerHeadHurl1[] =
    {
        {s_Vomit1, Vomit1, 1, Z_BOT, 250, 400, 100, 200, true, 256},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#define WALL_FLOOR_SHRAP 4097
    ANIMATOR DoShrapWallFloor;
#if 0
    static SHRAP SectorExpShrap[] =
    {
        {nullptr, WALL_FLOOR_SHRAP, 1, Z_BOT, 550, 800, 200, 400, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };
#endif

    int hz[3];
    short dang = 0;

    SHRAP* p = SmallGore;
    short shrap_shade = -15;
    short shrap_xsize = 48, shrap_ysize = 48;
    short retval = true;
    short shrap_pal = PALETTE_DEFAULT;
    int shrap_floor_dist = Z(2);
    int shrap_ceiling_dist = Z(2);
    int nx,ny;
    short jump_grav = ACTOR_GRAVITY;
    short start_ang = 0;
    DSWActor* ShrapOwner = nullptr;
    int shrap_bounce = false;
    short WaitTics = 64; // for FastShrap
    short shrap_type;
    int shrap_rand_zamt = 0;
    short shrap_ang = parentActor->int_ang();
    short shrap_delta_size = 0;
    short shrap_amt = 0;

    if (Prediction)
        return 0;

    // Don't spawn shrapnel in invalid sectors gosh dern it!
    if (!parentActor->insector())
    {
        return 0;
    }

    if (breakinfo)
    {
        shrap_type = breakinfo->shrap_type;
        shrap_amt = breakinfo->shrap_amt;
        goto AutoShrap;
    }
    else if ((parentActor->spr.extra & SPRX_BREAKABLE))
    {
        // if no user
        if (!parentActor->hasU())
        {
            // Jump to shrap type
            shrap_type = SP_TAG8(parentActor);
            goto UserShrap;
        }
        else
        {
            // has a user - is programmed
            change_actor_stat(parentActor, STAT_MISC);
            parentActor->spr.extra &= ~(SPRX_BREAKABLE);
            parentActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        }
    }

    ASSERT(parentActor->hasU());

    switch (parentActor->user.ID)
    {
    case ST1:
        switch (parentActor->spr.hitag)
        {
        case SPAWN_SPOT:
        {
            if (parentActor->user.LastDamage)
                shrap_type = SP_TAG3(parentActor);
            else
                shrap_type = SP_TAG6(parentActor);

UserShrap:

            shrap_delta_size = (int8_t)SP_TAG10(parentActor);
            shrap_rand_zamt = SP_TAG9(parentActor);
            // Hey, better limit this in case mappers go crazy, like I did. :)
            // Kills frame rate!
            shrap_amt = SP_TAG8(parentActor);
            if (shrap_amt > 5)
                shrap_amt = 5;

AutoShrap:

            switch (shrap_type)
            {
            case SHRAP_NONE:
                return false;

            case SHRAP_GLASS:
                PlaySound(DIGI_BREAKGLASS, parentActor, v3df_dontpan | v3df_doppler);
                p = GlassShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, GlassShrap, sizeof(GlassShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_GENERIC:
            case SHRAP_STONE:
                PlaySound(DIGI_BREAKSTONES,parentActor,v3df_dontpan|v3df_doppler);
                p = StoneShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, StoneShrap, sizeof(StoneShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 8 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_WOOD:
                PlaySound(DIGI_BREAKINGWOOD,parentActor,v3df_dontpan|v3df_doppler);
                p = WoodShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodShrap, sizeof(WoodShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_BLOOD:
                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                break;

            case SHRAP_GIBS:
                PlaySound(DIGI_GIBS1,parentActor,v3df_dontpan|v3df_doppler);
                p = SmallGore;
                shrap_xsize = shrap_ysize = 34;
                shrap_bounce = false;
                break;

            case SHRAP_TREE_BARK:
                PlaySound(DIGI_BREAKINGWOOD,parentActor,v3df_dontpan|v3df_doppler);
                p = WoodShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodShrap, sizeof(WoodShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_PAPER:
                p = PaperShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, PaperShrap, sizeof(PaperShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                break;

            case SHRAP_METAL:
                PlaySound(DIGI_BREAKMETAL,parentActor,v3df_dontpan|v3df_doppler);
                p = MetalShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, MetalShrap, sizeof(MetalShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;


            case SHRAP_COIN:
                PlaySound(DIGI_COINS,parentActor,v3df_dontpan|v3df_doppler);
                p = CoinShrap;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, CoinShrap, sizeof(CoinShrap));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_METALMIX:
                PlaySound(DIGI_BREAKMETAL,parentActor,v3df_dontpan|v3df_doppler);
                p = MetalMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, MetalMix, sizeof(MetalMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_MARBELS:
            {
                p = Marbels;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, Marbels, sizeof(Marbels));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 10 + shrap_delta_size;
                shrap_bounce = true;
            }
            break;

            case SHRAP_WOODMIX:
                PlaySound(DIGI_BREAKINGWOOD,parentActor,v3df_dontpan|v3df_doppler);
                p = WoodMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, WoodMix, sizeof(WoodMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = true;
                break;

            case SHRAP_PAPERMIX:
                PlaySound(DIGI_BREAKINGWOOD,parentActor,v3df_dontpan|v3df_doppler);
                p = PaperMix;
                if (shrap_amt)
                {
                    memcpy(CustomShrap, PaperMix, sizeof(PaperMix));
                    CustomShrap->num = shrap_amt;
                    p = CustomShrap;
                }

                shrap_xsize = shrap_ysize = 16 + shrap_delta_size;
                shrap_bounce = false;
                break;

            case SHRAP_SO_SMOKE:
                return false;

            case SHRAP_EXPLOSION:
            {
                auto spnum = SpawnLargeExp(parentActor);
                short size = spnum->spr.xrepeat;
                spnum->spr.xrepeat = spnum->spr.yrepeat = size + shrap_delta_size;

                return false;
            }

            case SHRAP_LARGE_EXPLOSION:
            {
                auto spnum = SpawnLargeExp(parentActor);
                short size = spnum->spr.xrepeat;
                spnum->spr.xrepeat = spnum->spr.yrepeat = size + shrap_delta_size;

                InitPhosphorus(spnum);

                return false;
            }

            default:
            {
                return false;
            }
            }
            break;
        }

        default:
            p = LavaShrapTable[RANDOM_P2(2<<8)>>8];
        }
        break;

    case BREAK_BARREL:
        PlaySound(DIGI_BREAKDEBRIS,parentActor,v3df_dontpan|v3df_doppler);
        p = WoodShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = true;
        ChangeState(parentActor, s_BreakBarrel);
        break;
    case BREAK_LIGHT:
        PlaySound(DIGI_BREAKGLASS,parentActor,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = true;
        ChangeState(parentActor, s_BreakLight);
        break;
    case BREAK_PEDISTAL:
        PlaySound(DIGI_BREAKSTONES,parentActor,v3df_dontpan|v3df_doppler);
        p = StoneShrap;
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = true;
        ChangeState(parentActor, s_BreakPedistal);
        break;
    case BREAK_BOTTLE1:
        PlaySound(DIGI_BREAKGLASS,parentActor,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = true;
        ChangeState(parentActor, s_BreakBottle1);
        break;
    case BREAK_BOTTLE2:
        PlaySound(DIGI_BREAKGLASS,parentActor,v3df_dontpan|v3df_doppler);
        p = GlassShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = true;
        ChangeState(parentActor, s_BreakBottle2);
        break;
    case BREAK_MUSHROOM:
        PlaySound(DIGI_BREAKDEBRIS,parentActor,v3df_dontpan|v3df_doppler);
        p = StoneShrap;
        shrap_xsize = shrap_ysize = 4;
        shrap_bounce = true;
        SetSuicide(parentActor); // kill next iteration
        break;
    case BOLT_EXP:
        return false;
//        p = BoltExpShrap;
//        break;
    case TANK_SHELL_EXP:
        return false;
//        p = BoltExpShrap;
//        break;
    case TRACER_EXP:
        return false;
//        p = TracerExpShrap;
//        shrap_xsize = shrap_ysize = 20;
//        WaitTics = 10;
//        break;
    case BOLT_THINMAN_R1:
        p = MetalShrap;
        if (shrap_amt)
        {
            memcpy(CustomShrap, MetalShrap, sizeof(MetalShrap));
            CustomShrap->num = 1;
            p = CustomShrap;
        }

        shrap_xsize = shrap_ysize = 10;
        break;
    case LAVA_BOULDER:
        PlaySound(DIGI_BREAKSTONES,parentActor,v3df_dontpan|v3df_doppler);
        p = LavaBoulderShrap;
        ShrapOwner = GetOwner(parentActor);
        shrap_xsize = shrap_ysize = 24;
        shrap_bounce = true;
        break;
    case SECTOR_EXP:
        //p = SectorExpShrap;
        //break;
        return false;
    case GRENADE_EXP:
        //p = SectorExpShrap;
        //break;
        return false;
    case FIREBALL_EXP:
        return false;
//        p = FireballExpShrap[RANDOM_P2(2<<8)>>8];
//        shrap_pal = parentActor->user.spal;
        break;
    case ELECTRO_PLAYER:
    case ELECTRO_ENEMY:
        ShrapOwner = GetOwner(parentActor);
        p = ElectroShrap;
        shrap_xsize = shrap_ysize = 20;
        break;
    case COOLIE_RUN_R0:
        if (means == WPN_NM_SECTOR_SQUISH)
            break;
//        return (false);
        break;
    case NINJA_DEAD:
        return false;
        break;
    case NINJA_Head_R0:
    {
        extern STATE* sg_PlayerHeadHurl[];

        if (parentActor->user.Rot == sg_PlayerHeadHurl)
        {
            p = PlayerHeadHurl1;
        }
        else
        {
            p = PlayerDeadHead;
            shrap_xsize = shrap_ysize = 16+8;
            shrap_bounce = true;
        }
        break;
    }
    case GIRLNINJA_RUN_R0:
        p = StdShrap;
        break;
    case NINJA_RUN_R0:
    {
        p = StdShrap;
        if (parentActor->user.PlayerP)
        {
            PLAYER* pp = parentActor->user.PlayerP;

            if (pp->DeathType == PLAYER_DEATH_CRUMBLE)
                p = PlayerGoreFall;
            else
                p = PlayerGoreFly;
        }
        break;
    }
    case GORO_RUN_R0:
        p = StdShrap;
        shrap_xsize = shrap_ysize = 64;
        break;
    case COOLG_RUN_R0:
        p = UpperGore;
        break;
    case RIPPER_RUN_R0:
        p = StdShrap;
        if (parentActor->user.spal != 0)
            shrap_xsize = shrap_ysize = 64;
        else
            shrap_xsize = shrap_ysize = 32;
        break;
    case RIPPER2_RUN_R0:
        p = StdShrap;
        if (parentActor->user.spal != 0)
            shrap_xsize = shrap_ysize = 64;
        else
            shrap_xsize = shrap_ysize = 32;
        break;
    case SERP_RUN_R0:
        p = StdShrap;
        //return (false);
        break;
    case SUMO_RUN_R0:
        p = StdShrap;
        break;
    case SKEL_RUN_R0:
        p = SkelGore;
        shrap_pal = PALETTE_SKEL_GORE;
        break;
    case HORNET_RUN_R0:
        p = SmallGore;
        shrap_pal = PALETTE_SKEL_GORE;
        break;
    case SKULL_R0:
    case SKULL_R0 + 1:
        p = FlamingGore;
        break;
    case SKULL_SERP:
        return false;
    case BETTY_R0:
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
        PlaySound(DIGI_BREAKGLASS,parentActor,v3df_dontpan|v3df_doppler);
        p = MetalShrap;
        shrap_xsize = shrap_ysize = 10;
        break;
    case ZILLA_RUN_R0:
        p = MetalShrap;
        shrap_xsize = shrap_ysize = 10;
        break;
    case EMP:
        p = EMPShrap;
        shrap_xsize = shrap_ysize = 8;
        shrap_bounce = false;
        break;
    }

    // second sprite involved
    // most of the time is is the weapon
    if (secondaryActor != nullptr)
    {
        if (secondaryActor->user.PlayerP && secondaryActor->user.PlayerP->sop_control)
        {
            p = StdShrap;
        }
        else
            switch (secondaryActor->user.ID)
            {
            case PLASMA_FOUNTAIN:
                p = HeartAttackShrap;
                break;
            }
    }

    hz[Z_TOP] = ActorZOfTop(parentActor);        // top
    hz[Z_BOT] = ActorZOfBottom(parentActor);        // bottom
    hz[Z_MID] = (hz[0] + hz[2]) >> 1;        // mid

    for (; p->id; p++)
    {
        if (!p->random_disperse)
        {
            //dang = (2048 / p->num);
            start_ang = NORM_ANGLE(shrap_ang - (p->ang_range >> 1));
            dang = (p->ang_range / p->num);
        }

        for (i = 0; i < p->num; i++)
        {
            auto actor = SpawnActor(STAT_SKIP4, p->id, p->state, parentActor->sector(),
                                    parentActor->int_pos().X, parentActor->int_pos().Y, hz[p->zlevel], shrap_ang, 512);

            if (p->random_disperse)
            {
                actor->set_int_ang(NORM_ANGLE(shrap_ang + (RANDOM_P2(p->ang_range<<5)>>5) - (p->ang_range >> 1)));
            }
            else
            {
                actor->set_int_ang(NORM_ANGLE(start_ang + (i * dang)));
            }

            // for FastShrap
            actor->user.change.Z = labs(actor->user.jump_speed*4) - RandomRange(labs(actor->user.jump_speed)*8)*2;
            actor->user.WaitTics = WaitTics + RandomRange(WaitTics/2);

            switch (actor->user.ID)
            {
            case GORE_Drip:
                shrap_bounce = false;
                break;
            case GORE_Lung:
                shrap_xsize = 20;
                shrap_ysize = 20;
                shrap_bounce = false;
                break;
            case GORE_Liver:
                shrap_xsize = 20;
                shrap_ysize = 20;
                shrap_bounce = false;
                break;
            case GORE_SkullCap:
                shrap_xsize = 24;
                shrap_ysize = 24;
                shrap_bounce = true;
                break;
            case GORE_Arm:
                shrap_xsize = 21;
                shrap_ysize = 21;
                shrap_bounce = false;
                break;
            case GORE_Head:
                shrap_xsize = 26;
                shrap_ysize = 30;
                shrap_bounce = true;
                break;
            case Vomit1:
                shrap_bounce = false;
				actor->spr.pos.Z -= 4;
                shrap_xsize = actor->user.pos.X = 12 + (RANDOM_P2(32<<8)>>8);
                shrap_ysize = actor->user.pos.Y = 12 + (RANDOM_P2(32<<8)>>8);
                actor->user.Counter = (RANDOM_P2(2048<<5)>>5);

                nx = bcos(actor->int_ang(), -6);
                ny = bsin(actor->int_ang(), -6);
                move_missile(actor, nx, ny, 0, Z(8), Z(8), CLIPMASK_MISSILE, MISSILEMOVETICS);

                if (RANDOM_P2(1024)<700)
                    actor->user.ID = 0;

                break;
            case EMP:
                shrap_bounce = false;
				actor->spr.pos.Z -= 4;
                shrap_xsize = actor->user.pos.X = 5 + (RANDOM_P2(4<<8)>>8);
                shrap_ysize = actor->user.pos.Y = 5 + (RANDOM_P2(4<<8)>>8);
                break;
            }

            actor->spr.shade = int8_t(shrap_shade);
            actor->spr.xrepeat = uint8_t(shrap_xsize);
            actor->spr.yrepeat = uint8_t(shrap_ysize);
            actor->spr.clipdist = 16 >> 2;

            if (ShrapOwner != nullptr)
            {
                SetOwner(ShrapOwner, actor);
            }

            if (shrap_rand_zamt)
            {
                actor->add_int_z(Z(RandomRange(shrap_rand_zamt) - (shrap_rand_zamt/2)));
            }

            actor->spr.pal = actor->user.spal = uint8_t(shrap_pal);

            actor->spr.xvel = p->min_vel*2;
            actor->spr.xvel += RandomRange(p->max_vel - p->min_vel);

            actor->user.floor_dist = shrap_floor_dist * zinttoworld;
            actor->user.ceiling_dist = shrap_ceiling_dist * zinttoworld;
            actor->user.jump_speed = p->min_jspeed;
            actor->user.jump_speed += RandomRange(p->max_jspeed - p->min_jspeed);
            actor->user.jump_speed = -actor->user.jump_speed;

            DoBeginJump(actor);
            actor->user.jump_grav = jump_grav;

            actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
            actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());

            if (!shrap_bounce)
                actor->user.Flags |= (SPR_BOUNCE);
        }
    }

    return retval;
}


void DoShrapMove(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, 0, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS*2);
}

int DoVomit(DSWActor* actor)
{
    actor->user.Counter = NORM_ANGLE(actor->user.Counter + (30*MISSILEMOVETICS));
    actor->spr.xrepeat = actor->user.pos.X + MulScale(12, bcos(actor->user.Counter), 14);
    actor->spr.yrepeat = actor->user.pos.Y + MulScale(12, bsin(actor->user.Counter), 14);
    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoJump(actor);
        DoJump(actor);
        DoShrapMove(actor);
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);
        DoFall(actor);
        DoShrapMove(actor);
    }
    else
    {
        ChangeState(actor, s_VomitSplash);
        DoFindGroundPoint(actor);
        MissileWaterAdjust(actor);
        actor->spr.pos.Z = actor->user.loz;
        actor->user.WaitTics = 60;
        actor->user.pos.X = actor->spr.xrepeat;
        actor->user.pos.Y = actor->spr.yrepeat;
        return 0;
    }

    if (actor->user.coll.type == kHitSprite)
    {
        if ((actor->user.coll.actor()->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            DoDamage(actor->user.coll.actor(), actor);
        }
    }
    return 0;
}


int DoVomitSplash(DSWActor* actor)
{
    if ((actor->user.WaitTics-=MISSILEMOVETICS) < 0)
    {
        KillActor(actor);
        return 0;
    }
    return 0;
}

int DoFastShrapJumpFall(DSWActor* actor)
{
    actor->add_int_pos({ actor->user.change.X * 2, actor->user.change.Y * 2, actor->user.change.Z * 2 });
    actor->user.WaitTics -= MISSILEMOVETICS;
    if (actor->user.WaitTics <= 0)
        KillActor(actor);

    return 0;
}

int DoTracerShrap(DSWActor* actor)
{
    actor->add_int_pos({ actor->user.change.X, actor->user.change.Y, actor->user.change.Z });

    actor->user.WaitTics -= MISSILEMOVETICS;
    if (actor->user.WaitTics <= 0)
        KillActor(actor);

    return 0;
}

int DoShrapJumpFall(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoShrapVelocity(actor);
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoShrapVelocity(actor);
    }
    else
    {
        if (!(actor->user.Flags & SPR_BOUNCE))
        {
            DoShrapVelocity(actor);
            return 0;
        }

        if (actor->user.ID == GORE_Drip)
            ChangeState(actor, s_GoreFloorSplash);
        else
            ShrapKillSprite(actor);
    }
    return 0;
}

int DoShrapDamage(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoJump(actor);
        DoShrapMove(actor);
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);
        DoShrapMove(actor);
    }
    else
    {
        if (!(actor->user.Flags & SPR_BOUNCE))
        {
            actor->user.Flags |= (SPR_BOUNCE);
            actor->user.jump_speed = -300;
            actor->spr.xvel >>= 2;
            DoBeginJump(actor);
            return 0;
        }

        KillActor(actor);
        return 0;
    }

    if (actor->user.coll.type == kHitSprite)
    {
        WeaponMoveHit(actor);
        KillActor(actor);
    }

    return 0;
}

int SpawnBlood(DSWActor* actor, DSWActor* weapActor, short hit_ang, int hit_x, int hit_y, int hit_z)
{
    int i;

    // state, id, num, zlevel, min_jspeed, max_jspeed, min_vel, max_vel,
    // random_disperse, ang_range;

    static SHRAP UziBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_MID, 100, 250, 10, 20, true, 512},  // 70,200 vels
        //{s_GoreSplash, PLASMA_Drip, 1, Z_BOT, 0, 0, 0, 0, false, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SmallBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_TOP, 100, 250, 10, 20, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP PlasmaFountainBlood[] =
    {
        {s_PlasmaDrip, PLASMA_Drip, 1, Z_TOP, 200, 500, 100, 300, true, 16},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP SomeBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 1, Z_TOP, 100, 250, 10, 20, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP MoreBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 2, Z_TOP, 100, 250, 10, 20, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };
#endif

    static SHRAP ExtraBlood[] =
    {
        {s_GoreDrip, GORE_Drip, 4, Z_TOP, 100, 250, 10, 20, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

    static SHRAP HariKariBlood[] =
    {
        {s_FastGoreDrip, GORE_Drip, 32, Z_TOP, 200, 650, 70, 100, true, 1024},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };

#if 0
    static SHRAP SwordPowerup[] =
    {
        {s_ElectroShrap, ELECTRO_SHARD, 16, Z_TOP, 75, 200, 70, 150, true, 512},
        {nullptr,0,0,0,0,0,0,0,0,0},
    };
#endif

    short dang = 0;

    SHRAP* p = UziBlood;
    short shrap_shade = -15;
    short shrap_xsize = 20, shrap_ysize = 20;
    short retval = true;
    short shrap_pal = PALETTE_DEFAULT;
    short start_ang = 0;

    switch (actor->user.ID)
    {
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    case ZILLA_RUN_R0:
        return 0;   // Don't put blood on trashcan
    }

    // second sprite involved
    // most of the time is the weapon
    if (weapActor != nullptr)
    {
        switch (weapActor->user.ID)
        {
        case NINJA_RUN_R0: //sword
            // if sprite and weapon are the same it must be HARIII-KARIII
            if (actor == weapActor)
            {
                p = HariKariBlood;
                hit_ang = actor->int_ang();
                hit_x = actor->int_pos().X;
                hit_y = actor->int_pos().Y;
                hit_z = ActorZOfTop(weapActor) + (ActorSizeZ(weapActor) >> 4);
            }
            else
            {
                p = ExtraBlood;
                hit_ang = NORM_ANGLE(weapActor->int_ang() + 1024);
                hit_x = actor->int_pos().X;
                hit_y = actor->int_pos().Y;
                hit_z = ActorZOfTop(weapActor) + (ActorSizeZ(weapActor) >> 2);
            }
            break;
        case SERP_RUN_R0:
            p = ExtraBlood;
            hit_ang = NORM_ANGLE(weapActor->int_ang() + 1024);
            hit_x = actor->int_pos().X;
            hit_y = actor->int_pos().Y;
            hit_z = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);
            break;
        case BLADE1:
        case BLADE2:
        case BLADE3:
        case 5011:
            p = SmallBlood;
            hit_ang = NORM_ANGLE(AngToSprite(actor, weapActor) + 1024);
            hit_x = actor->int_pos().X;
            hit_y = actor->int_pos().Y;
            hit_z = weapActor->int_pos().Z - (ActorSizeZ(weapActor) >> 1);
            break;
        case STAR1:
        case CROSSBOLT:
            p = SomeBlood;
            hit_ang = NORM_ANGLE(weapActor->int_ang() + 1024);
            hit_x = actor->int_pos().X;
            hit_y = actor->int_pos().Y;
            hit_z = weapActor->int_pos().Z;
            break;
        case PLASMA_FOUNTAIN:
            p = PlasmaFountainBlood;
            hit_ang = weapActor->int_ang();
            hit_x = actor->int_pos().X;
            hit_y = actor->int_pos().Y;
            hit_z = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);
            break;
        default:
            p = SomeBlood;
            hit_ang = NORM_ANGLE(weapActor->int_ang() + 1024);
            hit_x = actor->int_pos().X;
            hit_y = actor->int_pos().Y;
            hit_z = ActorZOfTop(weapActor) + (ActorSizeZ(weapActor) >> 2);
            break;
        }
    }
    else
    {
        hit_ang = NORM_ANGLE(hit_ang + 1024);
    }

    for (; p->state; p++)
    {
        if (!p->random_disperse)
        {
            start_ang = NORM_ANGLE(hit_ang - (p->ang_range >> 1) + 1024);
            dang = (p->ang_range / p->num);
        }

        for (i = 0; i < p->num; i++)
        {
            auto actorNew = SpawnActor(STAT_SKIP4, p->id, p->state, actor->sector(),
                              hit_x, hit_y, hit_z, hit_ang, 0);

            switch (actorNew->user.ID)
            {
            case ELECTRO_SHARD:
                shrap_xsize = 7;
                shrap_ysize = 7;
                break;

            case PLASMA_Drip:
                // Don't do central blood splats for every hitscan
                if (RANDOM_P2(1024) < 950)
                {
                    actorNew->spr.xrepeat = actorNew->spr.yrepeat = 0;
                }
                if (RANDOM_P2(1024) < 512)
                    actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
                //shrap_xsize = 96;
                //shrap_ysize = 75;
                //shrap_xsize = 10;
                //shrap_ysize = 10;
                break;
            }

            if (p->random_disperse)
            {
                actorNew->set_int_ang(NORM_ANGLE(hit_ang + (RANDOM_P2(p->ang_range<<5)>>5) - (p->ang_range >> 1)));
            }
            else
            {
                actorNew->set_int_ang(NORM_ANGLE(start_ang + (i * dang)));
            }

            actorNew->user.Flags |= (SPR_BOUNCE);

            actorNew->spr.shade = int8_t(shrap_shade);
            actorNew->spr.xrepeat = uint8_t(shrap_xsize);
            actorNew->spr.yrepeat = uint8_t(shrap_ysize);
            actorNew->spr.clipdist = 16 >> 2;

            actorNew->spr.pal = actorNew->user.spal = uint8_t(shrap_pal);

            actorNew->spr.xvel = p->min_vel;
            actorNew->spr.xvel += RandomRange(p->max_vel - p->min_vel);

            // special case
            // blood coming off of actors should have the acceleration of the actor
            // so add it in
            actorNew->spr.xvel += actor->spr.xvel;

            actorNew->user.ceiling_dist = actorNew->user.floor_dist = 2;
            actorNew->user.jump_speed = p->min_jspeed;
            actorNew->user.jump_speed += RandomRange(p->max_jspeed - p->min_jspeed);
            actorNew->user.jump_speed = -actorNew->user.jump_speed;

            actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
            actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());

            // for FastShrap
            actorNew->user.change.Z = labs(actorNew->user.jump_speed*4) - RandomRange(labs(actorNew->user.jump_speed)*8);
            actorNew->user.WaitTics = 64 + RANDOM_P2(32);

            actor->user.Flags |= (SPR_BOUNCE);

            DoBeginJump(actorNew);
        }
    }
    return retval;
}


bool VehicleMoveHit(DSWActor* actor)
{
    SECTOR_OBJECT* sop;
    SECTOR_OBJECT* hsop;
    bool TestKillSectorObject(SECTOR_OBJECT*);

    if (actor->user.coll.type == kHitNone)
        return false;

    sop = actor->user.sop_parent;

    // sprite controlling sop
    DSWActor*  ctrlr = sop->controller;
    if (!ctrlr) return false;

    switch (actor->user.coll.type)
    {
    case kHitSector:
    {
        sectortype* sectp = actor->user.coll.hitSector;

        if ((sectp->extra & SECTFX_SECTOR_OBJECT))
        {
            // shouldn't ever really happen
        }

        return true;
    }

    case kHitSprite:
    {
        auto hitActor = actor->user.coll.actor();

        if ((hitActor->spr.extra & SPRX_BREAKABLE))
        {
            HitBreakSprite(hitActor, actor->user.ID);
            return true;
        }

        if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            if (hitActor != GetOwner(ctrlr))
            {
                DoDamage(hitActor, ctrlr);
                return true;
            }
        }
        else
        {
            if (hitActor->spr.statnum == STAT_MINE_STUCK)
            {
                DoDamage(hitActor, actor);
                return true;
            }
        }

        return true;
    }

    case kHitWall:
    {
        auto wph = actor->user.coll.hitWall;

        if ((wph->extra & WALLFX_SECTOR_OBJECT))
        {
            // sector object collision
            if ((hsop = DetectSectorObjectByWall(wph)))
            {
                SopDamage(hsop, sop->ram_damage);
                if (hsop->max_damage <= 0)
                    SopCheckKill(hsop);
                else
                    DoSpawnSpotsForDamage(hsop->match_event);
            }
        }

        return true;
    }
    }

    return false;
}


bool WeaponMoveHit(DSWActor* actor)
{
    switch (actor->user.coll.type)
    {
    default:
        break;

    case kHitVoid:
        SetSuicide(actor);
        return true;

    case kHitSector:
    {
        sectortype* sectp;
        SECTOR_OBJECT* sop;

        sectp = actor->user.coll.hitSector;

        ASSERT(sectp->extra != -1);

        // hit floor - closer to floor than ceiling
        if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
        {
            // hit a floor sprite
            if (actor->user.lowActor)
            {

                if (actor->user.lowActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(actor, -1, actor->user.lowActor))
                        return true;
                }

                return true;
            }

            if (sectp->hasU() && FixedToInt(sectp->depth_fixed) > 0)
            {
                SpawnSplash(actor);
                return true;
            }

        }
        // hit ceiling
        else
        {
            // hit a floor sprite
            if (actor->user.highActor)
            {
                if (actor->user.highActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(actor, -1, actor->user.highActor))
                        return true;
                }
            }
        }


        if ((sectp->extra & SECTFX_SECTOR_OBJECT))
        {
            if ((sop = DetectSectorObject(sectp)))
            {
                DoDamage(sop->sp_child, actor);
                return true;
            }
        }

        if ((sectp->ceilingstat & CSTAT_SECTOR_SKY) && sectp->ceilingpicnum != FAF_MIRROR_PIC)
        {
            if (labs(actor->int_pos().Z - sectp->int_ceilingz()) < ActorSizeZ(actor))
            {
                SetSuicide(actor);
                return true;
            }
        }

        return true;
    }

    case kHitSprite:
    {
        auto hitActor = actor->user.coll.actor();

        ASSERT(hitActor->spr.extra != -1);

        if ((hitActor->spr.extra & SPRX_BREAKABLE))
        {
            HitBreakSprite(hitActor, actor->user.ID);
            return true;
        }

        if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            // make sure you didn't hit the Owner of the missile
            if (hitActor != GetOwner(actor))
            {
                if (actor->user.ID == STAR1)
                {
                    extern STATE s_TrashCanPain[];
                    switch (hitActor->user.ID)
                    {
                    case TRASHCAN:
                        PlaySound(DIGI_TRASHLID, actor, v3df_none);
                        PlaySound(DIGI_STARCLINK, actor, v3df_none);
                        if (hitActor->user.WaitTics <= 0)
                        {
                            hitActor->user.WaitTics = SEC(2);
                            ChangeState(hitActor,s_TrashCanPain);
                        }
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case ZILLA_RUN_R0:
                    case 623:
                    {
                        PlaySound(DIGI_STARCLINK, actor, v3df_none);
                    }
                    break;
                    }
                }
                DoDamage(hitActor, actor);
                return true;
            }
        }
        else
        {
            if (hitActor->spr.statnum == STAT_MINE_STUCK)
            {
                DoDamage(hitActor, actor);
                return true;
            }
        }

        if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(actor, -1, hitActor))
                return true;
        }

        if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
        {
            if (hitActor->spr.lotag || hitActor->spr.hitag)
            {
                ShootableSwitch(hitActor);
                return true;
            }
        }

        return true;
    }

    case kHitWall:
    {
        auto wph = actor->user.coll.hitWall;
        SECTOR_OBJECT* sop;

        ASSERT(wph->extra != -1);

        if ((wph->extra & WALLFX_SECTOR_OBJECT))
        {
            if ((sop = DetectSectorObjectByWall(wph)))
            {
                if (sop->max_damage != -999)
                    DoDamage(sop->sp_child, actor);
                return true;
            }
        }

        if (wph->lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(wph, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.ID);
            actor->user.coll.setNone();
            return true;
        }

        // clipmove does not correctly return the sprite for WALL sprites
        // on walls, so look with hitscan

        HitInfo hit{};
        hitscan(actor->int_pos(), actor->sector(), { bcos(actor->int_ang()), bsin(actor->int_ang()), actor->spr.zvel }, hit, CLIPMASK_MISSILE);

        if (!hit.hitSector)
        {
            return false;
        }

        if (hit.actor())
        {
            auto hitActor = hit.actor();

            if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (MissileHitMatch(actor, -1, hitActor))
                    return true;
            }

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                if (hitActor->spr.lotag || hitActor->spr.hitag)
                {
                    ShootableSwitch(hitActor);
                    return true;
                }
            }
        }

        return true;
    }
    }

    return false;
}

int DoUziSmoke(DSWActor* actor)
{
	actor->spr.pos.Z -= 0.78125; // !JIM! Make them float up
    return 0;
}

int DoShotgunSmoke(DSWActor* actor)
{
	actor->spr.pos.Z -= 0.78125; // !JIM! Make them float up
    return 0;
}

int DoMineSpark(DSWActor* actor)
{
    if (actor->spr.picnum != 0)
    {
        DoDamageTest(actor);
    }
    return 0;
}

int DoFireballFlames(DSWActor* actor)
{
    bool jumping = false;

    // if no Owner then stay where you are
    DSWActor* attach = actor->user.attachActor;
    if (attach != nullptr)
    {
        actor->spr.pos = DVector3(attach->spr.pos.XY(), ActorZOfMiddle(attach));

        if ((attach->spr.extra & SPRX_BURNABLE))
        {
            if ((actor->user.Counter2 & 1) == 0)
            {
                attach->spr.shade++;
                if (attach->spr.shade > 10)
                    attach->spr.shade = 10;
            }
        }
    }
    else
    {
        if (actor->user.Flags & (SPR_JUMPING))
        {
            DoJump(actor);
            jumping = true;
        }
        else if (actor->user.Flags & (SPR_FALLING))
        {
            DoFall(actor);
            jumping = true;
        }
        else
        {
            if (actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed) > 0)
            {
                if (abs(actor->sector()->floorz - actor->spr.pos.Z) <= 4)
                {
                    KillActor(actor);
                    return 0;
                }
            }

            if (TestDontStickSector(actor->sector()))
            {
                KillActor(actor);
                return 0;
            }
        }
    }

    if (!jumping)
    {
        if ((actor->user.WaitTics += MISSILEMOVETICS) > 4 * 120)
        {
            // shrink and go away
            actor->spr.xrepeat--;
            actor->spr.yrepeat--;

            if (((int8_t)actor->spr.xrepeat) == 0)
            {
                if (actor->user.attachActor != nullptr)
                {
                    actor->user.attachActor->user.flameActor = nullptr;
                    actor->user.attachActor->user.Flags2 &= ~SPR2_FLAMEDIE;
                }
                KillActor(actor);
                return 0;
            }
        }
        else
        {
            // grow until the right size
            if (actor->spr.xrepeat <= actor->user.Counter)
            {
                actor->spr.xrepeat += 3;
                actor->spr.yrepeat += 3;
            }
        }
    }

    actor->user.Counter2++;
    if (actor->user.Counter2 > 9)
    {
        actor->user.Counter2 = 0;
        DoFlamesDamageTest(actor);
    }

    return 0;
}

int DoBreakFlames(DSWActor* actor)
{
    bool jumping = false;

    if (actor->user.Flags & (SPR_JUMPING))
    {
        DoJump(actor);
        jumping = true;
    }
    else if (actor->user.Flags & (SPR_FALLING))
    {
        DoFall(actor);
        jumping = true;
    }
    else
    {
        if (actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed) > 0)
        {
            if (abs(actor->sector()->floorz - actor->spr.pos.Z) <= 4)
            {
                KillActor(actor);
                return 0;
            }
        }

        if (TestDontStickSector(actor->sector()))
        {
            KillActor(actor);
            return 0;
        }
    }

    if (!jumping)
    {
        if ((actor->user.WaitTics += MISSILEMOVETICS) > 4 * 120)
        {
            // shrink and go away
            actor->spr.xrepeat--;
            actor->spr.yrepeat--;

            if (((int8_t)actor->spr.xrepeat) == 0)
            {
                if (actor->user.attachActor != nullptr)
                {
                    actor->user.attachActor->user.flameActor = nullptr;
                    actor->user.attachActor->user.Flags2 &= ~SPR2_FLAMEDIE;
                }
                KillActor(actor);
                return 0;
            }
        }
        else
        {
            // grow until the right size
            if (actor->spr.xrepeat <= actor->user.Counter)
            {
                actor->spr.xrepeat += 3;
                actor->spr.yrepeat += 3;
            }

            if (actor->user.WaitTics + MISSILEMOVETICS > 4 * 120)
            {
                SpawnBreakStaticFlames(actor);
            }
        }
    }

    actor->user.Counter2++;
    if (actor->user.Counter2 > 9)
    {
        actor->user.Counter2 = 0;
        DoFlamesDamageTest(actor);
    }

    return 0;
}

int SetSuicide(DSWActor* actor)
{
    if (actor->hasU())
    {
        actor->user.Flags |= (SPR_SUICIDE);
        actor->user.RotNum = 0;
    }
    ChangeState(actor, s_Suicide);
    return 0;
}

int DoActorScale(DSWActor* actor)
{
    actor->user.scale_speed = 70;
    actor->user.scale_value = actor->spr.xrepeat << 8;
    actor->user.scale_tgt = actor->spr.xrepeat + 25;

    if (actor->user.scale_tgt > 256)
    {
        actor->user.scale_speed = 0;
        actor->user.scale_tgt = 256;
    }

    return 0;
}

int DoRipperGrow(DSWActor* actor)
{
    actor->user.scale_speed = 70;
    actor->user.scale_value = actor->spr.xrepeat << 8;
    actor->user.scale_tgt = actor->spr.xrepeat + 20;

    if (actor->user.scale_tgt > 128)
    {
        actor->user.scale_speed = 0;
        actor->user.scale_tgt = 128;
    }

    return 0;
}

void UpdateSinglePlayKills(DSWActor* actor)
{
    // single play and coop kill count
    if (gNet.MultiGameType != MULTI_GAME_COMMBAT && actor->hasU())
    {
        if (actor->user.Flags & (SPR_SUICIDE))
        {
            return;
        }

        switch (actor->user.ID)
        {
        case COOLIE_RUN_R0:
        case NINJA_RUN_R0:
        case NINJA_CRAWL_R0:
        case GORO_RUN_R0:
        case 1441:
        case COOLG_RUN_R0:
        case EEL_RUN_R0:
        case SUMO_RUN_R0:
        case ZILLA_RUN_R0:
        //case TOILETGIRL_R0:
        //case WASHGIRL_R0:
        //case BUNNY_RUN_R0:
        case RIPPER_RUN_R0:
        case RIPPER2_RUN_R0:
        case SERP_RUN_R0:
        case LAVA_RUN_R0:
        case SKEL_RUN_R0:
        case HORNET_RUN_R0:
        case GIRLNINJA_RUN_R0:
        case SKULL_R0:
        case BETTY_R0:
            // always give kills to the first player
            Player->Kills++;
            break;
        }
    }
}


int ActorChooseDeath(DSWActor* actor, DSWActor* weapActor)
{
    if (!actor->hasU()) return false;

    if (actor->user.Health > 0)
        return false;

    UpdateSinglePlayKills(actor);

    if (actor->user.Attrib)
        PlaySpriteSound(actor,attr_die,v3df_follow);

    switch (actor->user.ID)
    {
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    case TRASHCAN:
    case GORO_RUN_R0:
    case RIPPER2_RUN_R0:
        break;
    case BUNNY_RUN_R0:
    {
        Bunny_Count--;  // Bunny died, decrease the population
    }
    break;
    default:
        ActorCoughItem(actor);
        break;
    }

    switch (actor->user.ID)
    {
    case BETTY_R0:
    {
        DoBettyBeginDeath(actor);
        break;
    }
    case SKULL_R0:
    {
        DoSkullBeginDeath(actor);
        break;
    }
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
    case TRASHCAN:
    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:
    case 623:
    {
        if ((actor->user.ID == TOILETGIRL_R0 ||
             actor->user.ID == CARGIRL_R0 || actor->user.ID == MECHANICGIRL_R0 || actor->user.ID == SAILORGIRL_R0 || actor->user.ID == PRUNEGIRL_R0 ||
             actor->user.ID == WASHGIRL_R0) && weapActor->hasU() && weapActor->user.ID == NINJA_RUN_R0 && weapActor->user.PlayerP)
        {
            PLAYER* pp = weapActor->user.PlayerP;
            if (pp && !(pp->Flags & PF_DIVING))  // JBF: added null test
                pp->Bloody = true;
            PlaySound(DIGI_TOILETGIRLSCREAM, actor, v3df_none);
        }
        if (SpawnShrap(actor, weapActor))
            SetSuicide(actor);
        break;
    }

    // These are player zombies
    case ZOMBIE_RUN_R0:
        InitBloodSpray(actor,true,105);
        InitBloodSpray(actor,true,105);
        if (SpawnShrap(actor, weapActor))
            SetSuicide(actor);
        break;

    default:

        switch (weapActor->user.ID)
        {
        case NINJA_RUN_R0: //sword
            if (weapActor->user.PlayerP)
            {
                PLAYER* pp = weapActor->user.PlayerP;

                if (weapActor->user.WeaponNum == WPN_FIST && StdRandomRange(1000)>500 && pp == Player+myconnectindex)
                {
                    int choosesnd = StdRandomRange(6);

                    if (choosesnd == 0)
                        PlayerSound(DIGI_KUNGFU, v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 1)
                        PlayerSound(DIGI_PAYINGATTENTION, v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 2)
                        PlayerSound(DIGI_EATTHIS, v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 3)
                        PlayerSound(DIGI_TAUNTAI4, v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 4)
                        PlayerSound(DIGI_TAUNTAI5, v3df_follow|v3df_dontpan,pp);
                    else if (choosesnd == 5)
                        PlayerSound(DIGI_HOWYOULIKEMOVE, v3df_follow|v3df_dontpan,pp);
                    //PlayerSound(TauntAIVocs[choosesnd],&pp->posx,
                    //    &pp->posy,&pp->posy,v3df_dontpan|v3df_follow,pp);
                }
                else if (weapActor->user.WeaponNum == WPN_SWORD && StdRandomRange(1000)>500 && pp == Player+myconnectindex)
                {
                    short choose_snd;

                    choose_snd = StdRandomRange(1000);
                    if (choose_snd > 750)
                        PlayerSound(DIGI_SWORDGOTU1, v3df_follow|v3df_dontpan,pp);
                    else if (choose_snd > 575)
                        PlayerSound(DIGI_SWORDGOTU2, v3df_follow|v3df_dontpan,pp);
                    else if (choose_snd > 250)
                        PlayerSound(DIGI_SWORDGOTU3, v3df_follow|v3df_dontpan,pp);
                    else
                        PlayerSound(DIGI_CANBEONLYONE, v3df_follow|v3df_dontpan,pp);
                }
                if (!(pp->Flags & PF_DIVING))
                    pp->Bloody = true;
            }

            if (actor->user.WeaponNum == WPN_FIST)
                DoActorDie(actor, weapActor, 0);
            else if (actor->user.ID == NINJA_RUN_R0 || RandomRange(1000) < 500)
                DoActorDie(actor, weapActor, 0);
            else
            {
                // Can't gib bosses!
                if (actor->user.ID == SERP_RUN_R0 || actor->user.ID == SUMO_RUN_R0 || actor->user.ID == ZILLA_RUN_R0)
                {
                    DoActorDie(actor, weapActor, 0);
                    break;
                }

                // Gib out the ones you can't cut in half
                // Blood fountains
                InitBloodSpray(actor,true,-1);

                if (SpawnShrap(actor, weapActor))
                {
                    SetSuicide(actor);
                }
                else
                    DoActorDie(actor, weapActor, 0);

            }
            break;
        case BOLT_THINMAN_R0:
        case BOLT_THINMAN_R1:
        case BOLT_THINMAN_R2:
        case NAP_EXP:
        case BOLT_EXP:
        case TANK_SHELL_EXP:
        case SECTOR_EXP:
        case FIREBALL_EXP:
        case GRENADE_EXP:
        case MINE_EXP:
        case SKULL_R0:
        case BETTY_R0:
#if 0
            if (RANDOM_P2(1024) < 256 && weapActor->user.Radius != NUKE_RADIUS)
            {
                if (weapActor->user.ID == BOLT_THINMAN_R1 && weapActor->user.Radius == RAIL_RADIUS)
                {
                    SpawnShrapX(weapActor);    // Do rail gun shrap
                }
                DoActorDie(actor, weapActor, 0);

            }
            else
#endif
            {
                int choosesnd = 0;

                // For the Nuke, do residual radiation if he gibs
                if (weapActor->user.Radius == NUKE_RADIUS)
                    SpawnFireballFlames(actor, nullptr);

                // Random chance of taunting the AI's here
                if (RandomRange(1000) > 400)
                {
                    PLAYER* pp;

                    auto own = GetOwner(weapActor);
                    if (own && own->hasU())
                    {
                        pp = own->user.PlayerP;
                        if (pp)
                        {
                            choosesnd=StdRandomRange(MAX_TAUNTAI<<8)>>8;

                            if (pp == Player+myconnectindex)
                                PlayerSound(TauntAIVocs[choosesnd],v3df_dontpan|v3df_follow,pp);
                        }
                    }
                }

                // These guys cough items only if gibbed
                if (actor->user.ID == GORO_RUN_R0 || actor->user.ID == RIPPER2_RUN_R0)
                    ActorCoughItem(actor);

                // Blood fountains
                InitBloodSpray(actor,true,-1);

                // Bosses do not gib
                if (actor->user.ID == SERP_RUN_R0 || actor->user.ID == SUMO_RUN_R0 || actor->user.ID == ZILLA_RUN_R0)
                {
                    DoActorDie(actor, weapActor, 0);
                    return true;
                }

                if (SpawnShrap(actor, weapActor))
                {
                    SetSuicide(actor);
                }
                else
                    DoActorDie(actor, weapActor, 0);

            }

            break;
        default:
            DoActorDie(actor, weapActor, 0);
            break;
        }

        break;
    }
    return true;

}

int ActorHealth(DSWActor* actor, short amt)
{
    extern int FinishAnim;

    if (actor->user.ID == TRASHCAN && amt > -75)
    {
        actor->user.LastDamage = 100;
        return true;
    }

    actor->user.Flags |= (SPR_ATTACKED);

    actor->user.Health += amt;

    if (actor->user.ID == SERP_RUN_R0 && actor->spr.pal != 16 && (currentLevel->gameflags & LEVEL_SW_DEATHEXIT_SERPENT))
    {
        if (actor->user.Health < actor->user.MaxHealth/2)
        {
            FinishAnim = ANIM_SERP;
            // Hack for Last Warrior which apparently needs to continue with the next level here.
            if (!(currentLevel->gameflags & LEVEL_SW_DEATHEXIT_SERPENT_NEXT))
            {
                ChangeLevel(nullptr, g_nextskill, true);
                return true;
            }
            else FinishTimer = 1;
        }
    }

    if (actor->user.ID == SUMO_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_DEATHEXIT_SUMO))
    {
        if (actor->user.Health <= 0)
        {
            FinishTimer = 7*120;
            FinishAnim = ANIM_SUMO;
        }
    }

    if (actor->user.ID == ZILLA_RUN_R0 && (currentLevel->gameflags & LEVEL_SW_DEATHEXIT_ZILLA))
    {
        if (actor->user.Health <= 0)
        //if (actor->user.Health < actor->user.MaxHealth)
        {
            FinishTimer = 15*120;
            FinishAnim = ANIM_ZILLA;
        }
    }

    if (actor->user.Attrib && RANDOM_P2(1024) > 850)
        PlaySpriteSound(actor,attr_pain,v3df_follow|v3df_dontpan);

    // keep track of the last damage
    if (amt < 0)
        actor->user.LastDamage = -amt;

    // Do alternate Death2 if it exists
    if (actor->user.ActorActionSet && actor->user.ActorActionSet->Death2) // JBF: added null check
    {
#define DEATH2_HEALTH_VALUE 15

        if (actor->user.Health  <= DEATH2_HEALTH_VALUE)
        {
            // If he's dead, possibly choose a special death type
#if 1 // Problematic code, REMOVED.
            switch (actor->user.ID)
            {
            case NINJA_RUN_R0:
            {
                extern STATE* sg_NinjaGrabThroat[];
                extern STATE* sg_NinjaHariKari[];

                if (actor->user.Flags2 & (SPR2_DYING)) return true;
                if (actor->user.Flags & (SPR_FALLING | SPR_JUMPING | SPR_CLIMBING)) return true;

                if (!(actor->user.Flags2 & SPR2_DYING))
                {
                    short rnd;

                    rnd = RANDOM_P2(1024<<4)>>4;
                    if (rnd < 950)
                        return true;
                    actor->user.Flags2 |= (SPR2_DYING); // Only let it check this once!
                    actor->user.WaitTics = SEC(1) + SEC(RandomRange(2));
                    actor->user.Health = 60;
                    PlaySound(DIGI_NINJACHOKE, actor, v3df_follow);
                    InitPlasmaFountain(nullptr, actor);
                    InitBloodSpray(actor,false,105);
                    actor->set_int_ang(NORM_ANGLE(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y) + 1024));
                    actor->spr.cstat &= ~(CSTAT_SPRITE_YFLIP);
                    if (sw_ninjahack)
                        NewStateGroup(actor, sg_NinjaHariKari);
                        else
                    NewStateGroup(actor, sg_NinjaGrabThroat);
                }
                break;
            }
            }
#endif
        }
    }

    return true;
}

int SopDamage(SECTOR_OBJECT* sop, short amt)
{
    auto actor = sop->sp_child;

    // does not have damage
    if (sop->max_damage == -9999)
        return false;

    sop->max_damage += amt;

    // keep track of the last damage
    if (amt < 0)
        actor->user.LastDamage = -amt;

    return true;
}

int SopCheckKill(SECTOR_OBJECT* sop)
{
    bool killed = false;

    if ((sop->flags & SOBJ_BROKEN))
        return false;

    // does not have damage
    if (sop->max_damage == -9999)
        return killed;

    if (sop->max_damage <= 0)
    {
        // returns whether so was collapsed(killed)
        killed = TestKillSectorObject(sop);
        if (!killed)
        {
            VehicleSetSmoke(sop, SpawnVehicleSmoke);
            sop->flags |= (SOBJ_BROKEN);
        }
    }

    return killed;
}

int ActorPain(DSWActor* actor)
{
    // uzi/shotgun damages
    switch (actor->user.ID)
    {
    case TOILETGIRL_R0:
    case WASHGIRL_R0:
    case CARGIRL_R0:
    case MECHANICGIRL_R0:
    case SAILORGIRL_R0:
    case PRUNEGIRL_R0:
        actor->user.FlagOwner = 1;
        break;
    }

    if (RandomRange(1000) < 875 || actor->user.WaitTics > 0)
        return false;

    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING)))
    {
        if (actor->user.ActorActionSet && actor->user.ActorActionSet->Pain)
        {
            ActorLeaveTrack(actor);
            actor->user.WaitTics = 60;
            NewStateGroup(actor, actor->user.ActorActionSet->Pain);
            return true;
        }
    }

    return false;
}

int ActorPainPlasma(DSWActor* actor)
{
    if (!(actor->user.Flags & (SPR_JUMPING | SPR_FALLING | SPR_ELECTRO_TOLERANT)))
    {
        if (actor->user.ActorActionSet && actor->user.ActorActionSet->Pain)
        {
            actor->user.WaitTics = PLASMA_FOUNTAIN_TIME;
            NewStateGroup(actor, actor->user.ActorActionSet->Pain);
            return true;
        }
        else
        {
            actor->user.Vis = PLASMA_FOUNTAIN_TIME;
            InitActorPause(actor);
        }
    }

    return false;
}

int ActorStdMissile(DSWActor* actor, DSWActor* weapActor)
{
    assert(weapActor != nullptr);

    // Attack the player that is attacking you
    // Only if hes still alive
    auto own = GetOwner(weapActor);
    if (own && own->hasU())
    {
        if (own->user.PlayerP && GetOwner(actor) != own)
        {
            actor->user.targetActor = own;
        }
    }

    // Reset the weapons target before dying
    DSWActor* goal = weapActor->user.WpnGoalActor;
    if (goal != nullptr)
    {
        // attempt to see if it was killed
        ASSERT(goal->insector());
        if (goal->hasU())
            goal->user.Flags &= ~(SPR_TARGETED);
    }

    return 0;
}

int ActorDamageSlide(DSWActor* actor, int damage, int ang)
{
    int slide_vel,slide_dec;

    if (actor->user.Flags & (SPR_CLIMBING))
        return false;

    damage = abs(damage);

    if (!damage)
        return false;

    if (damage <= 10)
    {
        DoActorBeginSlide(actor, ang, 64, 5);
        return true;
    }
    else if (damage <= 20)
    {
        DoActorBeginSlide(actor, ang, 128, 5);
        return true;
    }
    else
    {
        slide_vel = (damage * 6) - (actor->user.MaxHealth);

        if (slide_vel < -1000) slide_vel = -1000;
        slide_dec = 5;

        DoActorBeginSlide(actor, ang, slide_vel, slide_dec);

        return true;
    }
}

int PlayerDamageSlide(PLAYER* pp, int damage, short ang)
{
    int slide_vel;

    damage = abs(damage);

    if (!damage)
        return false;

    if (damage <= 5)
    {
        //nudge
        //pp->slide_xvect = MOVEx(4, ang)<<15;
        //pp->slide_yvect = MOVEy(4, ang)<<15;
        //return(true);
        return false;
    }
    else if (damage <= 10)
    {
        //nudge
        pp->slide_vect.X = MOVEx(16, ang)<<15;
        pp->slide_vect.Y = MOVEy(16, ang)<<15;
        return true;
    }
    else if (damage <= 20)
    {
        //bigger nudge
        pp->slide_vect.X = MOVEx(64, ang)<<15;
        pp->slide_vect.Y = MOVEy(64, ang)<<15;
        return true;
    }
    else
    {
        slide_vel = (damage * 6);

        pp->slide_vect.X = MOVEx(slide_vel, ang)<<15;
        pp->slide_vect.Y = MOVEy(slide_vel, ang)<<15;

        return true;
    }
}


int GetDamage(DSWActor* actor, DSWActor* weapActor, int DamageNdx)
{
    auto d = &DamageData[DamageNdx];

    // if ndx does radius
    if (d->radius > 0 && weapActor)
    {
        int dist,a,b,c;
        int damage_per_pixel, damage_force, damage_amt;


        DISTANCE(weapActor->int_pos().X,weapActor->int_pos().Y,actor->int_pos().X,actor->int_pos().Y,dist,a,b,c);

        // take off the box around the player or else you'll never get
        // the max_damage;
        dist -= ((int)actor->spr.clipdist)<<(2);

        if (dist < 0) dist = 0;

        if ((unsigned)dist < d->radius)
        {
            damage_per_pixel = IntToFixed(d->damage_hi)/d->radius;

            //the closer your distance is to 0 the more damage
            damage_force = (d->radius - dist);
            damage_amt = -FixedToInt(damage_force * damage_per_pixel);

            //return(damage_amt);
            // formula: damage_amt = 75% + random(25%)
            return (damage_amt >> 1) + (damage_amt >> 2) + RandomRange(damage_amt >> 2);
        }
        else
        {
            return 0;
        }
    }

    return -(d->damage_lo + RandomRange(d->damage_hi - d->damage_lo));
}


int PlayerCheckDeath(PLAYER* pp, DSWActor* weapActor)
{
    DSWActor* actor = pp->actor;

    // Store off what player was struck by
    pp->HitBy = weapActor;

    if (actor->user.Health <= 0 && !(pp->Flags & PF_DEAD))
    {
        // pick a death type
        if (actor->user.LastDamage >= PLAYER_DEATH_EXPLODE_DAMMAGE_AMT)
            pp->DeathType = PLAYER_DEATH_EXPLODE;
        else if (actor->user.LastDamage >= PLAYER_DEATH_CRUMBLE_DAMMAGE_AMT)
            pp->DeathType = PLAYER_DEATH_CRUMBLE;
        else
            pp->DeathType = PLAYER_DEATH_FLIP;

        if (weapActor == nullptr)
        {
            pp->KillerActor = nullptr;
            DoPlayerBeginDie(pp);
            return true;
        }

        if (weapActor != nullptr && (weapActor->user.ID == RIPPER_RUN_R0 || weapActor->user.ID == RIPPER2_RUN_R0))
            pp->DeathType = PLAYER_DEATH_RIPPER;

        if (weapActor != nullptr && weapActor->user.ID == CALTROPS)
            pp->DeathType = PLAYER_DEATH_FLIP;

        if (weapActor != nullptr && weapActor->user.ID == NINJA_RUN_R0 && weapActor->user.PlayerP)
        {
            pp->DeathType = PLAYER_DEATH_FLIP;
            weapActor->user.PlayerP->Bloody = true;
        }

        // keep track of who killed you for death purposes
        // need to check all Killer variables when an enemy dies
        if (pp->KillerActor == nullptr)
        {
            auto own = GetOwner(weapActor);
            if (own)
                pp->KillerActor = own;
            else if ((weapActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                pp->KillerActor = weapActor;
        }

        // start the death process
        DoPlayerBeginDie(pp);

        // for death direction
        actor->user.slide_ang = getangle(actor->int_pos().X - weapActor->int_pos().X, actor->int_pos().Y - weapActor->int_pos().Y);
        // for death velocity
        actor->user.slide_vel = actor->user.LastDamage * 5;

        return true;
    }

    return false;
}

bool PlayerTakeDamage(PLAYER* pp, DSWActor* weapActor)
{
    if (weapActor == nullptr)
        return true;

    DSWActor* actor = pp->actor;

    auto weapOwner = GetOwner(weapActor);

    if (gNet.MultiGameType == MULTI_GAME_NONE)
    {
        // ZOMBIE special case for single play
        if (weapActor->user.ID == ZOMBIE_RUN_R0)
        {
            // if weapons Owner the player
            if (weapOwner == pp->actor)
                return false;
        }

        return true;
    }

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
    {
        // everything hurts you
        if (gNet.HurtTeammate)
            return true;

        // if weapon IS the YOURSELF take damage
        if (weapActor->user.PlayerP == pp)
            return true;

        // if the weapons Owner is YOURSELF take damage
        if (weapOwner && weapOwner->hasU() && weapOwner->user.PlayerP == pp)
            return true;

        // if weapon IS the player no damage
        if (weapActor->user.PlayerP)
            return false;

        // if the weapons Owner is a player
        if (weapOwner && weapOwner->hasU() && weapOwner->user.PlayerP)
            return false;
    }
    else if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.TeamPlay)
    {
        // everything hurts you
        if (gNet.HurtTeammate)
            return true;

        // if weapon IS the YOURSELF take damage
        if (weapActor->user.PlayerP == pp)
            return true;

        // if the weapons Owner is YOURSELF take damage
        if (weapOwner && weapOwner->hasU() && weapOwner->user.PlayerP == pp)
            return true;

        if (weapActor->user.PlayerP)
        {
            // if both on the same team then no damage
            if (weapActor->user.spal == actor->user.spal)
                return false;
        }

        // if the weapons Owner is a player
        if (weapOwner && weapOwner->hasU() && weapOwner->user.PlayerP)
        {
            // if both on the same team then no damage
            if (weapOwner->user.spal == actor->user.spal)
                return false;
        }
    }

    return true;
}


int StarBlood(DSWActor* actor, DSWActor* weapActor)
{
    short blood_num = 1;
    short i;

    if (actor->user.Health <= 0)
        blood_num = 4;

    for (i = 0; i < blood_num; i++)
        SpawnBlood(actor, weapActor, 0, 0, 0, 0);
    return 0;
}


/*

!AIC KEY - This is where damage is assesed when missiles hit actors and other
objects.

*/

// this was done wrong multiple times below, resulting in spurious crashes.
bool OwnerIs(DSWActor* actor, int pic)
{
    auto Own = GetOwner(actor);
    if (Own == nullptr || !Own->hasU()) return false;
    return Own->user.ID == pic;
}



int DoDamage(DSWActor* actor, DSWActor* weapActor)
{
    int damage=0;

    ASSERT(actor->hasU());

    // don't hit a dead player
    if (actor->user.PlayerP && (actor->user.PlayerP->Flags & PF_DEAD))
    {
        SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        return 0;
    }

    if (!weapActor || !weapActor->hasU() || (actor->user.Flags & SPR_SUICIDE))
        return 0;

    if ((weapActor->user.Flags & SPR_SUICIDE))
        return 0;

    if (actor->user.Attrib && RANDOM_P2(1024) > 850)
        PlaySpriteSound(actor,attr_pain,v3df_follow);

    if (actor->user.Flags & (SPR_DEAD))
    {
        SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        return 0;
    }


    // special case for shooting mines
    if (actor->spr.statnum == STAT_MINE_STUCK)
    {
        SpawnMineExp(actor);
        KillActor(actor);
        return 0;
    }

    // weapon is drivable object manned by player
    if (weapActor->user.PlayerP && weapActor->user.PlayerP->sop)
    {
        switch (weapActor->user.PlayerP->sop->track)
        {
        case SO_VEHICLE:
            damage = -200;

            if (actor->user.sop_parent)
            {
                break;
            }
            else if (actor->user.PlayerP)
            {
                PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
                if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
                {
                    PlayerUpdateHealth(actor->user.PlayerP, damage);
                    PlayerCheckDeath(actor->user.PlayerP, weapActor);
                }
            }
            else
            {
                PLAYER* pp = Player + screenpeek;

                ActorHealth(actor, damage);
                if (actor->user.Health <= 0)
                {
                    int choosesnd=0;
                    // Random chance of taunting the AI's here
                    if (StdRandomRange(1024) > 512 && pp == Player+myconnectindex)
                    {
                        choosesnd=RandomRange(MAX_TAUNTAI);
                        PlayerSound(TauntAIVocs[choosesnd],v3df_dontpan|v3df_follow,pp);
                    }
                    SpawnShrap(actor, weapActor);
                    SetSuicide(actor);
                    return 0;
                }
            }
            break;

            }
                }

    // weapon is the actor - no missile used - example swords, axes, etc
    switch (weapActor->user.ID)
    {
    case NINJA_RUN_R0:

        ASSERT(weapActor->user.PlayerP);

        if (weapActor->user.WeaponNum == WPN_SWORD)
            damage = GetDamage(actor, weapActor, WPN_SWORD);
        else
        {
            damage = GetDamage(actor, weapActor, WPN_FIST);
            // Add damage for stronger attacks!
            switch (weapActor->user.PlayerP->WpnKungFuMove)
            {
            case 1:
                damage -= 2 - RandomRange(7);
                break;
            case 2:
                damage -= 5 - RandomRange(10);
                break;
            }
            PlaySound(DIGI_CGTHIGHBONE,weapActor,v3df_follow|v3df_dontpan);
        }

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case SKEL_RUN_R0:
    case COOLG_RUN_R0:
    case GORO_RUN_R0:

        damage = GetDamage(actor, weapActor, DMG_SKEL_SLASH);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case HORNET_RUN_R0:
        PlaySound(DIGI_HORNETSTING, actor, v3df_follow|v3df_dontpan);
        damage = GetDamage(actor, weapActor, DMG_HORNET_STING);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case EEL_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_RIPPER_SLASH);
        damage /= 3;
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case RIPPER_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_RIPPER_SLASH);
        damage /= 3; // Little rippers aren't as tough.
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                if (PlayerCheckDeath(actor->user.PlayerP, weapActor))
                {
                    PlaySound(DIGI_RIPPERHEARTOUT,actor->user.PlayerP,v3df_dontpan|v3df_doppler);

                    DoRipperRipHeart(weapActor);
                }
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case RIPPER2_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_RIPPER_SLASH);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                if (PlayerCheckDeath(actor->user.PlayerP, weapActor))
                {
                    PlaySound(DIGI_RIPPERHEARTOUT,actor->user.PlayerP,v3df_dontpan|v3df_doppler);

                    DoRipper2RipHeart(weapActor);
                }
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case BUNNY_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_RIPPER_SLASH);
        damage /= 3;
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                if (PlayerCheckDeath(actor->user.PlayerP, weapActor))
                {
                    DoBunnyRipHeart(weapActor);
                }
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case SERP_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_SERP_SLASH);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage/4, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case BLADE1:
    case BLADE2:
    case BLADE3:
    case 5011:

        if (weapActor->user.ID == 5011)
            damage = -(3 + (RandomRange(4<<8)>>8));
        else
            damage = GetDamage(actor, weapActor, DMG_BLADE);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            if ((actor->user.BladeDamageTics -= synctics) < 0)
            {
                actor->user.BladeDamageTics = DAMAGE_BLADE_TIME;
                PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
                if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
                {
                    PlayerUpdateHealth(actor->user.PlayerP, damage);
                    PlayerCheckDeath(actor->user.PlayerP, weapActor);
                }
            }
        }
        else
        {
            if ((actor->user.BladeDamageTics -= ACTORMOVETICS) < 0)
            {
                actor->user.BladeDamageTics = DAMAGE_BLADE_TIME;
                ActorHealth(actor, damage);
            }

            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        break;

    case STAR1:
    case CROSSBOLT:
        damage = GetDamage(actor, weapActor, WPN_STAR);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            // Is the player blocking?
            if (actor->user.PlayerP->WpnKungFuMove == 3)
                damage /= 3;
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        StarBlood(actor, weapActor);

        weapActor->user.ID = 0;
        SetSuicide(weapActor);
        break;

    case SPEAR_R0:
        damage = GetDamage(actor, weapActor, DMG_SPEAR_TRAP);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        weapActor->user.ID = 0;
        SetSuicide(weapActor);
        break;

    case LAVA_BOULDER:
        damage = GetDamage(actor, weapActor, DMG_LAVA_BOULDER);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        weapActor->user.ID = 0;
        SetSuicide(weapActor);
        break;

    case LAVA_SHARD:
        damage = GetDamage(actor, weapActor, DMG_LAVA_SHARD);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBlood(actor, weapActor, 0, 0, 0, 0);

        weapActor->user.ID = 0;
        SetSuicide(weapActor);
        break;

    case UZI_SMOKE:
    case UZI_SMOKE+2:
        if (weapActor->user.ID == UZI_SMOKE)
            damage = GetDamage(actor, weapActor, WPN_UZI);
        else
            damage = GetDamage(actor, weapActor, WPN_UZI)/3; // Enemy Uzi, 1/3 damage

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            //PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            PlayerDamageSlide(actor->user.PlayerP, damage/2, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
            switch (actor->user.ID)
            {
            case TRASHCAN:
            case PACHINKO1:
            case PACHINKO2:
            case PACHINKO3:
            case PACHINKO4:
            case 623:
            case ZILLA_RUN_R0:
                break;
            default:
                if (RandomRange(1000) > 900)
                    InitBloodSpray(actor,false,105);
                if (RandomRange(1000) > 900)
                    SpawnMidSplash(actor);
                break;
            }
        }

        //SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        // reset id so no more damage is taken
        weapActor->user.ID = 0;
        break;

    case SHOTGUN_SMOKE:
        damage = GetDamage(actor, weapActor, WPN_SHOTGUN);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        //SpawnBlood(actor, weapActor, 0, 0, 0, 0);
        switch (actor->user.ID)
        {
        case TRASHCAN:
        case PACHINKO1:
        case PACHINKO2:
        case PACHINKO3:
        case PACHINKO4:
        case 623:
        case ZILLA_RUN_R0:
            break;
        default:
            if (RandomRange(1000) > 950)
                SpawnMidSplash(actor);
            break;
        }

        // reset id so no more damage is taken
        weapActor->user.ID = 0;
        break;

    case MIRV_METEOR:

        //damage = -DAMAGE_MIRV_METEOR;
        damage = GetDamage(actor, weapActor, DMG_MIRV_METEOR);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
        }

        SetSuicide(weapActor);
        break;

    case SERP_METEOR:

        //damage = -DAMAGE_SERP_METEOR;
        damage = GetDamage(actor, weapActor, DMG_SERP_METEOR);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
        }

        SetSuicide(weapActor);
        break;

    case BOLT_THINMAN_R0:
        damage = GetDamage(actor, weapActor, WPN_ROCKET);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
        }

        if (weapActor->user.Radius == NUKE_RADIUS)
            SpawnNuclearExp(weapActor);
        else
            SpawnBoltExp(weapActor);
        SetSuicide(weapActor);
        break;

    case BOLT_THINMAN_R1:
        //damage =  -(2000 + (65 + RandomRange(40))); // -2000 makes armor not count
        damage =  -(65 + RandomRange(40));

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            // this is special code to prevent the Zombie from taking out the Bosses to quick
            // if rail gun weapon Owner is not player
            auto own = GetOwner(weapActor);
            if (own && own->hasU() && !own->user.PlayerP)
            {
                // if actor is a boss
                if (actor->user.ID == ZILLA_RUN_R0 || actor->user.ID == SERP_RUN_R0 || actor->user.ID == SUMO_RUN_R0)
                    damage /= 2;
            }

            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage>>1, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        weapActor->user.ID = 0; // No more damage
        SpawnTracerExp(weapActor);
        SetSuicide(weapActor);
        break;

    case BOLT_THINMAN_R2:
        damage = (GetDamage(actor, weapActor, WPN_ROCKET)/2);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
        }

        if (weapActor->user.Radius == NUKE_RADIUS)
            SpawnNuclearExp(weapActor);
        else
            SpawnBoltExp(weapActor);
        SetSuicide(weapActor);
        break;

    case BOLT_THINMAN_R4:
        damage = GetDamage(actor, weapActor, DMG_GRENADE_EXP);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        SpawnBunnyExp(weapActor);
        SetSuicide(weapActor);
        break;

    case SUMO_RUN_R0:
        damage = GetDamage(actor, weapActor, DMG_FLASHBOMB);

        damage /= 3;
        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case BOLT_EXP:
        damage = GetDamage(actor, weapActor, DMG_BOLT_EXP);
        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case BLOOD_WORM:

        // Don't hurt blood worm zombies!
        if (actor->user.ID == ZOMBIE_RUN_R0)
            break;

        damage = GetDamage(actor, weapActor, WPN_HEART);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            //PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                if (PlayerCheckDeath(actor->user.PlayerP, weapActor))
                {
                    // degrade blood worm life
                    weapActor->user.Counter3 += (4*120)/MISSILEMOVETICS;
                }
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        // degrade blood worm life
        weapActor->user.Counter3 += (2*120)/MISSILEMOVETICS;

        break;


    case TANK_SHELL_EXP:
        damage = GetDamage(actor, weapActor, DMG_TANK_SHELL_EXP);
        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case MUSHROOM_CLOUD:
    case GRENADE_EXP:
        if (weapActor->user.Radius == NUKE_RADIUS) // Special Nuke stuff
            damage = (GetDamage(actor, weapActor, DMG_NUCLEAR_EXP));
        else
            damage = GetDamage(actor, weapActor, DMG_GRENADE_EXP);

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (OwnerIs(weapActor, SUMO_RUN_R0)) break;
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case MICRO_EXP:

        damage = GetDamage(actor, weapActor, DMG_MINE_EXP);

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }
        break;

    case MINE_EXP:
    {
        damage = GetDamage(actor, weapActor, DMG_MINE_EXP);
        if (OwnerIs(weapActor, SERP_RUN_R0))
        {
            damage /= 6;
        }

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            auto own = GetOwner(weapActor);
            if (own && own->hasU())
            {
                // Don't let serp skulls hurt the Serpent God
                if (OwnerIs(weapActor, SERP_RUN_R0)) break;
                // Don't let it hurt the SUMO
                if (OwnerIs(weapActor, SUMO_RUN_R0)) break;
            }
            if (actor->user.ID == TRASHCAN)
                ActorHealth(actor, -500);
            else
                ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        // reset id so no more damage is taken
        weapActor->user.ID = 0;
        break;
    }
#if 0
    case MINE_SHRAP:

        damage = GetDamage(actor, weapActor, DMG_MINE_SHRAP);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->spr.angle);
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->spr.angle);
            ActorChooseDeath(actor, weapActor);
        }

        // reset id so no more damage is taken
        weapActor->user.ID = 0;
        break;
#endif

    case NAP_EXP:
    {
        damage = GetDamage(actor, weapActor, DMG_NAPALM_EXP);

        // Sumo Nap does less
        if (OwnerIs(weapActor, SUMO_RUN_R0))
        damage /= 4;

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (OwnerIs(weapActor, SUMO_RUN_R0)) break;
            ActorHealth(actor, damage);
            ActorChooseDeath(actor, weapActor);
        }

        SetSuicide(weapActor);
        break;
    }
    case Vomit1:
    case Vomit2:

        damage = GetDamage(actor, weapActor, DMG_VOMIT);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

        SetSuicide(weapActor);
        break;

    case COOLG_FIRE:
        damage = GetDamage(actor, weapActor, DMG_COOLG_FIRE);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

//        actor->user.ID = 0;
        SetSuicide(weapActor);
        break;

    // Skull Exp
    case SKULL_R0:
    case BETTY_R0:

        damage = GetDamage(actor, weapActor, DMG_SKULL_EXP);

        if (actor->user.sop_parent)
        {
            if (actor->user.sop_parent->flags & (SOBJ_DIE_HARD))
                break;
            SopDamage(actor->user.sop_parent, damage);
            SopCheckKill(actor->user.sop_parent);
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, AngToPlayer(actor->user.PlayerP, weapActor));
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorDamageSlide(actor, damage, AngToSprite(actor, weapActor));
            ActorChooseDeath(actor, weapActor);
        }

        break;

    // Serp ring of skull
    case SKULL_SERP:
        //DoSkullBeginDeath(Weapon);
        break;

    case FIREBALL1:
    {
        damage = GetDamage(actor, weapActor, WPN_HOTHEAD);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        auto own = GetOwner(weapActor);
        if (own && own->hasU()) // For SerpGod Ring
            own->user.Counter--;
        SpawnFireballFlames(weapActor, actor);
        SetSuicide(weapActor);
        break;
    }
    case FIREBALL:
    case GORO_FIREBALL:

        damage = GetDamage(actor, weapActor, DMG_GORO_FIREBALL);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            PlayerDamageSlide(actor->user.PlayerP, damage, weapActor->int_ang());
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
            if (actor->user.PlayerP->Armor)
                PlaySound(DIGI_ARMORHIT,actor->user.PlayerP,v3df_dontpan|v3df_follow|v3df_doppler);
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorDamageSlide(actor, damage, weapActor->int_ang());
            ActorChooseDeath(actor, weapActor);
        }

        SpawnGoroFireballExp(weapActor);
        SetSuicide(weapActor);
        break;

    case FIREBALL_FLAMES:

        damage = -DamageData[DMG_FIREBALL_FLAMES].damage_lo;

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorChooseDeath(actor, weapActor);
        }

        break;

    case RADIATION_CLOUD:

        damage = GetDamage(actor, weapActor, DMG_RADIATION_CLOUD);

        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                PLAYER* pp = actor->user.PlayerP;

                PlayerSound(DIGI_GASHURT, v3df_dontpan|v3df_follow|v3df_doppler,pp);
                PlayerUpdateHealth(actor->user.PlayerP, damage-1000);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            // Don't let it hurt the SUMO
            if (OwnerIs(weapActor, SUMO_RUN_R0)) break;
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorChooseDeath(actor, weapActor);
        }

//        actor->user.ID = 0;
        weapActor->user.ID = 0;
        break;

    case PLASMA:

        //damage = GetDamage(actor, weapActor, WPN_HEART);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            //PlayerUpdateHealth(actor->user.PlayerP, damage);
            //PlayerCheckDeath(actor->user.PlayerP, weapActor);
        }
        else
        {
            if (actor->user.ID == SKULL_R0 || actor->user.ID == BETTY_R0)
            {
                ActorHealth(actor, damage);
                ActorStdMissile(actor, weapActor);
                ActorChooseDeath(actor, weapActor);
                SetSuicide(weapActor);
                break;
            }
            else if (actor->user.ID == RIPPER_RUN_R0)
            {
                DoRipperGrow(actor);
                break;
            }

            ActorPainPlasma(actor);
        }

        InitPlasmaFountain(weapActor, actor);

        SetSuicide(weapActor);

        break;

    case CALTROPS:
    {
        damage = GetDamage(actor, weapActor, DMG_MINE_SHRAP);
        if (actor->user.sop_parent)
        {
            break;
        }
        else if (actor->user.PlayerP)
        {
            if (PlayerTakeDamage(actor->user.PlayerP, weapActor))
            {
                if (RANDOM_P2(1024<<4)>>4 < 800)
                    PlayerSound(DIGI_STEPONCALTROPS, v3df_follow|v3df_dontpan, actor->user.PlayerP);
                PlayerUpdateHealth(actor->user.PlayerP, damage);
                PlayerCheckDeath(actor->user.PlayerP, weapActor);
            }
        }
        else
        {
            ActorHealth(actor, damage);
            ActorPain(actor);
            ActorStdMissile(actor, weapActor);
            ActorChooseDeath(actor, weapActor);
        }

        SetSuicide(weapActor);
        break;
        }
            }

    // If player take alot of damage, make him yell
    if (actor->hasU() && actor->user.PlayerP)
    {
        if (damage <= -40 && RandomRange(1000) > 700)
            PlayerSound(DIGI_SONOFABITCH, v3df_dontpan|v3df_follow, actor->user.PlayerP);
        else if (damage <= -40 && RandomRange(1000) > 700)
            PlayerSound(DIGI_PAINFORWEAK, v3df_dontpan|v3df_follow, actor->user.PlayerP);
        else if (damage <= -10)
            PlayerSound(PlayerPainVocs[RandomRange(MAX_PAIN)], v3df_dontpan|v3df_follow, actor->user.PlayerP);
    }

    return 0;
}

// Select death text based on ID
const char *DeathString(DSWActor* actor)
{
    if (!actor->hasU()) return " ";

    switch (actor->user.ID)
    {
    case NINJA_RUN_R0:
        return " ";
    case ZOMBIE_RUN_R0:
        return GStrings("Zombie");
    case BLOOD_WORM:
        return GStrings("Blood Worm");
    case SKEL_RUN_R0:
        return GStrings("Skeletor Priest");
    case COOLG_RUN_R0:
        return GStrings("Coolie Ghost");
    case GORO_RUN_R0:
        return GStrings("Guardian");
    case HORNET_RUN_R0:
        return GStrings("Hornet");
    case RIPPER_RUN_R0:
        return GStrings("Ripper Hatchling");
    case RIPPER2_RUN_R0:
        return GStrings("Ripper");
    case BUNNY_RUN_R0:
        return GStrings("Killer Rabbit");
    case SERP_RUN_R0:
        return GStrings("Serpent god");
    case GIRLNINJA_RUN_R0:
        return GStrings("Girl Ninja");
    case BLADE1:
    case BLADE2:
    case BLADE3:
    case 5011:
        return GStrings("blade");
    case STAR1:
        if (sw_darts) return GStrings("dart");
        else return GStrings("shuriken");
    case CROSSBOLT:
        return GStrings("crossbow bolt");
    case SPEAR_R0:
        return GStrings("spear");
    case LAVA_BOULDER:
    case LAVA_SHARD:
        return GStrings("lava boulder");
    case UZI_SMOKE:
        return GStrings("Uzi");
    case UZI_SMOKE+2:
        return GStrings("Evil Ninja Uzi");
    case SHOTGUN_SMOKE:
        return GStrings("shotgun");
    case MIRV_METEOR:
    case SERP_METEOR:
        return GStrings("meteor");
    case BOLT_THINMAN_R0:
        return GStrings("rocket");
    case BOLT_THINMAN_R1:
        return GStrings("rail gun");
    case BOLT_THINMAN_R2:
        return GStrings("enemy rocket");
    case BOLT_THINMAN_R4:  // BunnyRocket
        return GStrings("bunny rocket");
    case BOLT_EXP:
        return GStrings("explosion");
    case TANK_SHELL_EXP:
        return GStrings("tank shell");
    case MUSHROOM_CLOUD:
        return GStrings("nuclear bomb");
    case GRENADE_EXP:
        return GStrings("40mm grenade");
    case MICRO_EXP:
        return GStrings("micro missile");
    case MINE_EXP:
        //case MINE_SHRAP:
        return GStrings("sticky bomb");
    case NAP_EXP:
        return GStrings("napalm");
    case Vomit1:
    case Vomit2:
        return GStrings("vomit");
    case COOLG_FIRE:
        return GStrings("Coolie Ghost phlem");
    case SKULL_R0:
        return GStrings("Accursed Head");
    case BETTY_R0:
        return GStrings("Bouncing Betty");
    case SKULL_SERP:
        return GStrings("Serpent god Protector");
    case FIREBALL1:
    case FIREBALL:
    case GORO_FIREBALL:
    case FIREBALL_FLAMES:
        return GStrings("flames");
    case RADIATION_CLOUD:
        return GStrings("radiation");
    case CALTROPS:
        return GStrings("useitem 7");
    }
    return "";
}

int DoDamageTest(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, tx, ty;
    int tmin;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);
            if ((unsigned)dist > actor->user.Radius + itActor->user.Radius)
                continue;

            if (actor == itActor)
                continue;

            if (!(itActor->spr.cstat & CSTAT_SPRITE_BLOCK))
                continue;

            // !JIM! Put in a cansee so that you don't take damage through walls and such
            // For speed's sake, try limiting check only to radius weapons!
            if (actor->user.Radius > 200)
            {
                if (!FAFcansee(itActor->int_pos().X,itActor->int_pos().Y, ActorUpperZ(actor), itActor->sector(),actor->int_pos().X,actor->int_pos().Y,actor->int_pos().Z,actor->sector()))
                    continue;
            }

            if (GetOwner(actor) != itActor && SpriteOverlap(actor, itActor))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

static void DoHitscanDamage(DSWActor* weaponActor, DSWActor* hitActor)
{
    if (hitActor == nullptr)
        return;

    unsigned stat;

    // this routine needs some sort of sprite generated from the hitscan
    // such as a smoke or spark sprite - reason is because of DoDamage()

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        if (hitActor->spr.statnum == StatDamageList[stat])
        {
            DoDamage(hitActor, weaponActor);
            break;
        }
    }
}

int DoFlamesDamageTest(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, tx, ty;
    int tmin;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            switch (itActor->user.ID)
            {
            case TRASHCAN:
            case PACHINKO1:
            case PACHINKO2:
            case PACHINKO3:
            case PACHINKO4:
            case 623:
                continue;
            }

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);

            if ((unsigned)dist > actor->user.Radius + itActor->user.Radius)
                continue;

            if (actor == itActor)
                continue;

            if (!(itActor->spr.cstat & (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN)))
                continue;

            if (actor->spr.cstat & (CSTAT_SPRITE_INVISIBLE))
                continue;

            if (actor->user.Radius > 200) // Note: No weaps have bigger radius than 200 cept explosion stuff
            {
                if (FAFcansee(itActor->int_pos().X,itActor->int_pos().Y,int_ActorZOfMiddle(actor),itActor->sector(),actor->int_pos().X,actor->int_pos().Y,int_ActorZOfMiddle(actor),actor->sector()))
                {
                    DoDamage(itActor, actor);
                }
            }
            else if (SpriteOverlap(actor, itActor))
            {
                DoDamage(itActor, actor);
            }

        }
    }

    return 0;
}

walltype* PrevWall(walltype* wall_num)
{
    for(auto&wal : wallsofsector(wall_num->sectorp()))
    {
        if (wal.point2Wall() == wall_num) return &wal;
    }
    return wall_num; // should never happen unless the sector is malformed.
}


short StatBreakList[] =
{
    STAT_DEFAULT,
    STAT_BREAKABLE,
    STAT_NO_STATE,
    STAT_DEAD_ACTOR,
};

void TraverseBreakableWalls(sectortype* start_sect, int x, int y, int z, short ang, int radius)
{
    int k;
    int xmid,ymid;
    int dist;
    int break_count;

    int wall_ang;
    int hit_x,hit_y,hit_z;


    // limit radius
    if (radius > 2000)
        radius = 2000;

    break_count = 0;

    BFSSectorSearch search(start_sect);
    while (auto sect = search.GetNext())
    {
        for(auto& wal : wallsofsector(sect))
        {
            // see if this wall should be broken
            if (wal.lotag == TAG_WALL_BREAK)
            {
                // find midpoint
                xmid = (wal.wall_int_pos().X + wal.point2Wall()->wall_int_pos().X) >> 1;
                ymid = (wal.wall_int_pos().Y + wal.point2Wall()->wall_int_pos().Y) >> 1;

                // don't need to go further if wall is too far out

                dist = Distance(xmid, ymid, x, y);
                if (dist > radius)
                    continue;

                sectortype* sectp = nullptr;
                if (WallBreakPosition(&wal, &sectp, &hit_x, &hit_y, &hit_z, &wall_ang))
                {
                    if (hit_x != INT32_MAX && sectp != nullptr && FAFcansee(x, y, z, start_sect, hit_x, hit_y, hit_z, sectp))
                    {
                        HitBreakWall(&wal, INT32_MAX, INT32_MAX, INT32_MAX, ang, 0);

                        break_count++;
                        if (break_count > 4)
                        {
                            return;
                        }
                    }
                }
            }

            if (wal.twoSided())
                search.Add(wal.nextSector());
        }

    }
}


int DoExpDamageTest(DSWActor* actor)
{
    short i, stat;
    int dist, tx, ty;
    int tmin;
    int max_stat;
    short break_count;

    DSWActor* found_act = nullptr;
    int found_dist = 999999;
    int DoWallMoveMatch(short match);

    // crack sprites
    if (actor->user.ID != MUSHROOM_CLOUD)
        WeaponExplodeSectorInRange(actor);

    // Just like DoDamageTest() except that it doesn't care about the Owner

    max_stat = SIZ(StatDamageList);
    // don't check for mines if the weapon is a mine
    if (actor->spr.statnum == STAT_MINE_STUCK)
        max_stat--;

    for (stat = 0; stat < max_stat; stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);

            if ((unsigned)dist > actor->user.Radius + itActor->user.Radius)
                continue;

            if (itActor == actor)
                continue;

            if (StatDamageList[stat] == STAT_SO_SP_CHILD)
            {
                DoDamage(itActor, actor);
            }
            else
            {
                if ((unsigned)FindDistance3D(itActor->int_pos().X - actor->int_pos().X, itActor->int_pos().Y - actor->int_pos().Y, itActor->int_pos().Z - actor->int_pos().Z) > actor->user.Radius + itActor->user.Radius)
                    continue;

                // added hitscan block because mines no long clip against actors/players
                if (!(itActor->spr.cstat & (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN)))
                    continue;

                // Second parameter MUST have blocking bits set or cansee won't work
                // added second check for FAF water - hitscans were hitting ceiling
                if (!FAFcansee(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), itActor->int_pos().X, itActor->int_pos().Y, ActorUpperZ(actor), itActor->sector()) &&
                    !FAFcansee(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), itActor->int_pos().X, itActor->int_pos().Y, int_ActorLowerZ(actor), itActor->sector()))
                    continue;

                DoDamage(itActor, actor);
            }
        }
    }

    if (actor->user.ID == MUSHROOM_CLOUD) return 0;   // Central Nuke doesn't break stuff
    // Only secondaries do that

    TraverseBreakableWalls(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.Radius);

    break_count = 0;
    max_stat = SIZ(StatBreakList);
    // Breakable stuff
    for (stat = 0; stat < max_stat; stat++)
    {
        SWStatIterator it(StatBreakList[stat]);
        while (auto itActor = it.Next())
        {
            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);
            if ((unsigned)dist > actor->user.Radius)
                continue;

            dist = FindDistance3D(itActor->int_pos().X - actor->int_pos().X, itActor->int_pos().Y - actor->int_pos().Y, int_ActorZOfMiddle(itActor) - actor->int_pos().Z);
            if ((unsigned)dist > actor->user.Radius)
                continue;

            if (!FAFcansee(itActor->int_pos().X, itActor->int_pos().Y, int_ActorZOfMiddle(itActor), itActor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector()))
                continue;

            if ((itActor->spr.extra & SPRX_BREAKABLE))
            {
                HitBreakSprite(itActor, actor->user.ID);
                break_count++;
                if (break_count > 6)
                    break;
            }
        }
    }

    if (actor->user.ID == BLOOD_WORM)
        return 0;

    // wall damaging
    SWStatIterator it(STAT_WALL_MOVE);
    while (auto itActor = it.Next())
    {
        DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);
        if ((unsigned)dist > actor->user.Radius/4)
            continue;

        if (TEST_BOOL1(actor))
            continue;

        if (!CanSeeWallMove(actor, SP_TAG2(actor)))
            continue;

        if (dist < found_dist)
        {
            found_dist = dist;
            found_act = itActor;
        }
    }

    if (found_act)
    {
        if (SP_TAG2(found_act) == 0)
        {
            // just do one
            DoWallMove(found_act);
        }
        else
        {
            if (DoWallMoveMatch(SP_TAG2(found_act)))
            {
                DoSpawnSpotsForDamage(SP_TAG2(found_act));
            }
        }
    }
    return 0;
}


int DoMineExpMine(DSWActor* actor)
{
    int i;
    int dist, tx, ty;
    int tmin;
    int zdist;

    SWStatIterator it(STAT_MINE_STUCK);
    while (auto itActor = it.Next())
    {
        DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);
        if ((unsigned)dist > actor->user.Radius + itActor->user.Radius)
            continue;

        if (itActor == actor)
            continue;

        if (!(itActor->spr.cstat & CSTAT_SPRITE_BLOCK_HITSCAN))
            continue;

        // Explosions are spherical, not planes, so let's check that way, well cylindrical at least.
        zdist = abs(itActor->int_pos().Z - actor->int_pos().Z)>>4;
        if (SpriteOverlap(actor, itActor) || (unsigned)zdist < actor->user.Radius + itActor->user.Radius)
        {
            DoDamage(itActor, actor);
            // only explode one mine at a time
            break;
        }
    }

    return 0;
}

int DoStar(DSWActor* actor)
{
    int vel;

    const int STAR_STICK_RNUM = 400;
    const int STAR_BOUNCE_RNUM = 600;


    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        actor->user.motion_blur_num = 0;
        ScaleSpriteVector(actor, 54000);

        vel = ksqrt(SQ(actor->user.change.X) + SQ(actor->user.change.Y));

        if (vel > 100)
        {
            if ((RANDOM_P2(1024 << 4) >> 4) < 128)
                SpawnBubble(actor);
        }

        actor->spr.pos.Z += 0.5 * MISSILEMOVETICS;

        DoActorZrange(actor);
        MissileWaterAdjust(actor);

        if (actor->spr.pos.Z > actor->user.loz)
        {
            KillActor(actor);
            return true;
        }
    }
    else
    {
        vel = ksqrt(SQ(actor->user.change.X) + SQ(actor->user.change.Y));


        if (vel < 800)
        {
            actor->user.Counter += 50;
            actor->user.change.Z += actor->user.Counter;
        }
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone && !(actor->user.Flags & SPR_UNDERWATER))
    {
        switch (actor->user.coll.type)
        {
        default:
            break;

        case kHitWall:
        {
            short nw,wall_ang;
            walltype* wph;

            wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            // special case with MissileSetPos - don't queue star
            // from this routine
            if (actor->user.Flags & (SPR_SET_POS_DONT_KILL))
                break;

            // chance of sticking
            if (!(actor->user.Flags & SPR_BOUNCE) && RANDOM_P2(1024) < STAR_STICK_RNUM)
            {
                actor->user.motion_blur_num = 0;
                ChangeState(actor, s_StarStuck);
                actor->spr.xrepeat -= 16;
                actor->spr.yrepeat -= 16;
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                actor->spr.clipdist = 16L >> 2;
                actor->user.ceiling_dist = (2);
                actor->user.floor_dist = (2);
                // treat this just like a KillSprite but don't kill
                QueueStar(actor);
                return 0;
            }

            // chance of bouncing
            if (RANDOM_P2(1024) < STAR_BOUNCE_RNUM)
                break;

            wall_ang = NORM_ANGLE(getangle(wph->delta())+512);

            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 36000);
            actor->user.Flags |= (SPR_BOUNCE);
            actor->user.motion_blur_num = 0;
            actor->user.coll.setNone();
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;
            auto hit_sect = actor->user.coll.hitSector;

            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
            {
                if (hit_sect->hasU() && FixedToInt(hit_sect->depth_fixed) > 0)
                {
                    SpawnSplash(actor);
                    KillActor(actor);
                    return true;
                    // hit water - will be taken care of in WeaponMoveHit
                    //break;
                }
            }

            if (actor->user.lowActor)
                if (actor->user.lowActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                    break;
            if (actor->user.highActor)
                if (actor->user.highActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                    break;

            ScaleSpriteVector(actor, 58000);

            vel = ksqrt(SQ(actor->user.change.X) + SQ(actor->user.change.Y));

            if (vel < 500)
                break; // will be killed below - actor != 0

            // 32000 to 96000
            actor->user.change.X = MulScale(actor->user.change.X, 64000 + (RandomRange(64000) - 32000), 16);
            actor->user.change.Y = MulScale(actor->user.change.Y, 64000 + (RandomRange(64000) - 32000), 16);

            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                actor->user.change.Z = MulScale(actor->user.change.Z, 50000, 16); // floor
            else
                actor->user.change.Z = MulScale(actor->user.change.Z, 40000, 16); // ceiling

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // chance of sticking
                    if (RANDOM_P2(1024) < STAR_STICK_RNUM)
                        break;

                    // chance of bouncing
                    if (RANDOM_P2(1024) < STAR_BOUNCE_RNUM)
                        break;

                    actor->user.Flags |= (SPR_BOUNCE);
                    actor->user.motion_blur_num = 0;
                    actor->user.coll.setNone();

                }
                else
                {
                    // hit a sloped sector < 45 degrees
                    actor->user.Flags |= (SPR_BOUNCE);
                    actor->user.motion_blur_num = 0;
                    actor->user.coll.setNone();
                }

                // BREAK HERE - LOOOK !!!!!!!!!!!!!!!!!!!!!!!!
                break; // hit a slope
            }

            actor->user.Flags |= (SPR_BOUNCE);
            actor->user.motion_blur_num = 0;
            actor->user.coll.setNone();
            actor->user.change.Z = -actor->user.change.Z;

            // 32000 to 96000
            actor->user.change.X = MulScale(actor->user.change.X, 64000 + (RandomRange(64000) - 32000), 16);
            actor->user.change.Y = MulScale(actor->user.change.Y, 64000 + (RandomRange(64000) - 32000), 16);
            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                actor->user.change.Z = MulScale(actor->user.change.Z, 50000, 16); // floor
            else
                actor->user.change.Z = MulScale(actor->user.change.Z, 40000, 16); // ceiling

            break;
        }
        }
    }

    if (actor->user.coll.type != kHitNone)
    {
        if (actor->user.coll.type == kHitSprite && actor->user.coll.actor()->hasU())
        {
            auto su = actor->user.coll.actor();
            if (su->user.ID == TRASHCAN || su->user.ID == ZILLA_RUN_R0)
                PlaySound(DIGI_STARCLINK, actor, v3df_none);
        }

        if (actor->user.coll.type != kHitSprite) // Don't clank on sprites
            PlaySound(DIGI_STARCLINK, actor, v3df_none);

        if (WeaponMoveHit(actor))
        {
            KillActor(actor);
            return true;
        }
    }


    return false;

}

int DoCrossBolt(DSWActor* actor)
{
    DoBlurExtend(actor, 0, 2);

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            KillActor(actor);
            return true;
        }
    }

    return false;

}


int DoPlasmaDone(DSWActor* actor)
{
    actor->spr.xrepeat += actor->user.Counter;
    actor->spr.yrepeat -= 4;
    actor->user.Counter += 2;

    if (actor->spr.yrepeat < 6)
    {
        KillActor(actor);
        return 0;
    }

    return 0;
}

DSWActor* PickEnemyTarget(DSWActor* actor, short aware_range)
{
    TARGET_SORT* ts;

    DoPickTarget(actor, aware_range, false);

    for (ts = TargetSort; ts < &TargetSort[TargetSortCount]; ts++)
    {
        if (ts->actor != nullptr)
        {
            if (ts->actor == GetOwner(actor) || GetOwner(ts->actor) == GetOwner(actor))
                continue;

            return ts->actor;
        }
    }

    return nullptr;
}

int MissileSeek(DSWActor* actor, int16_t delay_tics, int16_t aware_range/*, int16_t dang_shift, int16_t turn_limit, int16_t z_limit*/)
{
    int zh;
    short ang2tgt, delta_ang;


    if (actor->user.WaitTics <= delay_tics)
        actor->user.WaitTics += MISSILEMOVETICS;

    if (actor->user.WpnGoalActor == nullptr)
    {
        if (actor->user.WaitTics > delay_tics)
        {
            DSWActor* hitActor;

            if (actor->user.Flags2 & (SPR2_DONT_TARGET_OWNER))
            {
                if ((hitActor = PickEnemyTarget(actor, aware_range)) != nullptr)
                {
                    actor->user.WpnGoalActor = hitActor;
                    hitActor->user.Flags |= (SPR_TARGETED);
                    hitActor->user.Flags |= (SPR_ATTACKED);
                }
            }
            else if ((hitActor = DoPickTarget(actor, aware_range, false)) != nullptr)
            {
                actor->user.WpnGoalActor = hitActor;
                hitActor->user.Flags |= (SPR_TARGETED);
                hitActor->user.Flags |= (SPR_ATTACKED);
            }
        }
    }

    DSWActor* goal = actor->user.WpnGoalActor;
    if (goal != nullptr)
    {
        // move to correct angle
        ang2tgt = getangle(goal->int_pos().X - actor->int_pos().X, goal->int_pos().Y - actor->int_pos().Y);

        delta_ang = getincangle(ang2tgt, actor->int_ang());

        if (labs(delta_ang) > 32)
        {
            if (delta_ang > 0)
                delta_ang = 32;
            else
                delta_ang = -32;
        } 

        actor->add_int_ang(-delta_ang);

        zh = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);

        delta_ang = (zh - actor->int_pos().Z)>>1;

        if (labs(delta_ang) > Z(16))
        {
            if (delta_ang > 0)
                delta_ang = Z(16);
            else
                delta_ang = -Z(16);
        }

        actor->spr.zvel = delta_ang;

        actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
        actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());
        actor->user.change.Z = actor->spr.zvel;
    }
    return 0;
}

// combination of vector manipulation
int ComboMissileSeek(DSWActor* actor, int16_t delay_tics, int16_t aware_range/*, int16_t dang_shift, int16_t turn_limit, int16_t z_limit*/)
{
    int dist;
    int zh;
    short ang2tgt, delta_ang;

    if (actor->user.WaitTics <= delay_tics)
        actor->user.WaitTics += MISSILEMOVETICS;

    if (actor->user.WpnGoalActor == nullptr)
    {
        if (actor->user.WaitTics > delay_tics)
        {
            DSWActor* hitActor;

            if ((hitActor = DoPickTarget(actor, aware_range, false)) != nullptr)
            {
                actor->user.WpnGoalActor = hitActor;
                hitActor->user.Flags |= (SPR_TARGETED);
                hitActor->user.Flags |= (SPR_ATTACKED);
            }
        }
    }

    DSWActor* goal = actor->user.WpnGoalActor;
    if (goal != nullptr)
    {
        int oz;

        // move to correct angle
        ang2tgt = getangle(goal->int_pos().X - actor->int_pos().X, goal->int_pos().Y - actor->int_pos().Y);

        delta_ang = getincangle(ang2tgt, actor->int_ang());

        if (labs(delta_ang) > 32)
        {
            if (delta_ang > 0)
                delta_ang = 32;
            else
                delta_ang = -32;
        }

        actor->add_int_ang(-delta_ang);

        actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
        actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());

        zh = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);

        dist = ksqrt(SQ(actor->int_pos().X - goal->int_pos().X) + SQ(actor->int_pos().Y - goal->int_pos().Y) + (SQ(actor->int_pos().Z - zh)>>8));

        oz = actor->user.change.Z;

        actor->user.change.Z = Scale(actor->spr.xvel, zh - actor->int_pos().Z, dist);
        actor->user.change.Z = (actor->user.change.Z + oz*15)/16;
    }
    return 0;
}

// completely vector manipulation
int VectorMissileSeek(DSWActor* actor, int16_t delay_tics, int16_t turn_speed, int16_t aware_range1, int16_t aware_range2)
{
    int dist;
    int zh;

    if (actor->user.WaitTics <= delay_tics)
        actor->user.WaitTics += MISSILEMOVETICS;

    if (actor->user.WpnGoalActor == nullptr)
    {
        if (actor->user.WaitTics > delay_tics)
        {
            DSWActor* hitActor;

            if (actor->user.Flags2 & (SPR2_DONT_TARGET_OWNER))
            {
                if ((hitActor = PickEnemyTarget(actor, aware_range1)) != nullptr)
                {
                    actor->user.WpnGoalActor = hitActor;
                    hitActor->user.Flags |= (SPR_TARGETED);
                    hitActor->user.Flags |= (SPR_ATTACKED);
                }
                else if ((hitActor = PickEnemyTarget(actor, aware_range2)) != nullptr)
                {
                    actor->user.WpnGoalActor = hitActor;
                    hitActor->user.Flags |= (SPR_TARGETED);
                    hitActor->user.Flags |= (SPR_ATTACKED);
                }
            }
            else
            {
                if ((hitActor = DoPickTarget(actor, aware_range1, false)) != nullptr)
                {
                    actor->user.WpnGoalActor = hitActor;
                    hitActor->user.Flags |= (SPR_TARGETED);
                    hitActor->user.Flags |= (SPR_ATTACKED);
                }
                else if ((hitActor = DoPickTarget(actor, aware_range2, false)) != nullptr)
                {
                    actor->user.WpnGoalActor = hitActor;
                    hitActor->user.Flags |= (SPR_TARGETED);
                    hitActor->user.Flags |= (SPR_ATTACKED);
                }
            }
        }
    }

    DSWActor* goal = actor->user.WpnGoalActor;
    if (goal != nullptr)
    {
        int ox,oy,oz;

        zh = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);

        dist = ksqrt(SQ(actor->int_pos().X - goal->int_pos().X) + SQ(actor->int_pos().Y - goal->int_pos().Y) + (SQ(actor->int_pos().Z - zh)>>8));

        ox = actor->user.change.X;
        oy = actor->user.change.Y;
        oz = actor->user.change.Z;

        actor->user.change.X = Scale(actor->spr.xvel, goal->int_pos().X - actor->int_pos().X, dist);
        actor->user.change.Y = Scale(actor->spr.xvel, goal->int_pos().Y - actor->int_pos().Y, dist);
        actor->user.change.Z = Scale(actor->spr.xvel, zh - actor->int_pos().Z, dist);

        // the large turn_speed is the slower the turn

        actor->user.change.X = (actor->user.change.X + ox*(turn_speed-1))/turn_speed;
        actor->user.change.Y = (actor->user.change.Y + oy*(turn_speed-1))/turn_speed;
        actor->user.change.Z = (actor->user.change.Z + oz*(turn_speed-1))/turn_speed;

        actor->set_int_ang(getangle(actor->user.change.X, actor->user.change.Y));
    }

    return 0;
}

// completely vector manipulation
int VectorWormSeek(DSWActor* actor, int16_t delay_tics, int16_t aware_range1, int16_t aware_range2)
{
    int dist;
    int zh;

    if (actor->user.WaitTics <= delay_tics)
        actor->user.WaitTics += MISSILEMOVETICS;

    if (actor->user.WpnGoalActor == nullptr)
    {
        if (actor->user.WaitTics > delay_tics)
        {
            DSWActor* hitActor;
            if ((hitActor = DoPickTarget(actor, aware_range1, false)) != nullptr)
            {
                actor->user.WpnGoalActor = hitActor;
                hitActor->user.Flags |= (SPR_TARGETED);
                hitActor->user.Flags |= (SPR_ATTACKED);
            }
            else if ((hitActor = DoPickTarget(actor, aware_range2, false)) != nullptr)
            {
                actor->user.WpnGoalActor = hitActor;
                hitActor->user.Flags |= (SPR_TARGETED);
                hitActor->user.Flags |= (SPR_ATTACKED);
            }
        }
    }

    DSWActor* goal = actor->user.WpnGoalActor;
    if (goal != nullptr)
    {
        int ox,oy,oz;

        zh = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);

        dist = ksqrt(SQ(actor->int_pos().X - goal->int_pos().X) + SQ(actor->int_pos().Y - goal->int_pos().Y) + (SQ(actor->int_pos().Z - zh)>>8));

        ox = actor->user.change.X;
        oy = actor->user.change.Y;
        oz = actor->user.change.Z;

        actor->user.change.X = Scale(actor->spr.xvel, goal->int_pos().X - actor->int_pos().X, dist);
        actor->user.change.Y = Scale(actor->spr.xvel, goal->int_pos().Y - actor->int_pos().Y, dist);
        actor->user.change.Z = Scale(actor->spr.xvel, zh - actor->int_pos().Z, dist);

        actor->user.change.X = (actor->user.change.X + ox*7)/8;
        actor->user.change.Y = (actor->user.change.Y + oy*7)/8;
        actor->user.change.Z = (actor->user.change.Z + oz*7)/8;

        actor->set_int_ang(getangle(actor->user.change.X, actor->user.change.Y));
    }

    return 0;
}

int DoBlurExtend(DSWActor* actor, int16_t interval, int16_t blur_num)
{
    if (actor->user.motion_blur_num >= blur_num)
        return 0;

    actor->user.Counter2++;
    if (actor->user.Counter2 > interval)
        actor->user.Counter2 = 0;

    if (!actor->user.Counter2)
    {
        actor->user.motion_blur_num++;
        if (actor->user.motion_blur_num > blur_num)
            actor->user.motion_blur_num = blur_num;
    }

    return 0;
}

int InitPlasmaFountain(DSWActor* wActor, DSWActor* sActor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, PLASMA_FOUNTAIN, s_PlasmaFountain, sActor->sector(),
                            sActor->int_pos().X, sActor->int_pos().Y, ActorZOfBottom(sActor), sActor->int_ang(), 0);

    actorNew->spr.shade = -40;
    if (wActor)
        SetOwner(GetOwner(wActor), actorNew);
    SetAttach(sActor, actorNew);
    actorNew->spr.yrepeat = 0;
    actorNew->spr.clipdist = 8>>2;

    actorNew->user.WaitTics = 120+60;
    actorNew->user.Radius = 50;
    return 0;
}

int DoPlasmaFountain(DSWActor* actor)
{
    // if no Owner then die
    if (actor->user.attachActor == nullptr)
    {
        KillActor(actor);
        return 0;
    }
    else
    {
        DSWActor* attachActor = actor->user.attachActor;
        if (!attachActor) return 0;

        // move with sprite
        SetActorZ(actor, attachActor->int_pos());
        actor->spr.angle = attachActor->spr.angle;

        actor->user.Counter++;
        if (actor->user.Counter > 3)
            actor->user.Counter = 0;

        if (!actor->user.Counter)
        {
            SpawnBlood(attachActor, actor, 0, 0, 0, 0);
            if (RandomRange(1000) > 600)
                InitBloodSpray(attachActor, false, 105);
        }
    }

    // kill the fountain
    if ((actor->user.WaitTics-=MISSILEMOVETICS) <= 0)
    {
        actor->user.WaitTics = 0;

        auto bak_cstat = actor->spr.cstat;
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
        actor->spr.cstat = bak_cstat;

        KillActor(actor);
    }
    return 0; 
}

int DoPlasma(DSWActor* actor)
{
    int32_t dax, day, daz;
	
	auto oldv = actor->spr.pos;
    DoBlurExtend(actor, 0, 4);

    dax = MOVEx(actor->spr.xvel, actor->int_ang());
    day = MOVEy(actor->spr.xvel, actor->int_ang());
    daz = actor->spr.zvel;

    actor->user.coll = move_missile(actor, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);

    {
        // this sprite is supposed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (actor->user.coll.type == kHitSprite)
        {
            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_BLOCK) && !(hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                auto hcstat = hitActor->spr.cstat;

                if (hitActor->hasU() && hitActor != actor->user.WpnGoalActor)
                {
					actor->spr.pos = oldv;

                    hitActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                    actor->user.coll = move_missile(actor, dax, day, daz, Z(16), Z(16), CLIPMASK_MISSILE, MISSILEMOVETICS);
                    hitActor->spr.cstat = hcstat;
                }
            }
        }
    }


    MissileHitDiveArea(actor);
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            if (actor->user.Flags & (SPR_SUICIDE))
            {
                KillActor(actor);
                return true;
            }
            else
            {
                actor->user.Counter = 4;
                ChangeState(actor, s_PlasmaDone);
            }

            return true;
        }
    }

    return false;
}


int DoCoolgFire(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            PlaySound(DIGI_CGMAGICHIT, actor, v3df_follow);
            ChangeState(actor, s_CoolgFireDone);
            auto own = GetOwner(actor);
            if (own && own->hasU() && own->user.ID != RIPPER_RUN_R0)  // JBF: added range check
                SpawnDemonFist(actor); // Just a red magic circle flash
            return true;
        }
    }

    return false;
}

int DoEelFire(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    return false;
}


void ScaleSpriteVector(DSWActor* actor, int scale)
{
    actor->user.change.X = MulScale(actor->user.change.X, scale, 16);
    actor->user.change.Y = MulScale(actor->user.change.Y, scale, 16);
    actor->user.change.Z = MulScale(actor->user.change.Z, scale, 16);
}

void WallBounce(DSWActor* actor, short ang)
{
    int old_ang;
    int k,l;
    int dax, day;

    actor->user.bounce++;

    k = MulScale(bcos(ang), bsin(ang), 13);
    l = bcos(ang << 1);

    dax = -actor->user.change.X;
    day = -actor->user.change.Y;

    actor->user.change.X = DMulScale(day, k, dax, l, 14);
    actor->user.change.Y = DMulScale(dax, k, -day, l, 14);

    old_ang = actor->int_ang();
    actor->set_int_ang(getangle(actor->user.change.X, actor->user.change.Y));

    // hack to prevent missile from sticking to a wall
    //
    if (old_ang == actor->int_ang())
    {
        actor->user.change.X = -actor->user.change.X;
        actor->user.change.Y = -actor->user.change.Y;
        actor->set_int_ang(getangle(actor->user.change.X, actor->user.change.Y));
    }
}


bool SlopeBounce(DSWActor* actor, bool *hit_wall)
{
    int k,l;
    int hiz,loz;
    int slope;
    int dax,day,daz;
    short daang;

    auto hit_sector = actor->user.coll.hitSector;

    getzsofslopeptr(hit_sector, actor->int_pos().X, actor->int_pos().Y, &hiz, &loz);

    // detect the ceiling and the hit_wall
    if (actor->int_pos().Z < ((hiz+loz) >> 1))
    {
        if (!(hit_sector->ceilingstat & CSTAT_SECTOR_SLOPE))
            slope = 0;
        else
            slope = hit_sector->ceilingheinum;
    }
    else
    {
        if (!(hit_sector->floorstat & CSTAT_SECTOR_SLOPE))
            slope = 0;
        else
            slope = hit_sector->floorheinum;
    }

    if (!slope)
        return false;

    // if greater than a 45 degree angle
    if (labs(slope) > 4096)
        *hit_wall = true;
    else
        *hit_wall = false;

    // get angle of the first wall of the sector
    auto wallp = hit_sector->firstWall();
    daang = getangle(wallp->delta());

    // k is now the slope of the ceiling or floor

    // normal vector of the slope
    dax = MulScale(slope, bsin(daang), 14);
    day = MulScale(slope, -bcos(daang), 14);
    daz = 4096; // 4096 = 45 degrees

    // reflection code
    k = ((actor->user.change.X*dax) + (actor->user.change.Y*day)) + MulScale(actor->user.change.Z, daz, 4);
    l = (dax*dax) + (day*day) + (daz*daz);

    // make sure divscale doesn't overflow
    if ((abs(k)>>14) < l)
    {
        k = DivScale(k, l, 17);
        actor->user.change.X -= MulScale(dax, k, 16);
        actor->user.change.Y -= MulScale(day, k, 16);
        actor->user.change.Z -= MulScale(daz, k, 12);

        actor->set_int_ang(getangle(actor->user.change.X, actor->user.change.Y));
    }

    return true;
}

extern STATE s_Phosphorus[];

int DoGrenade(DSWActor* actor)
{
    short i;

    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20;
        actor->user.change.Z += actor->user.Counter;
    }
    else
    {
        actor->user.Counter += 20;
        actor->user.change.Z += actor->user.Counter;
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            PlaySound(DIGI_40MMBNCE, actor, v3df_dontpan);

            auto hitActor = actor->user.coll.actor();

            // special case so grenade can ring gong
            if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (SP_TAG8(hitActor) & BIT(3))
                    DoMatchEverything(nullptr, hitActor->spr.hitag, -1);
            }

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hitActor->int_ang());
                WallBounce(actor, wall_ang);
                ScaleSpriteVector(actor, 32000);
            }
            else
            {
                if (actor->user.Counter2 == 1) // It's a phosphorus grenade!
                {
                    for (i=0; i<5; i++)
                    {
                        actor->set_int_ang(NORM_ANGLE(RandomRange(2048)));
                        InitPhosphorus(actor);
                    }
                }
                SpawnGrenadeExp(actor);
                KillActor(actor);
                return true;
            }


            break;
        }

        case kHitWall:
        {
            short nw,wall_ang;
            walltype* wph;

            wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            PlaySound(DIGI_40MMBNCE, actor, v3df_dontpan);

            wall_ang = NORM_ANGLE(getangle(wph->delta())+512);

            //actor->spr.angle = NORM_ANGLE(actor->spr.angle + 1);
            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 22000);

            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;
            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(actor, 22000); // 28000
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        if (!(actor->user.Flags & SPR_BOUNCE))
                        {
                            actor->user.Flags |= (SPR_BOUNCE);
                            ScaleSpriteVector(actor, 40000); // 18000
                            actor->user.coll.setNone();
                            actor->user.change.Z /= 4;
                            actor->user.Counter = 0;
                        }
                        else
                        {
                            if (actor->user.Counter2 == 1) // It's a phosphorus grenade!
                            {
                                for (i=0; i<5; i++)
                                {
                                    actor->set_int_ang(NORM_ANGLE(RandomRange(2048)));
                                    InitPhosphorus(actor);
                                }
                            }
                            SpawnGrenadeExp(actor);
                            KillActor(actor);
                            return true;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(actor, 22000);
                    }
                }
            }
            else
            {
                // hit floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    if (actor->user.Flags & (SPR_UNDERWATER))
                        actor->user.Flags |= (SPR_BOUNCE); // no bouncing underwater

                    if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                        actor->user.Flags |= (SPR_BOUNCE); // no bouncing on shallow water

                    if (!(actor->user.Flags & SPR_BOUNCE))
                    {
                        actor->user.Flags |= (SPR_BOUNCE);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        actor->user.change.Z = -actor->user.change.Z;
                        ScaleSpriteVector(actor, 40000); // 18000
                        actor->user.change.Z /= 4;
                        PlaySound(DIGI_40MMBNCE, actor, v3df_dontpan);
                    }
                    else
                    {
                        if (actor->user.Counter2 == 1) // It's a phosphorus grenade!
                        {
                            for (i=0; i<5; i++)
                            {
                                actor->set_int_ang(NORM_ANGLE(RandomRange(2048)));
                                InitPhosphorus(actor);
                            }
                        }
                        SpawnGrenadeExp(actor);
                        KillActor(actor);
                        return true;
                    }
                }
                else
                // hit something above
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 22000);
                    PlaySound(DIGI_40MMBNCE, actor, v3df_dontpan);
                }
            }
            break;
        }
        }
    }

    if (actor->user.bounce > 10)
    {
        SpawnGrenadeExp(actor);
        KillActor(actor);
        return true;
    }

    // if you haven't bounced or your going slow do some puffs
    if (!(actor->user.Flags & (SPR_BOUNCE|SPR_UNDERWATER)))
    {
        auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_Puff, actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 100);

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 40;
        actorNew->spr.yrepeat = 40;
        actorNew->opos = actor->opos;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        actorNew->user.change.X = actor->user.change.X;
        actorNew->user.change.Y = actor->user.change.Y;
        actorNew->user.change.Z = actor->user.change.Z;

        ScaleSpriteVector(actorNew, 22000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return false;
}

int DoVulcanBoulder(DSWActor* actor)
{
    actor->user.Counter += 40;
    actor->user.change.Z += actor->user.Counter;

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    int32_t const vel = ksqrt(SQ(actor->user.change.X) + SQ(actor->user.change.Y));

    if (vel < 30)
    {
        SpawnLittleExp(actor);
        KillActor(actor);
        return true;
    }

    if (actor->user.coll.type != kHitNone)
    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hitActor->int_ang());
                WallBounce(actor, wall_ang);
                ScaleSpriteVector(actor, 40000);
            }
            else
            {
                // hit an actor
                SpawnLittleExp(actor);
                KillActor(actor);
                return true;
            }
            break;
        }

        case kHitWall:
        {
            short nw,wall_ang;
            walltype* wph;

            wph = actor->user.coll.hitWall;

            if (wph->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(wph, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            wall_ang = NORM_ANGLE(getangle(wph->delta())+512);

            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 40000);
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a sloped sector - treated as a wall because of large slope
                    ScaleSpriteVector(actor, 30000);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sloped sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        actor->user.change.X = MulScale(actor->user.change.X, 30000, 16);
                        actor->user.change.Y = MulScale(actor->user.change.Y, 30000, 16);
                        actor->user.change.Z = MulScale(actor->user.change.Z, 12000, 16);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;

                        // limit to a reasonable bounce value
                        if (actor->user.change.Z > Z(32))
                            actor->user.change.Z = Z(32);
                    }
                    else
                    {
                        // hit a sloped ceiling
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        ScaleSpriteVector(actor, 30000);
                    }
                }
            }
            else
            {
                // hit unsloped floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;

                    actor->user.change.X = MulScale(actor->user.change.X, 20000, 16);
                    actor->user.change.Y = MulScale(actor->user.change.Y, 20000, 16);
                    actor->user.change.Z = MulScale(actor->user.change.Z, 32000, 16);

                    // limit to a reasonable bounce value
                    if (actor->user.change.Z > Z(24))
                        actor->user.change.Z = Z(24);

                    actor->user.change.Z = -actor->user.change.Z;

                }
                else
                // hit unsloped ceiling
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 30000);
                }
            }

            break;
        }
        }
    }

    return false;
}

bool OwnerIsPlayer(DSWActor* actor)
{
    auto own = GetOwner(actor);
    return (own && own->hasU() && own->user.PlayerP != nullptr);
}

int DoMineRangeTest(DSWActor* actor, int range)
{
    unsigned stat;
    int dist, tx, ty;
    int tmin;
    bool ownerisplayer = false;

    ownerisplayer = OwnerIsPlayer(actor);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);
            if (dist > range)
                continue;

            if (actor == itActor)
                continue;

            if (!(itActor->spr.cstat & CSTAT_SPRITE_BLOCK))
                continue;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            if (itActor->user.ID == GIRLNINJA_RUN_R0 && !ownerisplayer)
                continue;

            dist = FindDistance3D(actor->int_pos() - itActor->int_pos());
            if (dist > range)
                continue;

            if (!FAFcansee(itActor->int_pos().X,itActor->int_pos().Y,ActorUpperZ(actor),itActor->sector(),actor->int_pos().X,actor->int_pos().Y,actor->int_pos().Z,actor->sector()))
                continue;

            return true;
        }
    }

    return false;
}


int DoMineStuck(DSWActor* actor)
{
    constexpr int MINE_DETONATE_STATE = 99;

    // if no Owner then die
    DSWActor* attachActor = actor->user.attachActor;
    if (attachActor != nullptr)
    {
        ASSERT(attachActor->hasU());

        // Is it attached to a dead actor? Blow it up if so.
        if ((attachActor->user.Flags & SPR_DEAD) && actor->user.Counter2 < MINE_DETONATE_STATE)
        {
            actor->user.Counter2 = MINE_DETONATE_STATE;
            actor->user.WaitTics = SEC(1)/2;
        }

        vec3_t pos = { attachActor->int_pos().X, attachActor->int_pos().Y, attachActor->int_pos().Z - actor->user.pos.Z };
        SetActorZ(actor, &pos);
        actor->set_int_z(attachActor->int_pos().Z - (ActorSizeZ(attachActor) >> 1));
    }

    // not activated yet
    if (!(actor->user.Flags & SPR_ACTIVE))
    {
        if ((actor->user.WaitTics -= (MISSILEMOVETICS*2)) > 0)
            return false;

        // activate it
        //actor->user.WaitTics = 65536;
        actor->user.WaitTics = 32767;
        actor->user.Counter2 = 0;
        actor->user.Flags |= (SPR_ACTIVE);
    }

    // limit the number of times DoMineRangeTest is called
    actor->user.Counter++;
    if (actor->user.Counter > 1)
        actor->user.Counter = 0;

    if (actor->user.Counter2 != MINE_DETONATE_STATE)
    {
        if ((actor->user.Counter2++) > 30)
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.WaitTics = 32767;  // Keep reseting tics to make it stay forever
            actor->user.Counter2 = 0;
        }
    }

    if (!actor->user.Counter)
    {
        // not already in detonate state
        if (actor->user.Counter2 < MINE_DETONATE_STATE)
        {
            // if something came into range - detonate
            if (DoMineRangeTest(actor, 3000))
            {
                // move directly to detonate state
                actor->user.Counter2 = MINE_DETONATE_STATE;
                actor->user.WaitTics = SEC(1)/2;
            }
        }
    }

    actor->user.WaitTics -= (MISSILEMOVETICS * 2);


    // start beeping with pauses
    // quick and dirty beep countdown code
    switch (actor->user.Counter2)
    {
#if 0
    case 0:
        if (actor->user.WaitTics < SEC(45))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 1:
        if (actor->user.WaitTics < SEC(38))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 2:
        if (actor->user.WaitTics < SEC(30))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 3:
        if (actor->user.WaitTics < SEC(20))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 4:
        if (actor->user.WaitTics < SEC(15))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 5:
        if (actor->user.WaitTics < SEC(12))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 6:
        if (actor->user.WaitTics < SEC(10))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
    case 7:
        if (actor->user.WaitTics < SEC(8))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2++;
        }
        break;
#endif
    case 30:
        if (actor->user.WaitTics < SEC(6))
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            actor->user.Counter2 = MINE_DETONATE_STATE;
        }
        break;
    case MINE_DETONATE_STATE:
        if (actor->user.WaitTics < 0)
        {
            PlaySound(DIGI_MINEBEEP, actor, v3df_dontpan);
            SpawnMineExp(actor);
            KillActor(actor);
            return false;
        }
        break;
    }

    return false;
}

void SetMineStuck(DSWActor* actor)
{
    // stuck
    actor->user.Flags |= (SPR_BOUNCE);
    // not yet active for 1 sec
    actor->user.Flags &= ~(SPR_ACTIVE);
    actor->user.WaitTics = SEC(3);
    //actor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->user.Counter = 0;
    change_actor_stat(actor, STAT_MINE_STUCK);
    ChangeState(actor, s_MineStuck);
}

int DoMine(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        // decrease velocity
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20;
        actor->user.change.Z += actor->user.Counter;
    }
    else
    {
        //actor->user.Counter += 75;
        actor->user.Counter += 40;
        actor->user.change.Z += actor->user.Counter;
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        // check to see if you hit a sprite
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return 0;
        case kHitSprite:
        {
            auto hitActor = actor->user.coll.actor();

            SetMineStuck(actor);
            // Set the Z position
            actor->set_int_z(hitActor->int_pos().Z - (ActorSizeZ(hitActor) >> 1));

            // If it's not alive, don't stick it
            if (hitActor->hasU() && hitActor->user.Health <= 0) return false;    // JBF: added null check

            // check to see if sprite is player or enemy
            if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
            {
                PLAYER* pp;

                // attach weapon to sprite
                SetAttach(hitActor, actor);
                actor->user.pos.Z = hitActor->int_pos().Z - actor->int_pos().Z;

                auto own = GetOwner(actor);
                if (own && own->hasU())
                {
                    if (own->user.PlayerP)
                    {
                        pp = own->user.PlayerP;

                        if (RandomRange(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU1, v3df_follow|v3df_dontpan,pp);
                        else if (RandomRange(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU2, v3df_follow|v3df_dontpan,pp);
                        else if (RandomRange(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU3, v3df_follow|v3df_dontpan,pp);
                        else if (RandomRange(1000) > 800)
                            PlayerSound(DIGI_STICKYGOTU4, v3df_follow|v3df_dontpan,pp);
                    }
                }
            }
            else
            {
                if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
                {
                    actor->user.Flags2 |= (SPR2_ATTACH_WALL);
                }
                else if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
                {
                    // hit floor
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                        actor->user.Flags2 |= (SPR2_ATTACH_FLOOR);
                    else
                        actor->user.Flags2 |= (SPR2_ATTACH_CEILING);
                }
                else
                {
                    SpawnMineExp(actor);
                    KillActor(actor);
                    return false;
                }
            }

            break;
        }

        case kHitWall:
        {
            auto hit_wall = actor->user.coll.hitWall;

            if (hit_wall->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(hit_wall, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->user.ID);
                actor->user.coll.setNone();
                break;
            }

            SetMineStuck(actor);

            actor->user.Flags2 |= (SPR2_ATTACH_WALL);

            if ((hit_wall->extra & WALLFX_SECTOR_OBJECT))
            {
            }

            if ((hit_wall->extra & WALLFX_DONT_STICK))
            {
                SpawnMineExp(actor);
                KillActor(actor);
                return false;
            }

            break;
        }

        case kHitSector:
        {
            auto hit_sect = actor->user.coll.hitSector;

            SetMineStuck(actor);

            // hit floor
            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                actor->user.Flags2 |= (SPR2_ATTACH_FLOOR);
            else
                actor->user.Flags2 |= (SPR2_ATTACH_CEILING);


            if ((hit_sect->extra & SECTFX_SECTOR_OBJECT))
            {
                SpawnMineExp(actor);
                KillActor(actor);
                return false;
            }

            break;
        }
        }

        actor->user.coll.setNone();
    }

    return false;
}

int DoPuff(DSWActor* actor)
{
    actor->add_int_pos({ actor->user.change.X, actor->user.change.Y, actor->user.change.Z });
    return 0;
}

int DoRailPuff(DSWActor* actor)
{
    actor->spr.xrepeat += 4;
    actor->spr.yrepeat += 4;

    return 0;
}

int DoBoltThinMan(DSWActor* actor)
{
    int32_t dax, day, daz;

    DoBlurExtend(actor, 0, 4);

    dax = MOVEx(actor->spr.xvel, actor->int_ang());
    day = MOVEy(actor->spr.xvel, actor->int_ang());
    daz = actor->spr.zvel;

    actor->user.coll = move_missile(actor, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.Flags & (SPR_SUICIDE))
        return true;

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnBoltExp(actor);
            KillActor(actor);
            return true;
        }
    }


    return false;
}

int DoTracer(DSWActor* actor)
{
    for (int i = 0; i < 4; i++)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor))
            {
                KillActor(actor);
                return true;
            }
        }
    }

    actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);

    return false;
}

int DoEMP(DSWActor* actor)
{
    for (int i = 0; i < 4; i++)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (RandomRange(1000) > 500)
        {
            actor->spr.xrepeat = 52;
            actor->spr.yrepeat = 10;
        }
        else
        {
            actor->spr.xrepeat = 8;
            actor->spr.yrepeat = 38;
        }

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor))
            {
                KillActor(actor);
                return true;
            }
        }
    }

    actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);

    return false;
}

int DoEMPBurst(DSWActor* actor)
{
    DSWActor* attachActor = actor->user.attachActor;
    if (attachActor != nullptr)
    {
        vec3_t pos = { attachActor->int_pos().X, attachActor->int_pos().Y, attachActor->int_pos().Z - actor->user.pos.Z };
        SetActorZ(actor, &pos);
        actor->set_int_ang(NORM_ANGLE(attachActor->int_ang() + 1024));
    }

    // not activated yet
    if (!(actor->user.Flags & SPR_ACTIVE))
    {
        // activate it
        actor->user.WaitTics = SEC(7);
        actor->user.Flags |= (SPR_ACTIVE);
    }

    if (RandomRange(1000) > 500)
    {
        actor->spr.xrepeat = 52;
        actor->spr.yrepeat = 10;
    }
    else
    {
        actor->spr.xrepeat = 8;
        actor->spr.yrepeat = 38;
    }

    if ((RANDOM_P2(1024<<6)>>6) < 700)
    {
        SpawnShrapX(actor);
    }

    actor->user.WaitTics -= (MISSILEMOVETICS * 2);

    if (actor->user.WaitTics < 0)
    {
        // Spawn a big radius burst of sparks here and check for final damage amount
        KillActor(actor);
        return false;
    }

    return false;
}

int DoTankShell(DSWActor* actor)
{
    short i;

    for (i = 0; i < 4; i++)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor))
            {
                SpawnTankShellExp(actor);
                KillActor(actor);
                return true;
            }
        }
    }

    return false;
}

int DoTracerStart(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            KillActor(actor);
            return true;
        }
    }

    return false;
}

int DoLaser(DSWActor* actor)
{
    short spawn_count = 0;

    if (SW_SHAREWARE) return false; // JBF: verify

    while (true)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor))
            {
                SpawnBoltExp(actor);
                KillActor(actor);
                return true;
            }
        }

        spawn_count++;
        if (spawn_count < 256)
        {
            auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_LaserPuff, actor->sector(),
                              actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

            actorNew->spr.shade = -40;
            actorNew->spr.xrepeat = 16;
            actorNew->spr.yrepeat = 16;
            actorNew->spr.pal = actorNew->user.spal = PALETTE_RED_LIGHTING;

            actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
            actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

            actorNew->user.change.X = actorNew->user.change.Y = actorNew->user.change.Z = 0;
        }
    }
}

int DoLaserStart(DSWActor* actor)
{
    if (SW_SHAREWARE) return false; // JBF: verify

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnBoltExp(actor);
            KillActor(actor);
            return true;
        }
    }

    return 0;
}

int DoRail(DSWActor* actor)
{
    short spawn_count = 0;

    if (SW_SHAREWARE) return false; // JBF: verify

    while (true)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor) && actor->user.coll.type != kHitNone) // beware of side effects of WeaponMoveHit!
            {
                if (actor->user.coll.type == kHitSprite)
                {
                    auto hitActor = actor->user.coll.actor();

                    if (hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY)
                    {
                        auto cstat_save = hitActor->spr.cstat;

                        hitActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);
                        DoRail(actor);
                        hitActor->spr.cstat = cstat_save;
                        return true;
                    }
                    else
                    {
                        SpawnTracerExp(actor);
                        SpawnShrapX(actor);
                        KillActor(actor);
                        return true;
                    }
                }
                else
                {
                    SpawnTracerExp(actor);
                    SpawnShrapX(actor);
                    KillActor(actor);
                    return true;
                }
            }
        }

        spawn_count++;
        if (spawn_count < 128)
        {
            auto actorNew = SpawnActor(STAT_MISSILE, PUFF, &s_RailPuff[0][0], actor->sector(),
                              actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 20);

            actorNew->spr.xvel += (RandomRange(140)-RandomRange(140));
            actorNew->spr.yvel += (RandomRange(140)-RandomRange(140));
            actorNew->spr.zvel += (RandomRange(140)-RandomRange(140));

            actorNew->user.RotNum = 5;
            NewStateGroup(actorNew, sg_RailPuff);

            actorNew->spr.shade = -40;
            actorNew->spr.xrepeat = 10;
            actorNew->spr.yrepeat = 10;
            actorNew->opos = actor->opos;
            actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
            actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

            actorNew->user.change.X = actor->user.change.X;
            actorNew->user.change.Y = actor->user.change.Y;
            actorNew->user.change.Z = actor->user.change.Z;

            ScaleSpriteVector(actorNew, 1500);

            if (actor->user.Flags & (SPR_UNDERWATER))
                actorNew->user.Flags |= (SPR_UNDERWATER);
        }
    }
}

int DoRailStart(DSWActor* actor)
{
    if (SW_SHAREWARE) return false; // JBF: verify

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnTracerExp(actor);
            SpawnShrapX(actor);
            KillActor(actor);
            return true;
        }
    }
    return 0;
}

int DoRocket(DSWActor* actor)
{
    int dist,a,b,c;
    auto pos = actor->int_pos();


    if ((actor->user.FlagOwner -= ACTORMOVETICS)<=0 && actor->user.spal == 20)
    {
        DISTANCE(actor->int_pos().X, actor->int_pos().Y, actor->user.targetActor->int_pos().X, actor->user.targetActor->int_pos().Y, dist, a, b, c);
        actor->user.FlagOwner = dist>>6;
        // Special warn sound attached to each seeker spawned
        PlaySound(DIGI_MINEBEEP, actor, v3df_follow);
    }

    if (actor->user.Flags & (SPR_FIND_PLAYER))
    {
        VectorMissileSeek(actor, 30, 16, 128, 768);
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.Flags & (SPR_SUICIDE))
        return true;

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor) && actor->user.coll.type != kHitNone)
        {
            if (actor->user.ID == BOLT_THINMAN_R4)
            {
                SpawnBunnyExp(actor);
            }
            else if (actor->user.Radius == NUKE_RADIUS)
                SpawnNuclearExp(actor);
            else
                SpawnBoltExp(actor);

            KillActor(actor);
            return true;
        }
    }

    if (!actor->user.Counter)
    {
        auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_Puff, actor->sector(),
                          pos.X, pos.Y, pos.Z, actor->int_ang(), 100);

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 40;
        actorNew->spr.yrepeat = 40;
        actorNew->opos = actor->opos;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        actorNew->user.change.X = actor->user.change.X;
        actorNew->user.change.Y = actor->user.change.Y;
        actorNew->user.change.Z = actor->user.change.Z;

        ScaleSpriteVector(actorNew, 20000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }
    return false;
}

int DoMicroMini(DSWActor* actor)
{
    short i;

    for (i = 0; i < 3; i++)
    {
        actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

        MissileHitDiveArea(actor);

        if (actor->user.coll.type != kHitNone)
        {
            if (WeaponMoveHit(actor))
            {
                SpawnMicroExp(actor);
                KillActor(actor);
                return true;
            }
        }
    }

    return false;
}

int SpawnExtraMicroMini(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], actor->sector(),
                    actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), actor->spr.xvel);

    SetOwner(GetOwner(actor), actorNew);
    actorNew->spr.yrepeat = actorNew->spr.xrepeat = actor->spr.xrepeat;
    actorNew->spr.shade = actor->spr.shade;
    actorNew->spr.clipdist = actor->spr.clipdist;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_MicroMini[0]);
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = actor->user.Radius;
    actorNew->user.ceiling_dist = actor->user.ceiling_dist;
    actorNew->user.floor_dist = actor->user.floor_dist;
    actorNew->spr.cstat = actor->spr.cstat;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + RandomRange(64) - 32));
    actorNew->spr.zvel = actor->spr.zvel;
    actorNew->spr.zvel += RandomRange(Z(16)) - Z(8);

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;
    return 0;
}

int DoMicro(DSWActor* actor)
{
    if (SW_SHAREWARE) return false; // JBF: verify

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (!actor->user.Counter)
    {
        auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_Puff, actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 100);

        SetOwner(GetOwner(actor), actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 20;
        actorNew->spr.yrepeat = 20;
        actorNew->opos = actor->opos;
        actorNew->spr.zvel = actor->spr.zvel;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

        actorNew->user.change.X = actor->user.change.X;
        actorNew->user.change.Y = actor->user.change.Y;
        actorNew->user.change.Z = actor->user.change.Z;

        ScaleSpriteVector(actorNew, 20000);

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);

        // last smoke
        if ((actor->user.WaitTics -= MISSILEMOVETICS) <= 0)
        {
            SetActorZ(actorNew, actorNew->int_pos());
            NewStateGroup(actor, &sg_MicroMini[0]);
            actor->spr.xrepeat = actor->spr.yrepeat = 10;
            actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
            SpawnExtraMicroMini(actor);
            return true;
        }
    }


    // hit something
    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnMicroExp(actor);
            KillActor(actor);
            return true;
        }
    }

    return false;
}

int DoUziBullet(DSWActor* actor)
{
    int32_t dax, day, daz;
    int sx,sy;
    short i;

    // call move_sprite twice for each movement
    // otherwize the moves are in too big an increment
    for (i = 0; i < 2; i++)
    {
        dax = MOVEx((actor->spr.xvel >> 1), actor->int_ang());
        day = MOVEy((actor->spr.xvel >> 1), actor->int_ang());
        daz = actor->spr.zvel >> 1;

        sx = actor->int_pos().X;
        sy = actor->int_pos().Y;
        actor->user.coll = move_missile(actor, dax, day, daz, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);
        actor->user.Dist += Distance(sx, sy, actor->int_pos().X, actor->int_pos().Y);

        MissileHitDiveArea(actor);

        if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 128)
            SpawnBubble(actor);

        if (actor->user.coll.type != kHitNone)
        {
            WeaponMoveHit(actor);

            auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
            actorNew->spr.shade = -40;
            actorNew->spr.xrepeat = UZI_SMOKE_REPEAT;
            actorNew->spr.yrepeat = UZI_SMOKE_REPEAT;
            SetOwner(GetOwner(actor), actorNew);
            actorNew->spr.angle = actor->spr.angle;
            actorNew->spr.clipdist = 128 >> 2;
            actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

            if (!(actor->user.Flags & SPR_UNDERWATER))
            {
                actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, actorNew->sector(), actorNew->int_pos().X, actorNew->int_pos().Y, actorNew->int_pos().Z, 0, 0);
                actorNew->spr.shade = -40;
                actorNew->spr.xrepeat = UZI_SPARK_REPEAT;
                actorNew->spr.yrepeat = UZI_SPARK_REPEAT;
                SetOwner(GetOwner(actor), actorNew);
                actorNew->spr.angle = actor->spr.angle;
                actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
            }

            KillActor(actor);
            return true;
        }
        else if (actor->user.Dist > 8000)
        {
            KillActor(actor);
            return 0;
        }
    }
    return false;
}


int DoBoltSeeker(DSWActor* actor)
{
    int32_t dax, day, daz;

    MissileSeek(actor, 30, 768/*, 4, 48, 6*/);
    DoBlurExtend(actor, 0, 4);

    dax = MOVEx(actor->spr.xvel, actor->int_ang());
    day = MOVEy(actor->spr.xvel, actor->int_ang());
    daz = actor->spr.zvel;

    actor->user.coll = move_missile(actor, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnBoltExp(actor);
            KillActor(actor);
            return true;
        }
    }
    return false;
}

int DoBoltShrapnel(DSWActor* actor)
{
    return 0;
}

int DoBoltFatMan(DSWActor* actor)
{
    return 0;
}

int DoElectro(DSWActor* actor)
{
    int32_t dax, day, daz;

    DoBlurExtend(actor, 0, 4);

    // only seek on Electro's after a hit on an actor
    if (actor->user.Counter > 0)
        MissileSeek(actor, 30, 512/*, 3, 52, 2*/);

    dax = MOVEx(actor->spr.xvel, actor->int_ang());
    day = MOVEy(actor->spr.xvel, actor->int_ang());
    daz = actor->spr.zvel;

    actor->user.coll = move_missile(actor, dax, day, daz, CEILING_DIST, FLOOR_DIST, CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.Flags & (SPR_SUICIDE))
        return true;

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            switch (actor->user.coll.type)
            {
            case kHitSprite:
            {
                auto hitActor = actor->user.coll.actor();

                if (!(hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY) || hitActor->user.ID == SKULL_R0 || hitActor->user.ID == BETTY_R0)
                    SpawnShrap(actor, nullptr);
                break;
            }

            default:
                SpawnShrap(actor, nullptr);
                break;
            }

            KillActor(actor);
            return true;
        }
    }
    return false;
}

int DoLavaBoulder(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);
    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.Flags & (SPR_SUICIDE))
        return true;

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnShrap(actor, nullptr);
            KillActor(actor);
            return true;
        }
    }

    return false;
}

int DoSpear(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.Flags & (SPR_SUICIDE))
        return true;

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            KillActor(actor);
            return true;
        }
    }

    return false;
}

int SpawnCoolieExp(DSWActor* actor)
{
    int zh,nx,ny;

    ASSERT(actor->hasU());

    actor->user.Counter = RandomRange(120);  // This is the wait til birth time!

    zh = actor->int_pos().Z - ActorSizeZ(actor) + (ActorSizeZ(actor) >> 2);
    nx = actor->int_pos().X + MOVEx(64, actor->int_ang() + 1024);
    ny = actor->int_pos().Y + MOVEy(64, actor->int_ang() + 1024);

    PlaySound(DIGI_COOLIEEXPLODE, actor, v3df_none);

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_EXP, s_BoltExp, actor->sector(),
                            nx, ny, zh, actor->int_ang(), 0);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(actor, actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.Radius = DamageData[DMG_BOLT_EXP].radius;
    DoExpDamageTest(actorNew);
    return 0;
}

void SpawnFireballFlames(DSWActor* actor, DSWActor* enemyActor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
        return;

    if (enemyActor != nullptr)
    {
        // test for already burned
        if ((enemyActor->spr.extra & SPRX_BURNABLE) && enemyActor->spr.shade > 40)
            return;

        if (!enemyActor->hasU())
        {
            ASSERT(true == false);
            return;
        }

        auto flameActor = enemyActor->user.flameActor;
        if (flameActor != nullptr)
        {
            int sizez = ActorSizeZ(enemyActor) + (ActorSizeZ(enemyActor) >> 2);

            if ((enemyActor->spr.extra & SPRX_BURNABLE))
                return;

            if (flameActor->user.Counter >= GetRepeatFromHeight(flameActor, sizez))
            {
                // keep flame only slightly bigger than the enemy itself
                flameActor->user.Counter = GetRepeatFromHeight(flameActor, sizez);
            }
            else
            {
                //increase max size
                flameActor->user.Counter += GetRepeatFromHeight(flameActor, 8<<8);
            }

            // Counter is max size
            if (flameActor->user.Counter >= 230)
            {
                // this is far too big
                flameActor->user.Counter = 230;
            }

            if (flameActor->user.WaitTics < 2*120)
                flameActor->user.WaitTics = 2*120;  // allow it to grow again

            return;
        }
    }

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL_FLAMES, s_FireballFlames, actor->sector(),
                      actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    if (enemyActor != nullptr)
        enemyActor->user.flameActor = actorNew;

    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 16;
    if (enemyActor != nullptr)
    {
        // large flame for trees and such
        if ((enemyActor->spr.extra & SPRX_BURNABLE))
        {
            int sizez = ActorSizeZ(enemyActor) + (ActorSizeZ(enemyActor) >> 2);
            actorNew->user.Counter = GetRepeatFromHeight(actorNew, sizez);
        }
        else
        {
            actorNew->user.Counter = GetRepeatFromHeight(actorNew, ActorSizeZ(enemyActor)>>1);
        }
    }
    else
    {
        actorNew->user.Counter = 48; // max flame size
    }

    SetOwner(GetOwner(actor), actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //actorNew->user.Radius = DamageData[DMG_FIREBALL_FLAMES].radius;
    actorNew->user.Radius = 200;

    if (enemyActor != nullptr)
    {
        SetAttach(enemyActor, actorNew);
    }
    else
    {
        if (TestDontStickSector(actorNew->sector()))
        {
            KillActor(actorNew);
            return;
        }

        actorNew->user.floor_dist = actorNew->user.ceiling_dist = 0;

        DoFindGround(actorNew);
        actorNew->user.jump_speed = 0;
        DoBeginJump(actorNew);
    }

    PlaySound(DIGI_FIRE1,actorNew,v3df_dontpan|v3df_doppler);
}


int SpawnBreakFlames(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL_FLAMES+1, s_BreakFlames, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 16;
    actorNew->user.Counter = 48; // max flame size

    actorNew->spr.shade = -40;
    if (actor->hasU())
        actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.Radius = 200;

    actorNew->user.floor_dist = actorNew->user.ceiling_dist = 0;

    DoFindGround(actorNew);
    actorNew->user.jump_speed = 0;
    DoBeginJump(actorNew);

    PlaySound(DIGI_FIRE1,actorNew,v3df_dontpan|v3df_doppler);

    return 0;
}


void SpawnBreakStaticFlames(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_STATIC_FIRE, FIREBALL_FLAMES, nullptr, actor->sector(),
                      actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    if (RandomRange(1000) > 500)
        actorNew->spr.picnum = 3143;
    else
        actorNew->spr.picnum = 3157;

    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    actorNew->spr.xrepeat = 32;
    actorNew->spr.yrepeat = 32;

    actorNew->spr.shade = -40;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.Radius = 200;
    actorNew->user.floor_dist = actorNew->user.ceiling_dist = 0;
    actorNew->set_int_z(getflorzofslopeptr(actorNew->sector(), actorNew->int_pos().X, actorNew->int_pos().Y));

    PlaySound(DIGI_FIRE1,actorNew,v3df_dontpan|v3df_doppler);
}


void SpawnFireballExp(DSWActor* actor)
{
    ASSERT(actor->hasU());

    if (actor->user.Flags & (SPR_SUICIDE))
        return;

    PlaySound(DIGI_SMALLEXP, actor, v3df_none);

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL_EXP, s_FireballExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    actorNew->spr.xrepeat = 52;
    actorNew->spr.yrepeat = 52;
    SetOwner(GetOwner(actor), actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->user.Flags |= (actor->user.Flags & (SPR_UNDERWATER));
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(actor, actorNew, Z(15), Z(15));

    if (RANDOM_P2(1024) < 150)
        SpawnFireballFlames(actorNew, nullptr);
}

void SpawnGoroFireballExp(DSWActor* actor)
{
    ASSERT(actor->hasU());

    if (actor->user.Flags & (SPR_SUICIDE))
        return;

    PlaySound(DIGI_MEDIUMEXP, actor, v3df_none);

    auto actorNew = SpawnActor(STAT_MISSILE, 0, s_FireballExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 16;
    SetOwner(GetOwner(actor), actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.pal = actorNew->user.spal = actor->user.spal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(actor, actorNew, Z(15), Z(15));
}

void SpawnBoltExp(DSWActor* actor)
{
    ASSERT(actor->hasU());

    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return;

    PlaySound(DIGI_BOLTEXPLODE, actor, v3df_none);

    auto expActor = SpawnActor(STAT_MISSILE, BOLT_EXP, s_BoltExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 76;
    expActor->spr.yrepeat = 76;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    expActor->user.Radius = DamageData[DMG_BOLT_EXP].radius;

    SpawnExpZadjust(actor, expActor, Z(40), Z(40));

    DoExpDamageTest(expActor);

    SetExpQuake(actor); // !JIM! made rocket launcher shake things
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);
}

int SpawnBunnyExp(DSWActor* actor)
{
    ASSERT(actor->hasU());

    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return -1;

    PlaySound(DIGI_BUNNYDIE3, actor, v3df_none);

    actor->user.ID = BOLT_EXP; // Change id
    InitBloodSpray(actor, true, -1);
    InitBloodSpray(actor, true, -1);
    InitBloodSpray(actor, true, -1);
    DoExpDamageTest(actor);

    return 0;
}

void SpawnTankShellExp(DSWActor* actor)
{
    ASSERT(actor->hasU());

    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return;

    PlaySound(DIGI_BOLTEXPLODE, actor, v3df_none);

    auto expActor = SpawnActor(STAT_MISSILE, TANK_SHELL_EXP, s_TankShellExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 64+32;
    expActor->spr.yrepeat = 64+32;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    expActor->user.Radius = DamageData[DMG_TANK_SHELL_EXP].radius;

    SpawnExpZadjust(actor, expActor, Z(40), Z(40));
    DoExpDamageTest(expActor);
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);
}


void SpawnNuclearSecondaryExp(DSWActor* actor, short ang)
{
    ASSERT(actor->hasU());

    auto expActor = SpawnActor(STAT_MISSILE, GRENADE_EXP, s_GrenadeExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 512);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -128;
    expActor->spr.xrepeat = 218;
    expActor->spr.yrepeat = 152;
    expActor->spr.clipdist = actor->spr.clipdist;
    expActor->user.ceiling_dist = (16);
    expActor->user.floor_dist = (16);
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //ang = RANDOM_P2(2048);
    int32_t const vel = (2048+128) + RandomRange(2048);
    expActor->user.change.X = MOVEx(vel, ang);
    expActor->user.change.Y = MOVEy(vel, ang);
    expActor->user.Radius = 200; // was NUKE_RADIUS
    expActor->user.coll = move_missile(expActor, expActor->user.change.X, expActor->user.change.Y, 0,
                           expActor->user.int_ceiling_dist(),expActor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (FindDistance3D(expActor->int_pos() - actor->int_pos()) < 1024)
    {
        KillActor(expActor);
        return;
    }

    SpawnExpZadjust(actor, expActor, Z(50), Z(10));
    InitChemBomb(expActor);
}

void SpawnNuclearExp(DSWActor* actor)
{
    short ang=0;
    PLAYER* pp = nullptr;
    short rnd_rng;

    ASSERT(actor->hasU());
    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return;

    PlaySound(DIGI_NUCLEAREXP, actor, v3df_dontpan | v3df_doppler);

    auto own = GetOwner(actor);
    if (own && own->hasU())
    {
        pp = own->user.PlayerP;
        rnd_rng = RandomRange(1000);

        if (rnd_rng > 990)
            PlayerSound(DIGI_LIKEHIROSHIMA, v3df_follow|v3df_dontpan,pp);
        else if (rnd_rng > 980)
            PlayerSound(DIGI_LIKENAGASAKI, v3df_follow|v3df_dontpan,pp);
        else if (rnd_rng > 970)
            PlayerSound(DIGI_LIKEPEARL, v3df_follow|v3df_dontpan,pp);
    }

    // Spawn big mushroom cloud
    auto expActor = SpawnActor(STAT_MISSILE, MUSHROOM_CLOUD, s_NukeMushroom, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(own, expActor);
    expActor->spr.shade = -128;
    expActor->spr.xrepeat = 255;
    expActor->spr.yrepeat = 255;
    expActor->spr.clipdist = actor->spr.clipdist;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.spal = expActor->spr.pal = PALETTE_PLAYER1;  // Set nuke puff to gray

    InitChemBomb(expActor);


    // Do central explosion
    expActor = SpawnActor(STAT_MISSILE, MUSHROOM_CLOUD, s_GrenadeExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    SetOwner(own, expActor);
    expActor->spr.shade = -128;
    expActor->spr.xrepeat = 218;
    expActor->spr.yrepeat = 152;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);

    expActor->user.Radius = NUKE_RADIUS;

    SpawnExpZadjust(actor, expActor, Z(30), Z(30));

    DoExpDamageTest(expActor);

    // Nuclear effects
    SetNuclearQuake(actor);

    SetFadeAmt(pp, -80, 1); // Nuclear flash

    // Secondary blasts
    ang = RANDOM_P2(2048);
    SpawnNuclearSecondaryExp(expActor, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(expActor, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(expActor, ang);
    ang = ang + 512 + RANDOM_P2(256);
    SpawnNuclearSecondaryExp(expActor, ang);
}

void SpawnTracerExp(DSWActor* actor)
{
    DSWActor* expActor;

    ASSERT(actor->hasU());
    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return ;

    if (actor->user.ID == BOLT_THINMAN_R1)
        expActor = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R1, s_TracerExp, actor->sector(),
                                actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    else
        expActor = SpawnActor(STAT_MISSILE, TRACER_EXP, s_TracerExp, actor->sector(),
                                actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 4;
    expActor->spr.yrepeat = 4;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);

    if (actor->user.ID == BOLT_THINMAN_R1)
    {
        expActor->user.Radius = DamageData[DMG_BASIC_EXP].radius;
        DoExpDamageTest(expActor);
    }
    else
        expActor->user.Radius = DamageData[DMG_BOLT_EXP].radius;
}

void SpawnMicroExp(DSWActor* actor)
{
    ASSERT(actor->hasU());
    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return ;

    auto expActor = SpawnActor(STAT_MISSILE, MICRO_EXP, s_MicroExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 32;
    expActor->spr.yrepeat = 32;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_YFLIP);
    expActor->user.Radius = DamageData[DMG_BOLT_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(actor, expActor, Z(20), Z(20));
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);
}

void AddSpriteToSectorObject(DSWActor* actor, SECTOR_OBJECT* sop)
{
    unsigned sn;

    // make sure it has a user
    if (!actor->hasU())
    {
        SpawnUser(actor, 0, nullptr);
    }

    // find a free place on this list
    for (sn = 0; sn < SIZ(sop->so_actors); sn++)
    {
        if (sop->so_actors[sn] == nullptr)
            break;
    }

    if (sn >= SIZ(sop->so_actors) - 1) return;
    sop->so_actors[sn] = actor;
    so_setspriteinterpolation(sop, actor);

    actor->user.Flags |= (SPR_ON_SO_SECTOR|SPR_SO_ATTACHED);

    actor->user.pos.X = sop->int_pmid().X - actor->int_pos().X;
    actor->user.pos.Y = sop->int_pmid().Y - actor->int_pos().Y;
    actor->user.pos.Z = sop->mid_sector->int_floorz() - actor->int_pos().Z;

    actor->user.sang = actor->int_ang();
}

void SpawnBigGunFlames(DSWActor* actor, DSWActor* Operator, SECTOR_OBJECT* sop, bool smallflames)
{
    unsigned sn;

    auto expActor = SpawnActor(STAT_MISSILE, MICRO_EXP, s_BigGunFlame, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(Operator, expActor);
    expActor->spr.shade = -40;
    if (smallflames)
    {
        expActor->spr.xrepeat = 12;
        expActor->spr.yrepeat = 12;
    }
    else
    {
        expActor->spr.xrepeat = 34;
        expActor->spr.yrepeat = 34;
    }
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        expActor->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    // place all sprites on list
    for (sn = 0; sn < SIZ(sop->so_actors); sn++)
    {
        if (sop->so_actors[sn] == nullptr)
            break;
    }

    if (sn >= SIZ(sop->so_actors) - 1) return;

    sop->so_actors[sn] = expActor;
    so_setspriteinterpolation(sop, expActor);

    expActor->user.Flags |= (actor->user.Flags & (SPR_ON_SO_SECTOR|SPR_SO_ATTACHED));

    if (actor->user.Flags & (SPR_ON_SO_SECTOR))
    {
        // move with sector its on
        expActor->set_int_z(actor->sector()->int_floorz() - actor->user.pos.Z);
        expActor->backupz();
    }
    else
    {
        // move with the mid sector
        expActor->set_int_z(sop->mid_sector->int_floorz() - actor->user.pos.Z);
        expActor->backupz();
    }

    expActor->user.pos.X = actor->user.pos.X;
    expActor->user.pos.Y = actor->user.pos.Y;
    expActor->user.pos.Z = actor->user.pos.Z;
}

void SpawnGrenadeSecondaryExp(DSWActor* actor, int ang)
{
    int vel;

    ASSERT(actor->hasU());
    auto expActor = SpawnActor(STAT_MISSILE, GRENADE_EXP, s_GrenadeSmallExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 1024);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 32;
    expActor->spr.yrepeat = 32;
    expActor->spr.clipdist = actor->spr.clipdist;
    expActor->user.ceiling_dist = (16);
    expActor->user.floor_dist = (16);
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    //ang = RANDOM_P2(2048);
    vel = (1024+512) + RandomRange(1024);
    expActor->user.change.X = MOVEx(vel, ang);
    expActor->user.change.Y = MOVEy(vel, ang);

    expActor->user.coll = move_missile(expActor, expActor->user.change.X, expActor->user.change.Y, 0,
                           expActor->user.int_ceiling_dist(),expActor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (FindDistance3D(expActor->int_pos() - actor->int_pos()) < 1024)
    {
        KillActor(expActor);
        return;
    }

    SpawnExpZadjust(actor, expActor, Z(50), Z(10));
    expActor->backuppos();
}

int SpawnGrenadeSmallExp(DSWActor* actor)
{
    int ang = RANDOM_P2(2048);
    SpawnGrenadeSecondaryExp(actor, ang);
    return 0;
}

void SpawnGrenadeExp(DSWActor* actor)
{
    int dx,dy,dz;

    ASSERT(actor->hasU());
    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return;

    PlaySound(DIGI_30MMEXPLODE, actor, v3df_none);

    if (RandomRange(1000) > 990)
    {
        auto own = GetOwner(actor);
        if (own != nullptr && own->hasU() && own->user.PlayerP)
        {
            PlayerSound(DIGI_LIKEFIREWORKS, v3df_follow|v3df_dontpan, own->user.PlayerP);
        }
    }

    dx = actor->int_pos().X;
    dy = actor->int_pos().Y;
    dz = actor->int_pos().Z;

    if (actor->user.ID == ZILLA_RUN_R0)
    {
        dx += RandomRange(1000)-RandomRange(1000);
        dy += RandomRange(1000)-RandomRange(1000);
        dz = int_ActorZOfMiddle(actor) + RandomRange(1000)-RandomRange(1000);
    }

    auto expActor = SpawnActor(STAT_MISSILE, GRENADE_EXP, s_GrenadeExp, actor->sector(),
                            dx, dy, dz, actor->int_ang(), 0);


    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 64 + 32;
    expActor->spr.yrepeat = 64 + 32;
    expActor->spr.clipdist = actor->spr.clipdist;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_GRENADE_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(actor, expActor, Z(100), Z(30));

    DoExpDamageTest(expActor);

    SetExpQuake(expActor);
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 0);
}

void SpawnExpZadjust(DSWActor* actor, DSWActor* expActor, int upper_zsize, int lower_zsize)
{
    int tos_z, bos_z;

    ASSERT(expActor->hasU());

    if (actor->hasU())
    {
        tos_z = expActor->int_pos().Z - upper_zsize;
        bos_z = expActor->int_pos().Z + lower_zsize;

        if (tos_z <= actor->user.int_hiz() + Z(4))
        {
            expActor->set_int_z(actor->user.int_hiz() + upper_zsize);
            expActor->spr.cstat |= (CSTAT_SPRITE_YFLIP);
        }
        else if (bos_z > actor->user.int_loz())
        {
            expActor->set_int_z(actor->user.int_loz() - lower_zsize);
        }
    }
    else
    {
        int cz,fz;

        getzsofslopeptr(expActor->sector(), expActor->int_pos().X, expActor->int_pos().Y, &cz, &fz);

        tos_z = expActor->int_pos().Z - upper_zsize;
        bos_z = expActor->int_pos().Z + lower_zsize;

        if (tos_z <= cz + Z(4))
        {
            expActor->set_int_z(cz + upper_zsize);
            expActor->spr.cstat |= (CSTAT_SPRITE_YFLIP);
        }
        else if (bos_z > fz)
        {
            expActor->set_int_z(fz - lower_zsize);
        }
    }

    expActor->backupz();
}

void SpawnMineExp(DSWActor* actor)
{
    ASSERT(actor->hasU());
    if (actor->hasU() && (actor->user.Flags & SPR_SUICIDE))
        return;

    change_actor_stat(actor, STAT_MISSILE);

    PlaySound(DIGI_MINEBLOW, actor, v3df_none);

    auto expActor = SpawnActor(STAT_MISSILE, MINE_EXP, s_MineExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(GetOwner(actor), expActor);
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 64 + 44;
    expActor->spr.yrepeat = 64 + 44;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_MINE_EXP].radius;

    //
    // All this stuff assures that explosions do not go into floors &
    // ceilings
    //

    SpawnExpZadjust(actor, expActor, Z(100), Z(20));
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);

    SetExpQuake(expActor);
}


int DoMineExp(DSWActor* actor)
{
    DoExpDamageTest(actor);
    return 0;
}

int DoSectorExp(DSWActor* actor)
{
    actor->add_int_pos({ actor->user.change.X, actor->user.change.Y, 0 });
    return 0;
}

DSWActor* SpawnSectorExp(DSWActor* actor)
{
    short explosion;

    ASSERT(actor->hasU());
    if (actor->user.Flags & (SPR_SUICIDE))
        return nullptr;

    PlaySound(DIGI_30MMEXPLODE, actor, v3df_none);

    auto expActor = SpawnActor(STAT_MISSILE, GRENADE_EXP, s_SectorExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 90; // was 40,40
    expActor->spr.yrepeat = 90;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_SECTOR_EXP].radius;

    DoExpDamageTest(expActor);
    SetExpQuake(expActor);
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);

    return expActor;
}

// called from SpawnShrap
DSWActor* SpawnLargeExp(DSWActor* actor)
{
    PlaySound(DIGI_30MMEXPLODE, actor, v3df_none);

    auto expActor = SpawnActor(STAT_MISSILE, GRENADE_EXP, s_SectorExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 90; // was 40,40
    expActor->spr.yrepeat = 90;
    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_SECTOR_EXP].radius;

    SpawnExpZadjust(actor, expActor, Z(50), Z(50));

    // Should not cause other sectors to explode
    DoExpDamageTest(expActor);
    SetExpQuake(expActor);
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);

    return expActor;
}

void SpawnMeteorExp(DSWActor* actor)
{
    DSWActor* expActor;

    ASSERT(actor->hasU());
    if (actor->user.Flags & (SPR_SUICIDE))
        return;

    if (actor->user.spal == 25)    // Serp ball
    {
        expActor = SpawnActor(STAT_MISSILE, METEOR_EXP, s_TeleportEffect2, actor->sector(),
                                actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    }
    else
    {
        PlaySound(DIGI_MEDIUMEXP, actor, v3df_none);
        expActor = SpawnActor(STAT_MISSILE, METEOR_EXP, s_MeteorExp, actor->sector(),
                                actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    }

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    expActor->spr.shade = -40;
    if (actor->spr.yrepeat < 64)
    {
        // small
        expActor->spr.xrepeat = 64;
        expActor->spr.yrepeat = 64;
    }
    else
    {
        // large - boss
        expActor->spr.xrepeat = 80;
        expActor->spr.yrepeat = 80;
    }

    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_BASIC_EXP].radius;
}

void SpawnLittleExp(DSWActor* actor)
{
    short explosion;

    PlaySound(DIGI_HEADSHOTHIT, actor, v3df_none);
    auto expActor = SpawnActor(STAT_MISSILE, BOLT_EXP, s_SectorExp, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    expActor->spr.shade = -127;

    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_BASIC_EXP].radius;
    DoExpDamageTest(expActor);
    SpawnVis(nullptr, expActor->sector(), expActor->spr.pos, 16);
}

int DoFireball(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        actor->spr.xrepeat = actor->spr.yrepeat -= 1;
        if (actor->spr.xrepeat <= 37)
        {
            SpawnSmokePuff(actor);
            KillActor(actor);
            return true;
        }
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.coll.type != kHitNone)
    {
        bool hit_burn = false;

        if (WeaponMoveHit(actor))
        {
            switch (actor->user.coll.type)
            {
            case kHitSprite:
            {
                auto hitActor = actor->user.coll.actor();

                if ((hitActor->spr.extra & SPRX_BURNABLE))
                {
                    if (!hitActor->hasU())
                        SpawnUser(hitActor, hitActor->spr.picnum, nullptr);
                    SpawnFireballFlames(actor, hitActor);
                    hit_burn = true;
                }

                break;
            }
            }

            if (!hit_burn)
            {
                if (actor->user.ID == GORO_FIREBALL)
                    SpawnGoroFireballExp(actor);
                else
                    SpawnFireballExp(actor);
            }

            KillActor(actor);
            return true;
        }
    }
    return false;
}

int DoFindGround(DSWActor* actor)
{
    Collision ceilhit, florhit;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    auto save_cstat = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrange(actor->int_pos(), actor->sector(), &actor->user.hiz, &ceilhit, &actor->user.loz, &florhit, (((int) actor->spr.clipdist) << 2) - GETZRANGE_CLIP_ADJ, CLIPMASK_PLAYER);
    actor->spr.cstat = save_cstat;

    switch (florhit.type)
    {
    case kHitSprite:
    {
        auto florActor = florhit.actor();

        if ((florActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            // found a sprite floor
            actor->user.lowActor = florActor;
            actor->user.lo_sectp = nullptr;
            return true;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            auto bak_cstat = florActor->spr.cstat;
            florActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            DoFindGround(actor);
            florActor->spr.cstat = bak_cstat;
        }

        return false;
    }
    case kHitSector:
    {
        actor->user.lo_sectp = florhit.hitSector;
        actor->user.lowActor = nullptr;
        return true;
    }

    default:
        ASSERT(true == false);
        break;
    }

    return false;
}

int DoFindGroundPoint(DSWActor* actor)
{
    Collision ceilhit, florhit;

    // recursive routine to find the ground - either sector or floor sprite
    // skips over enemy and other types of sprites

    auto save_cstat = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    FAFgetzrangepoint(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), &actor->user.hiz, &ceilhit, &actor->user.loz, &florhit);
    actor->spr.cstat = save_cstat;

    switch (florhit.type)
    {
    case kHitSprite:
    {
        auto florActor = florhit.actor();

        if ((florActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
        {
            // found a sprite floor
            actor->user.lowActor = florActor;
            actor->user.lo_sectp = nullptr;
            return true;
        }
        else
        {
            // reset the blocking bit of what you hit and try again -
            // recursive
            auto bak_cstat = florActor->spr.cstat;
            florActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            DoFindGroundPoint(actor);
            florActor->spr.cstat = bak_cstat;
        }

        return false;
    }
    case kHitSector:
    {
        actor->user.lo_sectp = florhit.hitSector;
        actor->user.lowActor = nullptr;
        return true;
    }

    default:
        ASSERT(true == false);
        break;
    }

    return false;
}

int DoNapalm(DSWActor* actor)
{
     DoBlurExtend(actor, 1, 7);

    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        actor->spr.xrepeat = actor->spr.yrepeat -= 1;
        if (actor->spr.xrepeat <= 30)
        {
            SpawnSmokePuff(actor);
            KillActor(actor);
            return true;
        }
    }

	auto oldv = actor->spr.pos;

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    {
        // this sprite is suPlayerosed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (actor->user.coll.type == kHitSprite)
        {
            auto hitActor = actor->user.coll.actor();

            if ((hitActor->spr.cstat & CSTAT_SPRITE_BLOCK) && !(hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                auto hcstat = hitActor->spr.cstat;

				actor->spr.pos = oldv;

                hitActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);
                hitActor->spr.cstat = hcstat;
            }
        }
    }

    actor->user.Counter++;
    if (actor->user.Counter > 2)
        actor->user.Counter = 0;

    if (!actor->user.Counter)
    {
        PlaySound(DIGI_NAPPUFF, actor, v3df_none);

        auto expActor = SpawnActor(STAT_MISSILE, NAP_EXP, s_NapExp, actor->sector(),
                                actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

        expActor->spr.hitag = LUMINOUS; //Always full brightness
        SetOwner(actor, expActor);
        expActor->spr.shade = -40;
        expActor->spr.cstat = actor->spr.cstat;
        expActor->spr.xrepeat = 48;
        expActor->spr.yrepeat = 64;
        expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        if (RANDOM_P2(1024) < 512)
            expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
        expActor->spr.cstat &= ~(CSTAT_SPRITE_TRANSLUCENT);
        expActor->user.Radius = 1500;

        DoFindGroundPoint(expActor);
        MissileWaterAdjust(expActor);
        expActor->spr.pos.Z = expActor->user.loz;
        expActor->backupz();

        if (actor->user.Flags & (SPR_UNDERWATER))
            expActor->user.Flags |= SPR_UNDERWATER;

        ASSERT(expActor->spr.picnum == 3072);
        ASSERT(expActor->user.Tics == 0);
    }

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            KillActor(actor);
            return true;
        }
    }
    return false;
}


int DoBloodWorm(DSWActor* actor)
{
    short ang;
    int xvect,yvect;
    int bx,by;
    int amt;

    actor->user.coll = move_ground_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (actor->user.coll.type != kHitNone)
    {
        actor->user.change.X = -actor->user.change.X;
        actor->user.change.Y = -actor->user.change.Y;
        actor->user.coll.setNone();
        actor->set_int_ang(NORM_ANGLE(actor->int_ang() + 1024));
        return true;
    }

    MissileHitDiveArea(actor);

    if (!actor->user.z_tgt)
    {
        // stay alive for 10 seconds
        if (++actor->user.Counter3 > 3)
        {
            InitBloodSpray(actor, false, 1);
            InitBloodSpray(actor, false, 1);
            InitBloodSpray(actor, false, 1);

            // Kill any old zombies you own
            SWStatIterator it(STAT_ENEMY);
            while (auto itActor = it.Next())
            {
                if (!itActor->hasU()) continue;

                if (itActor->user.ID == ZOMBIE_RUN_R0 && GetOwner(itActor) == GetOwner(actor))
                {
                    InitBloodSpray(itActor, true, 105);
                    InitBloodSpray(itActor, true, 105);
                    SetSuicide(itActor);
                    break;
                }
            }

            SpawnZombie2(actor);
            KillActor(actor);
            return true;
        }
    }

    ang = NORM_ANGLE(actor->int_ang() + 512);

    xvect = bcos(ang);
    yvect = bsin(ang);

    bx = actor->int_pos().X;
    by = actor->int_pos().Y;

    amt = RANDOM_P2(2048) - 1024;
    actor->add_int_pos({ MulScale(amt,xvect, 15), MulScale(amt,yvect, 15), 0 });

    auto sect = actor->sector();
    updatesectorz(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, &sect);
    if (sect)
    {
        GlobalSkipZrange = true;
        InitBloodSpray(actor, false, 1);
        GlobalSkipZrange = false;
    }

    actor->set_int_xy(bx, by);

    return false;
}


int DoMeteor(DSWActor* actor)
{
    return false;
}

int DoSerpMeteor(DSWActor* actor)
{
	auto oldv = actor->spr.pos;

    actor->spr.xrepeat += MISSILEMOVETICS * 2;
    if (actor->spr.xrepeat > 80)
        actor->spr.xrepeat = 80;

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (actor->user.coll.type != kHitNone)
    {
        // this sprite is supposed to go through players/enemys
        // if hit a player/enemy back up and do it again with blocking reset
        if (actor->user.coll.type == kHitSprite)
        {
            auto hitActor = actor->user.coll.actor();

            if (hitActor->hasU() && hitActor->user.ID >= SKULL_R0 && hitActor->user.ID <= SKULL_SERP)
            {
                auto hcstat = hitActor->spr.cstat;

				actor->spr.pos = oldv;

                hitActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);
                hitActor->spr.cstat = hcstat;
            }
        }

        if (WeaponMoveHit(actor))
        {
            SpawnMeteorExp(actor);
            KillActor(actor);
            return true;
        }
    }
    return false;
}


int DoMirvMissile(DSWActor* actor)
{
    actor->spr.xrepeat += MISSILEMOVETICS * 2;
    if (actor->spr.xrepeat > 80)
        actor->spr.xrepeat = 80;

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);

    if (actor->user.coll.type != kHitNone)
    {
        if (WeaponMoveHit(actor))
        {
            SpawnMeteorExp(actor);
            KillActor(actor);
            return true;
        }
    }
    return false;
}

int DoMirv(DSWActor* actor)
{
    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z, actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS);

    MissileHitDiveArea(actor);

    if (actor->user.Flags & (SPR_UNDERWATER) && (RANDOM_P2(1024 << 4) >> 4) < 256)
        SpawnBubble(actor);


    actor->user.Counter++;
    actor->user.Counter &= 1;

    if (!actor->user.Counter)
    {
        int i;
        static short angs[] =
        {
            512,
            -512
        };

        for (i = 0; i < 2; i++)
        {
            auto actorNew = SpawnActor(STAT_MISSILE, MIRV_METEOR, &sg_MirvMeteor[0][0], actor->sector(),
                              actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, NORM_ANGLE(actor->int_ang() + angs[i]), 800);

            actorNew->user.RotNum = 5;
            NewStateGroup(actorNew, &sg_MirvMeteor[0]);
            actorNew->user.StateEnd = s_MirvMeteorExp;

            SetOwner(actor, actorNew);
            actorNew->spr.shade = -40;
            actorNew->spr.xrepeat = 40;
            actorNew->spr.yrepeat = 40;
            actorNew->spr.clipdist = 32L >> 2;
            actorNew->spr.zvel = 0;
            actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

            actorNew->user.ceiling_dist = (16);
            actorNew->user.floor_dist = (16);
            actorNew->user.Dist = 200;
            //actorNew->user.Dist = 0;

            actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
            actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
            actorNew->user.change.Z = actorNew->spr.zvel;

            if (actor->user.Flags & (SPR_UNDERWATER))
                actorNew->user.Flags |= (SPR_UNDERWATER);
        }

    }

    if (actor->user.coll.type != kHitNone)
    {
        SpawnMeteorExp(actor);
        KillActor(actor);
        return true;
    }

    return false;
}

bool MissileSetPos(DSWActor* actor, ANIMATOR* DoWeapon, int dist)
{
    int oldvel, oldzvel;
    int oldxc, oldyc, oldzc;
    bool retval = false;

    // backup values
    oldxc = actor->user.change.X;
    oldyc = actor->user.change.Y;
    oldzc = actor->user.change.Z;
    oldvel = actor->spr.xvel;
    oldzvel = actor->spr.zvel;

    // make missile move in smaller increments
    actor->spr.xvel = short((dist * 6) / MISSILEMOVETICS);
    //actor->spr.zvel = (actor->spr.zvel*4) / MISSILEMOVETICS;
    actor->spr.zvel = short((actor->spr.zvel*6) / MISSILEMOVETICS);

    // some Weapon Animators use this
    actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
    actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());
    actor->user.change.Z = actor->spr.zvel;

    actor->user.Flags |= (SPR_SET_POS_DONT_KILL);
    if ((*DoWeapon)(actor))
        retval = true;
    actor->user.Flags &= ~(SPR_SET_POS_DONT_KILL);

    // reset values
    actor->user.change.X = oldxc;
    actor->user.change.Y = oldyc;
    actor->user.change.Z = oldzc;
    actor->spr.xvel = oldvel;
    actor->spr.zvel = oldzvel;

    // update for interpolation
    actor->backuppos();

    return retval;
}

bool TestMissileSetPos(DSWActor* actor, ANIMATOR* DoWeapon, int dist, int zvel)
{
    int oldvel, oldzvel;
    int oldxc, oldyc, oldzc;
    bool retval = false;

    // backup values
    oldxc = actor->user.change.X;
    oldyc = actor->user.change.Y;
    oldzc = actor->user.change.Z;
    oldvel = actor->spr.xvel;
    oldzvel = actor->spr.zvel;

    // make missile move in smaller increments
    actor->spr.xvel = short((dist * 6) / MISSILEMOVETICS);
    //actor->spr.zvel = (actor->spr.zvel*4) / MISSILEMOVETICS;
    zvel = short((zvel*6) / MISSILEMOVETICS);

    // some Weapon Animators use this
    actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
    actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());
    actor->user.change.Z = zvel;

    actor->user.Flags |= (SPR_SET_POS_DONT_KILL);
    if ((*DoWeapon)(actor))
        retval = true;
    actor->user.Flags &= ~(SPR_SET_POS_DONT_KILL);

    // reset values
    actor->user.change.X = oldxc;
    actor->user.change.Y = oldyc;
    actor->user.change.Z = oldzc;
    actor->spr.xvel = oldvel;
    actor->spr.zvel = oldzvel;

    // update for interpolation
    actor->backuppos();

    return retval;
}

enum
{
    RINGMOVETICS = (MISSILEMOVETICS * 2),
    RING_OUTER_DIST = 3200,
    RING_INNER_DIST = 800,
};

int DoRing(DSWActor* actor)
{
    auto own = GetOwner(actor);
    if (!own) return 0; // this would crash.
    PLAYER* pp = own->user.PlayerP;;
    int cz,fz;

    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        actor->spr.xrepeat = actor->spr.yrepeat -= 1;
        if (actor->spr.xrepeat <= 30)
        {
            SpawnSmokePuff(actor);
            KillActor(actor);
            return true;
        }
    }

    int z;
    // move the center with the player
    if (pp)
        z = pp->int_ppos().Z + Z(20);
    else
        z = int_ActorZOfMiddle(own) + Z(30);

	actor->spr.pos = DVector3(own->spr.pos.XY(), z * zinttoworld);

    // go out until its time to come back in
    if (actor->user.Counter2 == false)
    {
        actor->user.Dist += 8 * RINGMOVETICS;

        if (actor->user.Dist > RING_OUTER_DIST)
        {
            actor->user.Counter2 = true;
        }
    }
    else
    {
        actor->user.Dist -= 8 * RINGMOVETICS;

        if (actor->user.Dist <= RING_INNER_DIST)
        {
            if (!pp)
                own->user.Counter--;
            KillActor(actor);
            return 0;
        }
    }

    // rotate the ring
    actor->set_int_ang(NORM_ANGLE(actor->int_ang() + (4 * RINGMOVETICS) + RINGMOVETICS));

    // put it out there
    actor->add_int_pos({ MulScale(actor->user.Dist, bcos(actor->int_ang()), 14), MulScale(actor->user.Dist, bsin(actor->int_ang()), 14),
        pp ? (actor->user.Dist * (-pp->horizon.horiz.asq16() >> 9)) >> 9 : 0 });

    SetActor(actor, actor->int_pos());

    ASSERT(actor->insector());

    getzsofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y, &cz, &fz);

    // bound the sprite by the sectors ceiling and floor
    if (actor->int_pos().Z > fz)
    {
        actor->set_int_z(fz);
    }

    if (actor->int_pos().Z < cz + ActorSizeZ(actor))
    {
        actor->set_int_z(cz + ActorSizeZ(actor));
    }

    // Done last - check for damage
    DoDamageTest(actor);

    return 0;
}



void InitSpellRing(PLAYER* pp)
{
    short ang, ang_diff, ang_start, missiles;
    short max_missiles = 16;
    short ammo;

    ammo = NAPALM_MIN_AMMO;

    if (pp->WpnAmmo[WPN_HOTHEAD] < ammo)
        return;
    else
        PlayerUpdateAmmo(pp, WPN_HOTHEAD, -ammo);

    ang_diff = 2048 / max_missiles;

    ang_start = NORM_ANGLE(pp->angle.ang.Buildang() - (2048 / 2));

    if (!SW_SHAREWARE)
        PlaySound(DIGI_RFWIZ, pp, v3df_none);

    if (!pp->insector())
        return;

    for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
    {
        auto actorNew = SpawnActor(STAT_MISSILE_SKIP4, FIREBALL1, s_Ring, pp->cursector, pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, ang, 0);

        actorNew->spr.hitag = LUMINOUS; //Always full brightness
        actorNew->spr.xvel = 500;
        SetOwner(pp->actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 32;
        actorNew->spr.yrepeat = 32;
        actorNew->spr.zvel = 0;

        actorNew->user.pos.Z = Z(20);
        actorNew->user.Dist = RING_INNER_DIST;
        actorNew->user.Counter = max_missiles;
        actorNew->user.Counter2 = 0;
        actorNew->user.ceiling_dist = (10);
        actorNew->user.floor_dist = (10);

        // put it out there
        actorNew->add_int_pos({ MulScale(actorNew->user.Dist, bcos(actorNew->int_ang()), 14), MulScale(actorNew->user.Dist, bsin(actorNew->int_ang()), 14),
                pp->int_ppos().Z + Z(20) + ((actorNew->user.Dist * (-pp->horizon.horiz.asq16() >> 9)) >> 9) });

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));

        actorNew->backuppos();

        if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }
}

int DoSerpRing(DSWActor* actor)
{
    int dist,a,b,c;
    int cz,fz;

    auto own = GetOwner(actor);
    // if Owner does not exist or he's dead on the floor
    // kill off all of his skull children
    if (own == nullptr || own->user.RotNum < 5)
    {
        UpdateSinglePlayKills(actor);
        DoSkullBeginDeath(actor);
        // +2 does not spawn shrapnel
        actor->user.ID = SKULL_SERP;
        return 0;
    }

    int z = actor->int_pos().Z + actor->spr.zvel;
    if (z > own->int_pos().Z - actor->user.pos.Z)
        z = own->int_pos().Z - actor->user.pos.Z;

    // move the center with the player
	actor->spr.pos = DVector3(own->spr.pos.XY(), z * zinttoworld);


    // go out until its time to come back in
    if (actor->user.Counter2 == false)
    {
        actor->user.Dist += 8 * RINGMOVETICS;

        if (actor->user.Dist > actor->user.TargetDist)
            actor->user.Counter2 = true;
    }

    // rotate the ring
    actor->user.slide_ang = NORM_ANGLE(actor->user.slide_ang + actor->spr.yvel);

    // rotate the heads
    if (actor->user.Flags & (SPR_BOUNCE))
        actor->set_int_ang(NORM_ANGLE(actor->int_ang() + (28 * RINGMOVETICS)));
    else
        actor->set_int_ang(NORM_ANGLE(actor->int_ang() - (28 * RINGMOVETICS)));

    // put it out there
    actor->add_int_pos({ MulScale(actor->user.Dist, bcos(actor->user.slide_ang), 14), MulScale(actor->user.Dist, bsin(actor->user.slide_ang), 14), 0 });

    SetActor(actor, actor->int_pos());

    ASSERT(actor->insector());

    getzsofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y, &cz, &fz);

    // bound the sprite by the sectors ceiling and floor
    if (actor->int_pos().Z > fz)
    {
        actor->set_int_z(fz);
    }

    if (actor->int_pos().Z < cz + ActorSizeZ(actor))
    {
        actor->set_int_z(cz + ActorSizeZ(actor));
    }

    if (actor->user.Counter2 > 0)
    {
        DSWActor* tActor = own->user.targetActor;
        if (!tActor->hasU() ||
            !tActor->user.PlayerP ||
            !(tActor->user.PlayerP->Flags & PF_DEAD))
        {
            actor->user.targetActor = own->user.targetActor;
            DISTANCE(actor->int_pos().X, actor->int_pos().Y, actor->user.targetActor->int_pos().X, actor->user.targetActor->int_pos().Y, dist, a,b,c);

            // if ((dist ok and random ok) OR very few skulls left)
            if ((dist < 18000 && (RANDOM_P2(2048<<5)>>5) < 16) || own->user.Counter < 4)
            {
                auto sect = actor->sector();
                updatesector(actor->int_pos().X, actor->int_pos().Y, &sect);

                // if (valid sector and can see target)
                if (sect != nullptr && CanSeePlayer(actor))
                {
                    extern STATE* sg_SkullJump[];
                    actor->user.ID = SKULL_R0;
                    actor->set_int_ang(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));
                    actor->spr.xvel = dist>>5;
                    actor->spr.xvel += (actor->spr.xvel >> 1);
                    actor->spr.xvel += (RANDOM_P2(128<<8)>>8);
                    actor->user.jump_speed = -800;
                    change_actor_stat(actor, STAT_ENEMY);
                    NewStateGroup(actor, sg_SkullJump);
                    DoBeginJump(actor);
                    return 0;
                }
            }
        }
    }

    return 0;
}

int InitLavaFlame(DSWActor* actor)
{
    return 0;
}

int InitLavaThrow(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;
    short w;

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, LAVA_BOULDER, s_LavaBoulder, actor->sector(),
                    nx, ny, nz, nang, NINJA_BOLT_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    actorNew->spr.yrepeat = 72;
    actorNew->spr.xrepeat = 72;
    actorNew->spr.shade = -15;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);

    if (RANDOM_P2(1024) > 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    actorNew->user.Radius = 200;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 256>>2;
    actorNew->user.ceiling_dist = (14);
    actorNew->user.floor_dist = (14);

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoLavaBoulder, 1200);

    // find the distance to the target (player)
    dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

    return 0;
}

void InitVulcanBoulder(DSWActor* actor)
{
    int nx, ny, nz, nang;
    int zsize;
    int zvel, zvel_rand;
    short delta;
    short vel;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - Z(40);

    if (SP_TAG7(actor))
    {
        delta = SP_TAG5(actor);
        nang = actor->int_ang() + (RandomRange(delta) - (delta >> 1));
        nang = NORM_ANGLE(nang);
    }
    else
    {
        nang = RANDOM_P2(2048);
    }

    if (SP_TAG6(actor))
        vel = SP_TAG6(actor);
    else
        vel = 800;

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, LAVA_BOULDER, s_VulcanBoulder, actor->sector(),
                    nx, ny, nz, nang, (vel/2 + vel/4) + RandomRange(vel/4));

    SetOwner(actor, actorNew);
    actorNew->spr.xrepeat = actorNew->spr.yrepeat = 8 + RandomRange(72);
    actorNew->spr.shade = -40;
    actorNew->set_int_ang(nang);
    actorNew->user.Counter = 0;

    zsize = ActorSizeZ(actorNew);

    actorNew->user.Radius = 200;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 256>>2;
    actorNew->user.ceiling_dist = zsize/2 * inttoworld;
    actorNew->user.floor_dist = zsize/2 * inttoworld;
    if (RANDOM_P2(1024) > 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) > 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    if (SP_TAG7(actor))
    {
        zvel = SP_TAG7(actor);
        zvel_rand = SP_TAG8(actor);
    }
    else
    {
        zvel = 50;
        zvel_rand = 40;
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = -Z(zvel) + -Z(RandomRange(zvel_rand));
}

int InitSerpRing(DSWActor* actor)
{
    short ang, ang_diff, ang_start, missiles;
    short max_missiles;

    const int SERP_RING_DIST = 2800; // Was 3500

    extern STATE s_SkullExplode[];
    extern STATE s_SkullRing[5][1];
    extern STATE* sg_SkullRing[];

    max_missiles = 12;

    actor->user.Counter = max_missiles;

    ang_diff = 2048 / max_missiles;

    ang_start = NORM_ANGLE(actor->int_ang() - (2048 / 2));

    PlaySound(DIGI_SERPSUMMONHEADS, actor, v3df_none);

    for (missiles = 0, ang = ang_start; missiles < max_missiles; ang += ang_diff, missiles++)
    {
        auto actorNew = SpawnActor(STAT_SKIP4, SKULL_SERP, &s_SkullRing[0][0], actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, ang, 0);

        actorNew->spr.xvel = 500;
        SetOwner(actor, actorNew);
        actorNew->spr.shade = -20;
        actorNew->spr.xrepeat = 64;
        actorNew->spr.yrepeat = 64;
        actorNew->spr.yvel = 2*RINGMOVETICS;
        actorNew->spr.zvel = Z(3);
        actorNew->spr.pal = 0;

        actorNew->set_int_z(ActorZOfTop(actor) - Z(20));
        actorNew->user.pos.Z = Z(50);

        // ang around the serp is now slide_ang
        actorNew->user.slide_ang = actorNew->int_ang();
        // randomize the head turning angle
        actorNew->set_int_ang(RANDOM_P2(2048<<5)>>5);

        // control direction of spinning
        actor->user.Flags ^= SPR_BOUNCE;
        actorNew->user.Flags |= (actor->user.Flags & (SPR_BOUNCE));

        actorNew->user.Dist = 600;
        actorNew->user.TargetDist = SERP_RING_DIST;
        actorNew->user.Counter2 = 0;

        actorNew->user.StateEnd = s_SkullExplode;
        actorNew->user.Rot = sg_SkullRing;

        // defaults do change the statnum
        EnemyDefaults(actorNew, nullptr, nullptr);
        change_actor_stat(actorNew, STAT_SKIP4);
        actorNew->spr.extra &= ~(SPRX_PLAYER_OR_ENEMY);

        actorNew->spr.clipdist = (128+64) >> 2;
        actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

        actorNew->user.Radius = 400;
    }
    return 0;
}

void InitSpellNapalm(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    unsigned i;
    short ammo;

    static const MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {0, 1100, 0},
        {600 * 6, 400, -512},
    };

    ammo = NAPALM_MIN_AMMO;

    if (pp->WpnAmmo[WPN_HOTHEAD] < ammo)
        return;
    else
        PlayerUpdateAmmo(pp, WPN_HOTHEAD, -ammo);

    PlaySound(DIGI_NAPFIRE, pp, v3df_none);

    if (!pp->insector())
        return;

    for (i = 0; i < SIZ(mp); i++)
    {
        auto actor = SpawnActor(STAT_MISSILE, FIREBALL1, s_Napalm, pp->cursector,
                                pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z + Z(12), pp->angle.ang.Buildang(), NAPALM_VELOCITY*2);

        actor->spr.hitag = LUMINOUS; //Always full brightness

        if (i==0) // Only attach sound to first projectile
        {
            PlaySound(DIGI_NAPWIZ, actor, v3df_follow);
        }

        SetOwner(pp->actor, actor);
        actor->spr.shade = -40;
        actor->spr.xrepeat = 32;
        actor->spr.yrepeat = 32;
        actor->spr.clipdist = 0;
        actor->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
        actor->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
        actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        actor->user.Flags2 |= (SPR2_BLUR_TAPER_FAST);

        actor->user.floor_dist = (1);
        actor->user.ceiling_dist = (1);
        actor->user.Dist = 200;

        auto oclipdist = plActor->spr.clipdist;
        plActor->spr.clipdist = 1;

        if (mp[i].dist_over != 0)
        {
            actor->set_int_ang(NORM_ANGLE(actor->int_ang() + mp[i].ang));
            HelpMissileLateral(actor, mp[i].dist_over);
            actor->set_int_ang(NORM_ANGLE(actor->int_ang() - mp[i].ang));
        }

        actor->user.change.X = MOVEx(actor->spr.xvel, actor->int_ang());
        actor->user.change.Y = MOVEy(actor->spr.xvel, actor->int_ang());
        actor->user.change.Z = actor->spr.zvel;

        if (MissileSetPos(actor, DoNapalm, mp[i].dist_out))
        {
            plActor->spr.clipdist = oclipdist;
            KillActor(actor);
            continue;
        }

        if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actor))
            actor->user.Flags |= (SPR_UNDERWATER);

        plActor->spr.clipdist = oclipdist;

        actor->user.Counter = 0;

    }
}

int InitEnemyNapalm(DSWActor* actor)
{
    short dist;
    unsigned i;

    static const MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {0, 1100, 0},
        {600 * 6, 400, -512},
    };

    PlaySound(DIGI_NAPFIRE, actor, v3df_none);

    for (i = 0; i < SIZ(mp); i++)
    {
        auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL1, s_Napalm, actor->sector(),
                        actor->int_pos().X, actor->int_pos().Y, ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2), actor->int_ang(), NAPALM_VELOCITY);

        actorNew->spr.hitag = LUMINOUS; //Always full brightness
        if (i==0) // Only attach sound to first projectile
        {
            PlaySound(DIGI_NAPWIZ, actorNew, v3df_follow);
        }

        if (actor->user.ID == ZOMBIE_RUN_R0)
            SetOwner(GetOwner(actor), actorNew);
        else
            SetOwner(actor, actorNew);

        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 32;
        actorNew->spr.yrepeat = 32;
        actorNew->spr.clipdist = 0;
        actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        actorNew->user.Flags2 |= (SPR2_BLUR_TAPER_FAST);

        actorNew->user.floor_dist = (1);
        actorNew->user.ceiling_dist = (1);
        actorNew->user.Dist = 200;

        auto oclipdist = actor->spr.clipdist;
        actor->spr.clipdist = 1;

        if (mp[i].dist_over != 0)
        {
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + mp[i].ang));
            HelpMissileLateral(actorNew, mp[i].dist_over);
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - mp[i].ang));
        }

        // find the distance to the target (player)
		dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

        if (dist != 0)
            actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;

        MissileSetPos(actorNew, DoNapalm, mp[i].dist_out);

        actor->spr.clipdist = oclipdist;

        actor->user.Counter = 0;

    }
    return 0;
}

int InitSpellMirv(PLAYER* pp)
{
    PlaySound(DIGI_MIRVFIRE, pp, v3df_none);

    if (!pp->insector())
        return 0;

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL1, s_Mirv, pp->cursector,
                            pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z + Z(12), pp->angle.ang.Buildang(), MIRV_VELOCITY);

    PlaySound(DIGI_MIRVWIZ, actorNew, v3df_follow);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = 72;
    actorNew->spr.yrepeat = 72;
    actorNew->spr.clipdist = 32 >> 2;
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.floor_dist = (16);
    actorNew->user.ceiling_dist = (16);
    actorNew->user.Dist = 200;

    DSWActor* plActor = pp->actor;
    auto oclipdist = plActor->spr.clipdist;
    plActor->spr.clipdist = 0;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoMirv, 600);
    plActor->spr.clipdist = oclipdist;

    actorNew->user.Counter = 0;
    return 0;
}

int InitEnemyMirv(DSWActor* actor)
{
    int dist;

    PlaySound(DIGI_MIRVFIRE, actor, v3df_none);

    auto actorNew = SpawnActor(STAT_MISSILE, MIRV_METEOR, s_Mirv, actor->sector(),
                    actor->int_pos().X, actor->int_pos().Y, ActorZOfTop(actor) + (ActorSizeZ(actor) >> 2), actor->int_ang(), MIRV_VELOCITY);

    PlaySound(DIGI_MIRVWIZ, actorNew, v3df_follow);

    SetOwner(actor, actorNew);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = 72;
    actorNew->spr.yrepeat = 72;
    actorNew->spr.clipdist = 32L >> 2;

    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->user.floor_dist = (16);
    actorNew->user.ceiling_dist = (16);
    actorNew->user.Dist = 200;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoMirv, 600);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;
    return 0;
}


int InitSwordAttack(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    unsigned stat;
    int dist;
    short reach, face;

    PlaySound(DIGI_SWORDSWOOSH, pp, v3df_dontpan | v3df_doppler);

    if (pp->Flags & (PF_DIVING))
    {
        DSWActor* bubble;
        int nx, ny;
        short random_amt;

        static const int16_t dangs[] =
        {
            -256, -128, 0, 128, 256
        };

        for (size_t i = 0; i < countof(dangs); i++)
        {
            if (RandomRange(1000) < 500) continue; // Don't spawn bubbles every time
            bubble = SpawnBubble(pp->actor);
            if (bubble != nullptr)
            {
                bubble->set_int_ang(pp->angle.ang.Buildang());

                random_amt = (RANDOM_P2(32 << 8) >> 8) - 16;

                // back it up a bit to get it out of your face
                nx = MOVEx((1024 + 256) * 3, NORM_ANGLE(bubble->int_ang() + dangs[i] + random_amt));
                ny = MOVEy((1024 + 256) * 3, NORM_ANGLE(bubble->int_ang() + dangs[i] + random_amt));

                move_missile(bubble, nx, ny, 0, plActor->user.int_ceiling_dist(), plActor->user.int_floor_dist(), CLIPMASK_PLAYER, 1);
            }
        }
    }

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (!itActor->hasU())
                break;

            if (itActor->user.PlayerP == pp)
                break;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            dist = Distance(pp->int_ppos().X, pp->int_ppos().Y, itActor->int_pos().X, itActor->int_pos().Y);

            reach = 1000; // !JIM! was 800
            face = 200;

            if (dist < CloseRangeDist(itActor, plActor, reach) && PlayerFacingRange(pp, itActor, face))
            {
                if (SpriteOverlapZ(pp->actor, itActor, Z(20)))
                {
                    if (FAFcansee(itActor->int_pos().X, itActor->int_pos().Y, int_ActorZOfMiddle(itActor), itActor->sector(), plActor->int_pos().X, plActor->int_pos().Y, int_ActorZOfMiddle(plActor), plActor->sector()))
                        DoDamage(itActor, pp->actor);
                }
            }
        }
    }

    // all this is to break glass
    {
        HitInfo hit{};
        short daang;
        int daz;

        daang = pp->angle.ang.Buildang();
        daz = -MulScale(pp->horizon.horiz.asq16(), 2000, 16) + (RandomRange(24000) - 12000);

        FAFhitscan(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector,       // Start position
            bcos(daang),      // X vector of 3D ang
            bsin(daang),      // Y vector of 3D ang
            daz,              // Z vector of 3D ang
            hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
            return 0;

        if (FindDistance3D(pp->int_ppos() - hit.int_hitpos()) < 700)
        {

            if (hit.actor() != nullptr)
            {
                extern STATE s_TrashCanPain[];
                auto hitActor = hit.actor();

                if (hitActor->hasU())     // JBF: added null check
                {
                    switch (hitActor->user.ID)
                    {
                    case ZILLA_RUN_R0:
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_SWORDCLANK, hit.int_hitpos(), v3df_none);
                        break;
                    case TRASHCAN:
                        if (hitActor->user.WaitTics <= 0)
                        {
                            hitActor->user.WaitTics = SEC(2);
                            ChangeState(hitActor, s_TrashCanPain);
                        }
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_SWORDCLANK, hit.int_hitpos(), v3df_none);
                        PlaySound(DIGI_TRASHLID, hitActor, v3df_none);
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case 623:
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_SWORDCLANK, hit.int_hitpos(), v3df_none);
                        break;
                    }
                }

                if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(nullptr, WPN_STAR, hitActor))
                        return 0;
                }

                if ((hitActor->spr.extra & SPRX_BREAKABLE))
                {
                    HitBreakSprite(hitActor, 0);
                }

                // hit a switch?
                if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
                {
                    ShootableSwitch(hitActor);
                }

            }

            if (hit.hitWall != nullptr)
            {
                if (hit.hitWall->twoSided())
                {
                    if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
                    {
                        if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (hit.hitWall->lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, plActor->user.ID);
                }
                // hit non breakable wall - do sound and puff
                else
                {
                    SpawnSwordSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                    PlaySound(DIGI_SWORDCLANK, hit.int_hitpos(), v3df_none);
                }
            }
        }
    }
    return 0;
}

int InitFistAttack(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    unsigned stat;
    int dist;
    short reach,face;

    PlaySound(DIGI_STAR, pp, v3df_dontpan|v3df_doppler);

    if (pp->Flags & (PF_DIVING))
    {
        DSWActor* bubble;
        int nx,ny;
        short random_amt;

        static int16_t dangs[] =
        {
            -128,128
        };

        for (size_t i = 0; i < countof(dangs); i++)
        {
            bubble = SpawnBubble(pp->actor);
            if (bubble != nullptr)
            {
                bubble->set_int_ang(pp->angle.ang.Buildang());

                random_amt = (RANDOM_P2(32<<8)>>8) - 16;

                // back it up a bit to get it out of your face
                nx = MOVEx((1024+256)*3, NORM_ANGLE(bubble->int_ang() + dangs[i] + random_amt));
                ny = MOVEy((1024+256)*3, NORM_ANGLE(bubble->int_ang() + dangs[i] + random_amt));

                move_missile(bubble, nx, ny, 0, plActor->user.int_ceiling_dist(), plActor->user.int_floor_dist(), CLIPMASK_PLAYER, 1);
            }
        }
    }

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor->user.PlayerP == pp)
                break;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            dist = Distance(pp->int_ppos().X, pp->int_ppos().Y, itActor->int_pos().X, itActor->int_pos().Y);

            if (pp->InventoryActive[2]) // Shadow Bombs give you demon fist
            {
                face = 190;
                reach = 2300;
            }
            else
            {
                reach = 1000;
                face = 200;
            }

            if (dist < CloseRangeDist(itActor, plActor, reach) && PlayerFacingRange(pp, itActor, face))
            {
                if (SpriteOverlapZ(pp->actor, itActor, Z(20)) || face == 190)
                {
                    if (FAFcansee(itActor->int_pos().X, itActor->int_pos().Y, int_ActorZOfMiddle(itActor), itActor->sector(), plActor->int_pos().X, plActor->int_pos().Y, int_ActorZOfMiddle(plActor), plActor->sector()))
                        DoDamage(itActor, plActor);
                    if (face == 190)
                    {
                        SpawnDemonFist(itActor);
                    }
                }
            }
        }
    }


    // all this is to break glass
    {
        HitInfo hit{};
        short daang;
        int daz;

        daang = pp->angle.ang.Buildang();
        daz = -MulScale(pp->horizon.horiz.asq16(), 2000, 16) + (RandomRange(24000) - 12000);

        FAFhitscan(pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z, pp->cursector,       // Start position
                   bcos(daang),      // X vector of 3D ang
                   bsin(daang),      // Y vector of 3D ang
                   daz,              // Z vector of 3D ang
                   hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
            return 0;

        if (FindDistance3D(pp->int_ppos() - hit.int_hitpos()) < 700)
        {

            if (hit.actor() != nullptr)
            {
                extern STATE s_TrashCanPain[];
                auto hitActor = hit.actor();

                if (hitActor->hasU())     // JBF: added null check
                {
                    switch (hitActor->user.ID)
                    {
                    case ZILLA_RUN_R0:
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_ARMORHIT, hit.int_hitpos(), v3df_none);
                        break;
                    case TRASHCAN:
                        if (hitActor->user.WaitTics <= 0)  
                        {
                            hitActor->user.WaitTics = SEC(2);
                            ChangeState(hitActor, s_TrashCanPain);
                        }
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_ARMORHIT, hit.int_hitpos(), v3df_none);
                        PlaySound(DIGI_TRASHLID, hitActor, v3df_none);
                        break;
                    case PACHINKO1:
                    case PACHINKO2:
                    case PACHINKO3:
                    case PACHINKO4:
                    case 623:
                        SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                        PlaySound(DIGI_ARMORHIT, hit.int_hitpos(), v3df_none);
                        break;
                    }
                }

                if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(nullptr, WPN_STAR, hitActor))
                        return 0;
                }

                if ((hitActor->spr.extra & SPRX_BREAKABLE))
                {
                    HitBreakSprite(hitActor,0);
                }

                // hit a switch?
                if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
                {
                    ShootableSwitch(hitActor);
                }

                switch (hitActor->spr.picnum)
                {
                case 5062:
                case 5063:
                case 4947:
                    SpawnSwordSparks(pp, hit.hitSector, nullptr, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                    PlaySound(DIGI_ARMORHIT, hit.int_hitpos(), v3df_none);
                    if (RandomRange(1000) > 700)
                        PlayerUpdateHealth(pp,1); // Give some health
                    hitActor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
                    break;
                }
            }


            if (hit.hitWall != nullptr)
            {
                if (hit.hitWall->twoSided())
                {
                    if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
                    {
                        if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (hit.hitWall->lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, plActor->user.ID);
                }
                // hit non breakable wall - do sound and puff
                else
                {
                    SpawnSwordSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
                    PlaySound(DIGI_ARMORHIT, hit.int_hitpos(), v3df_none);
                    if (PlayerTakeDamage(pp, nullptr))
                    {
                        PlayerUpdateHealth(pp, -(RandomRange(2<<8)>>8));
                        PlayerCheckDeath(pp, nullptr);
                    }
                }
            }
        }

        return 0;
    }
}

int InitSumoNapalm(DSWActor* actor)
{
    short dist;
    short ang;

    static const MISSILE_PLACEMENT mp[] =
    {
        {0, 1100, 0},
    };

    PlaySound(DIGI_NAPFIRE, actor, v3df_none);

    ang = actor->int_ang();
    for (int j = 0; j < 4; j++)
    {
        for (size_t i = 0; i < countof(mp); i++)
        {
            auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL1, s_Napalm, actor->sector(),
                actor->int_pos().X, actor->int_pos().Y, ActorZOfTop(actor), ang, NAPALM_VELOCITY);

            actorNew->spr.hitag = LUMINOUS; //Always full brightness
            if (i == 0) // Only attach sound to first projectile
            {
                PlaySound(DIGI_NAPWIZ, actorNew, v3df_follow);
            }

            SetOwner(actor, actorNew);
            actorNew->spr.shade = -40;
            actorNew->spr.xrepeat = 32;
            actorNew->spr.yrepeat = 32;
            actorNew->spr.clipdist = 0;
            actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
            actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            actorNew->user.Flags2 |= (SPR2_BLUR_TAPER_FAST);

            actorNew->user.floor_dist = (1);
            actorNew->user.ceiling_dist = (1);
            actorNew->user.Dist = 200;

            auto oclipdist = actor->spr.clipdist;
            actor->spr.clipdist = 1;

            if (mp[i].dist_over != 0)
            {
                actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + mp[i].ang));
                HelpMissileLateral(actorNew, mp[i].dist_over);
                actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - mp[i].ang));
            }

            // find the distance to the target (player)
			dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

            if (dist != 0)
                actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

            actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
            actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
            actorNew->user.change.Z = actorNew->spr.zvel;

            MissileSetPos(actorNew, DoNapalm, mp[i].dist_out);

            actor->spr.clipdist = oclipdist;

            actor->user.Counter = 0;

        }
        ang += 512;
    }
    return 0;
}

int InitSumoSkull(DSWActor* actor)
{
    extern STATE s_SkullExplode[];
    extern STATE s_SkullWait[5][1];
    extern STATE* sg_SkullWait[];
    extern ATTRIBUTE SkullAttrib;


    PlaySound(DIGI_SERPSUMMONHEADS, actor, v3df_none);

    auto actorNew = SpawnActor(STAT_ENEMY, SKULL_R0, &s_SkullWait[0][0], actor->sector(), actor->int_pos().X, actor->int_pos().Y, int_ActorZOfMiddle(actor), actor->int_ang(), 0);

    actorNew->spr.xvel = 500;
    SetOwner(actor, actorNew);
    actorNew->spr.shade = -20;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.yrepeat = 64;
    actorNew->spr.pal = 0;

    // randomize the head turning angle
    actorNew->set_int_ang(RANDOM_P2(2048<<5)>>5);

    // control direction of spinning
    actor->user.Flags ^= SPR_BOUNCE;
    actorNew->user.Flags |= (actor->user.Flags & (SPR_BOUNCE));

    actorNew->user.StateEnd = s_SkullExplode;
    actorNew->user.Rot = sg_SkullWait;

    actorNew->user.Attrib = &SkullAttrib;
    DoActorSetSpeed(actor, NORM_SPEED);
    actorNew->user.Counter = RANDOM_P2(2048);
    actorNew->user.pos.Z = actorNew->int_pos().Z;
    actorNew->user.Health = 100;

    // defaults do change the statnum
    EnemyDefaults(actorNew, nullptr, nullptr);
    actorNew->spr.extra |= SPRX_PLAYER_OR_ENEMY;

    actorNew->spr.clipdist = (128+64) >> 2;
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->user.Radius = 400;
    return 0;
}

int InitSumoStompAttack(DSWActor* actor)
{
    unsigned stat;
    int dist;
    short reach;


    PlaySound(DIGI_30MMEXPLODE, actor, v3df_dontpan|v3df_doppler);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor != actor->user.targetActor)
                break;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

			dist = DistanceI(actor->spr.pos, itActor->spr.pos);

            reach = 16384;

            if (dist < CloseRangeDist(itActor, actor, reach))
            {
                if (FAFcansee(itActor->int_pos().X, itActor->int_pos().Y, int_ActorZOfMiddle(itActor), itActor->sector(), actor->int_pos().X, actor->int_pos().Y, int_ActorZOfMiddle(actor), actor->sector()))
                    DoDamage(itActor, actor);
            }
        }
    }


    return 0;
}

int InitMiniSumoClap(DSWActor* actor)
{
    int dist;
    short reach;

    auto targetActor = actor->user.targetActor;
    if (!targetActor) return 0;

	dist = DistanceI(actor->spr.pos, targetActor->spr.pos);

    reach = 10000;

    if (dist < CloseRangeDist(targetActor, actor, 1000))
    {
        if (SpriteOverlapZ(actor, targetActor, Z(20)))
        {
            if (FAFcansee(targetActor->int_pos().X, targetActor->int_pos().Y, int_ActorZOfMiddle(targetActor), targetActor->sector(), actor->int_pos().X, actor->int_pos().Y, int_ActorZOfMiddle(actor), actor->sector()))
            {
                PlaySound(DIGI_CGTHIGHBONE, actor, v3df_follow | v3df_dontpan);
                DoDamage(targetActor, actor);
            }
        }
    }
    else if (dist < CloseRangeDist(targetActor, actor, reach))
    {
        if (FAFcansee(targetActor->int_pos().X, targetActor->int_pos().Y, int_ActorZOfMiddle(targetActor), targetActor->sector(), actor->int_pos().X, actor->int_pos().Y, int_ActorZOfMiddle(actor), actor->sector()))
        {
            PlaySound(DIGI_30MMEXPLODE, actor, v3df_none);
            SpawnFireballFlames(actor, targetActor);
        }
    }

    return 0;
}

int WeaponAutoAim(DSWActor* actor, DSWActor* mislActor, short ang, bool test)
{
    int dist;
    int zh;

    if (actor->hasU() && actor->user.PlayerP)
    {
        if (Autoaim(actor->user.PlayerP->pnum) != 1)
        {
            return -1;
        }
    }

    DSWActor* hitActor;
    if ((hitActor = DoPickTarget(actor, ang, test)) != nullptr)
    {
        mislActor->user.WpnGoalActor = hitActor;
        hitActor->user.Flags |= (SPR_TARGETED);
        hitActor->user.Flags |= (SPR_ATTACKED);

        mislActor->set_int_ang(NORM_ANGLE(getangle(hitActor->int_pos().X - mislActor->int_pos().X, hitActor->int_pos().Y - mislActor->int_pos().Y)));
        dist = FindDistance2D(mislActor->int_pos().vec2 - hitActor->int_pos().vec2);

        if (dist != 0)
        {
            int tos, diff, siz;

            tos = ActorZOfTop(hitActor);
            diff = mislActor->int_pos().Z - tos;
            siz = ActorSizeZ(hitActor);

            // hit_sprite is below
            if (diff < -Z(50))
                zh = tos + (siz >> 1);
            else
            // hit_sprite is above
            if (diff > Z(50))
                zh = tos + (siz >> 3);
            else
                zh = tos + (siz >> 2);

            mislActor->spr.zvel = (mislActor->spr.xvel * (zh - mislActor->int_pos().Z)) / dist;
        }
        return 0;
    }

    return -1;
}

int WeaponAutoAimZvel(DSWActor* actor, DSWActor* missileActor, int *zvel, short ang, bool test)
{
    int dist;
    int zh;

#if 0
    //formula for leading a player
    dist = Distance(actor->int_pos().X, actor->int_pos().Y, hp->pos.X, hp->pos.Y);
    time_to_target = dist/missileActor->spr.xvel;
    lead_dist = time_to_target*hp->vel;
#endif

    if (actor->hasU() && actor->user.PlayerP)
    {
        if (!Autoaim(actor->user.PlayerP->pnum))
        {
            return -1;
        }
    }

    DSWActor* hitActor;
    if ((hitActor = DoPickTarget(actor, ang, test)) != nullptr)
    {
        missileActor->user.WpnGoalActor = hitActor;
        hitActor->user.Flags |= (SPR_TARGETED);
        hitActor->user.Flags |= (SPR_ATTACKED);

        missileActor->set_int_ang(NORM_ANGLE(getangle(hitActor->int_pos().X - missileActor->int_pos().X, hitActor->int_pos().Y - missileActor->int_pos().Y)));
        dist = FindDistance2D(missileActor->int_pos().vec2 - hitActor->int_pos().vec2);

        if (dist != 0)
        {
            int tos, diff, siz;

            tos = ActorZOfTop(hitActor);
            diff = missileActor->int_pos().Z - tos;
            siz = ActorSizeZ(hitActor);

            // hit_sprite is below
            if (diff < -Z(50))
                zh = tos + (siz >> 1);
            else
                // hit_sprite is above
                if (diff > Z(50))
                    zh = tos + (siz >> 3);
                else
                    zh = tos + (siz >> 2);

            *zvel = (missileActor->spr.xvel * (zh - missileActor->int_pos().Z)) / dist;
        }
        return 0;
    }

    return -1;
}


DSWActor* AimHitscanToTarget(DSWActor* actor, int *z, short *ang, int z_ratio)
{
    int dist;
    int zh;
    int xvect;
    int yvect;

    DSWActor* hitActor = actor->user.targetActor;
    if (hitActor == nullptr)
        return nullptr;

    hitActor->user.Flags |= (SPR_TARGETED);
    hitActor->user.Flags |= (SPR_ATTACKED);

    *ang = getangle(hitActor->int_pos().X - actor->int_pos().X, hitActor->int_pos().Y - actor->int_pos().Y);

    // find the distance to the target
    dist = ksqrt(SQ(actor->int_pos().X - hitActor->int_pos().X) + SQ(actor->int_pos().Y - hitActor->int_pos().Y));

    if (dist != 0)
    {
        zh = ActorUpperZ(hitActor);

        xvect = bcos(*ang);
        yvect = bsin(*ang);

        if (hitActor->int_pos().X - actor->int_pos().X != 0)
            *z = Scale(xvect,zh - *z,hitActor->int_pos().X - actor->int_pos().X);
        else if (hitActor->int_pos().Y - actor->int_pos().Y != 0)
            *z = Scale(yvect,zh - *z,hitActor->int_pos().Y - actor->int_pos().Y);
        else
            *z = 0;

        // so actors won't shoot straight up at you
        // need to be a bit of a distance away
        // before they have a valid shot
        if (labs(*z / dist) > z_ratio)
        {
            return nullptr;
        }
    }


    return hitActor;
}

DSWActor* WeaponAutoAimHitscan(DSWActor* actor, int *z, short *ang, bool test)
{
    int dist;
    int zh;
    int xvect;
    int yvect;

    if (actor->hasU() && actor->user.PlayerP)
    {
        if (!Autoaim(actor->user.PlayerP->pnum))
        {
            return nullptr;
        }
    }

    DSWActor* picked;
    if ((picked = DoPickTarget(actor, *ang, test)) != nullptr)
    {

        picked->user.Flags |= (SPR_TARGETED);
        picked->user.Flags |= (SPR_ATTACKED);

        *ang = NORM_ANGLE(getangle(picked->int_pos().X - actor->int_pos().X, picked->int_pos().Y - actor->int_pos().Y));

        // find the distance to the target
        dist = ksqrt(SQ(actor->int_pos().X - picked->int_pos().X) + SQ(actor->int_pos().Y - picked->int_pos().Y));

        if (dist != 0)
        {
            zh = ActorZOfTop(picked) + (ActorSizeZ(picked) >> 2);

            xvect = bcos(*ang);
            yvect = bsin(*ang);

            if (picked->int_pos().X - actor->int_pos().X != 0)
                *z = Scale(xvect,zh - *z,picked->int_pos().X - actor->int_pos().X);
            else if (picked->int_pos().Y - actor->int_pos().Y != 0)
                *z = Scale(yvect,zh - *z,picked->int_pos().Y - actor->int_pos().Y);
            else
                *z = 0;
        }
    }

    return picked;
}

void WeaponHitscanShootFeet(DSWActor* actor, DSWActor* hitActor, int *zvect)
{
    int dist;
    int zh;
    int xvect;
    int yvect;
    int z;
    short ang;

    ang = NORM_ANGLE(getangle(hitActor->int_pos().X - actor->int_pos().X, hitActor->int_pos().Y - actor->int_pos().Y));

    // find the distance to the target
	dist = int((actor->spr.pos.XY() - hitActor->spr.pos.XY()).Length() * worldtoint);

    if (dist != 0)
    {
        zh = ActorZOfBottom(hitActor) + Z(20);
        z = actor->int_pos().Z;

        xvect = bcos(ang);
        yvect = bsin(ang);

        if (hitActor->int_pos().X - actor->int_pos().X != 0)
            //*z = xvect * ((zh - *z)/(hitActor->int_pos().X - actor->int_pos().X));
            *zvect = Scale(xvect,zh - z, hitActor->int_pos().X - actor->int_pos().X);
        else if (hitActor->int_pos().Y - actor->int_pos().Y != 0)
            //*z = yvect * ((zh - *z)/(hitActor->int_pos().Y - actor->int_pos().Y));
            *zvect = Scale(yvect,zh - z, hitActor->int_pos().Y - actor->int_pos().Y);
        else
            *zvect = 0;
    }
}

int InitStar(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    int nx, ny, nz;
    int zvel;

    static short dang[] = {-12, 12};
    const int STAR_REPEAT = 26;
    const int STAR_HORIZ_ADJ = 100;

    PlayerUpdateAmmo(pp, plActor->user.WeaponNum, -3);

    PlaySound(DIGI_STAR, pp, v3df_dontpan|v3df_doppler);

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    nz = pp->int_ppos().Z + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, STAR1, s_Star, pp->cursector, nx, ny, nz, pp->angle.ang.Buildang(), STAR_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = actorNew->spr.xrepeat = STAR_REPEAT;
    actorNew->spr.shade = -25;
    actorNew->spr.clipdist = 32 >> 2;
    // actorNew->spr.zvel was overflowing with this calculation - had to move to a local
    // long var
    zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT+STAR_HORIZ_ADJ, 16);

    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->user.WeaponNum = plActor->user.WeaponNum;
    actorNew->user.Radius = 100;
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    // zvel had to be tweaked alot for this weapon
    // MissileSetPos seemed to be pushing the sprite too far up or down when
    // the horizon was tilted.  Never figured out why.
    actorNew->spr.zvel = zvel >> 1;
    if (MissileSetPos(actorNew, DoStar, 1000))
    {
        KillActor(actorNew);
        return 0;
    }

    if (WeaponAutoAim(pp->actor, actorNew, 32, false) != -1)
    {
        zvel = actorNew->spr.zvel;
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    actorNew->backuppos();

    for (size_t i = 0; i < countof(dang); i++)
    {
        auto actorNew2 = SpawnActor(STAT_MISSILE, STAR1, s_Star, pp->cursector, nx, ny, nz, NORM_ANGLE(actorNew->int_ang() + dang[i]), actorNew->spr.xvel);

        SetOwner(GetOwner(actorNew), actorNew2);
        actorNew2->spr.yrepeat = actorNew2->spr.xrepeat = STAR_REPEAT;
        actorNew2->spr.shade = actorNew->spr.shade;

        actorNew2->spr.extra = actorNew->spr.extra;
        actorNew2->spr.clipdist = actorNew->spr.clipdist;
        actorNew2->user.WeaponNum = actorNew->user.WeaponNum;
        actorNew2->user.Radius = actorNew->user.Radius;
        actorNew2->user.ceiling_dist = actorNew->user.ceiling_dist;
        actorNew2->user.floor_dist = actorNew->user.floor_dist;
        actorNew2->user.Flags2 = actorNew->user.Flags2 & ~(SPR2_FLAMEDIE); // mask out any new flags here for safety.

        if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew2))
            actorNew2->user.Flags |= SPR_UNDERWATER;

        zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT+STAR_HORIZ_ADJ, 16);
        actorNew2->spr.zvel = zvel >> 1;

        if (MissileSetPos(actorNew2, DoStar, 1000))
        {
            KillActor(actorNew2);
            return 0;
        }

        // move the same as middle star
        zvel = actorNew->user.change.Z;

        actorNew2->user.change.X = MOVEx(actorNew2->spr.xvel, actorNew2->int_ang());
        actorNew2->user.change.Y = MOVEy(actorNew2->spr.xvel, actorNew2->int_ang());
        actorNew2->user.change.Z = zvel;

        actorNew2->backuppos();
    }

    return 0;
}

void InitHeartAttack(PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    short i = 0;

    static const MISSILE_PLACEMENT mp[] =
    {
        {0, 1100, 0},
    };

    PlayerUpdateAmmo(pp, WPN_HEART, -1);

    if (!pp->insector())
        return;

    auto actorNew = SpawnActor(STAT_MISSILE_SKIP4, BLOOD_WORM, s_BloodWorm, pp->cursector,
                            pp->int_ppos().X, pp->int_ppos().Y, pp->int_ppos().Z + Z(12), pp->angle.ang.Buildang(), BLOOD_WORM_VELOCITY*2);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    SetOwner(pp->actor, actorNew);
    actorNew->spr.shade = -10;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.yrepeat = 52;
    actorNew->spr.clipdist = 0;
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.Flags2 |= (SPR2_DONT_TARGET_OWNER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    actorNew->user.floor_dist = (1);
    actorNew->user.ceiling_dist = (1);
    actorNew->user.Dist = 200;

    auto oclipdist = plActor->spr.clipdist;
    plActor->spr.clipdist = 1;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoBloodWorm, mp[i].dist_out);

    plActor->spr.clipdist = oclipdist;
    actorNew->user.Counter = 0;
    actorNew->user.Counter2 = 0;
    actorNew->user.Counter3 = 0;
    actorNew->user.WaitTics = 0;
}

int ContinueHitscan(PLAYER* pp, sectortype* sect, int x, int y, int z, short ang, int xvect, int yvect, int zvect)
{
    HitInfo hit{};
    DSWActor* actor = pp->actor;

    FAFhitscan(x, y, z, sect,
               xvect, yvect, zvect,
               hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
        return 0;

    if (hit.actor() == nullptr && hit.hitWall == nullptr)
    {
        if (abs(hit.hitpos.Z - hit.hitSector->ceilingz) <= 1)
        {
            hit.hitpos.Z += 16;
            if ((hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY))
                return 0;
        }
        else if (abs(hit.hitpos.Z - hit.hitSector->floorz) <= 1)
        {
        }
    }

    if (hit.hitWall != nullptr)
    {
        if (hit.hitWall->twoSided())
        {
            if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
            {
                if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                {
                    return 0;
                }
            }
        }

        if (hit.hitWall->lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ang, actor->user.ID);
            return 0;
        }

        QueueHole(hit.hitSector, hit.hitWall, hit.hitpos);
    }

    // hit a sprite?
    if (hit.actor() != nullptr)
    {
        auto hitActor = hit.actor();

        if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(nullptr, WPN_SHOTGUN, hit.actor()))
                return 0;
        }

        if ((hitActor->spr.extra & SPRX_BREAKABLE))
        {
            HitBreakSprite(hit.actor(),0);
            return 0;
        }

        if (BulletHitSprite(pp->actor, hit.actor(), hit.hitpos, 0))
            return 0;

        // hit a switch?
        if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
        {
            ShootableSwitch(hit.actor());
        }
    }

    auto j = SpawnShotgunSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ang);
    DoHitscanDamage(j, hit.actor());

    return 0;
}

int InitShotgun(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    short daang,ndaang, i;
    HitInfo hit{};
    int daz, ndaz;
    int nx,ny,nz;
    int xvect,yvect,zvect;
    short cstat = 0;

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);

    PlaySound(DIGI_RIOTFIRE2, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (pp->WpnShotgunAuto)
    {
        switch (pp->WpnShotgunType)
        {
        case 1:
            pp->WpnShotgunAuto--;
        }
    }

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;
    daz = nz = pp->int_ppos().Z + pp->bob_z;

    daang = 64;
    if (WeaponAutoAimHitscan(pp->actor, &daz, &daang, false) != nullptr)
    {
    }
    else
    {
        daz = -MulScale(pp->horizon.horiz.asq16(), 2000, 16);
        daang = pp->angle.ang.Buildang();
    }

    for (i = 0; i < 12; i++)
    {
        if (pp->WpnShotgunType == 0)
        {
            ndaz = daz + (RandomRange(Z(120)) - Z(45));
            ndaang = NORM_ANGLE(daang + (RandomRange(30) - 15));
        }
        else
        {
            ndaz = daz + (RandomRange(Z(200)) - Z(65));
            ndaang = NORM_ANGLE(daang + (RandomRange(70) - 30));
        }

        xvect = bcos(ndaang);
        yvect = bsin(ndaang);
        zvect = ndaz;
        FAFhitscan(nx, ny, nz, pp->cursector,               // Start position
                   xvect, yvect, zvect,
                   hit, CLIPMASK_MISSILE);

        if (hit.hitSector == nullptr)
        {
            continue;
        }

        if (hit.actor() == nullptr && hit.hitWall == nullptr)
        {
            if (abs(hit.hitpos.Z - hit.hitSector->ceilingz) <= 1)
            {
                hit.hitpos.Z += 16;
                cstat |= (CSTAT_SPRITE_YFLIP);

                if ((hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY))
                    continue;

                if (SectorIsUnderwaterArea(hit.hitSector))
                {
                    WarpToSurface(hit.hitpos, &hit.hitSector);
                    ContinueHitscan(pp, hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ndaang, xvect, yvect, zvect);
                    continue;
                }
            }
            else if (abs(hit.hitpos.Z - hit.hitSector->floorz) <= 1)
            {
                if ((hit.hitSector->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
                {
                    SpawnSplashXY(hit.int_hitpos().X,hit.int_hitpos().Y,hit.int_hitpos().Z,hit.hitSector);

                    if (SectorIsDiveArea(hit.hitSector))
                    {
						WarpToUnderwater(hit.hitpos, &hit.hitSector);
                        ContinueHitscan(pp, hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ndaang, xvect, yvect, zvect);
                    }

                    continue;
                }
            }
        }

        if (hit.hitWall != nullptr)
        {
            if (hit.hitWall->twoSided())
            {
                if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
                {
                    if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                    {
                        continue;
                    }
                }
            }

            if (hit.hitWall->lotag == TAG_WALL_BREAK)
            {
                HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ndaang, actor->user.ID);
                continue;
            }

            QueueHole(hit.hitSector, hit.hitWall, hit.hitpos);
        }

        // hit a sprite?
        if (hit.actor() != nullptr)
        {
            auto hitActor = hit.actor();

            if (hitActor->hasU() && hitActor->user.ID == TRASHCAN)   // JBF: added null check
            {
                extern STATE s_TrashCanPain[];

                PlaySound(DIGI_TRASHLID, hitActor, v3df_none);
                if (hitActor->user.WaitTics <= 0)
                {
                    hitActor->user.WaitTics = SEC(2);
                    ChangeState(hitActor,s_TrashCanPain);
                }
            }

            if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
            {
                if (MissileHitMatch(nullptr, WPN_SHOTGUN, hitActor))
                    continue;
            }

            if ((hitActor->spr.extra & SPRX_BREAKABLE))
            {
                HitBreakSprite(hitActor,0);
                continue;
            }

            if (BulletHitSprite(pp->actor, hitActor, hit.hitpos, SHOTGUN_SMOKE))
                continue;

            // hit a switch?
            if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
            {
                ShootableSwitch(hitActor);
            }
        }

        auto j = SpawnShotgunSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, ndaang);
        DoHitscanDamage(j, hit.actor());
    }

    DoPlayerBeginRecoil(pp, SHOTGUN_RECOIL_AMT);
    return 0;
}

int InitLaser(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;

    DoPlayerBeginRecoil(pp, RAIL_RECOIL_AMT);
    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);
    PlaySound(DIGI_RIOTFIRE, pp, v3df_dontpan|v3df_doppler);

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    nz = pp->int_ppos().Z + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, s_Laser, pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), 300);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;
    actorNew->spr.clipdist = 64L>>2;

    // the slower the missile travels the less of a zvel it needs
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 11;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 900);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    // the slower the missile travels the less of a zvel it needs
    // move it 1200 dist in increments - works better
    if (MissileSetPos(actorNew, DoLaserStart, 300))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    if (MissileSetPos(actorNew, DoLaserStart, 300))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    if (MissileSetPos(actorNew, DoLaserStart, 300))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    if (MissileSetPos(actorNew, DoLaserStart, 300))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }

    actor->spr.clipdist = oclipdist;

    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 5));
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    return 0;
}



int InitRail(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int zvel;

    if (SW_SHAREWARE) return false; // JBF: verify

    DoPlayerBeginRecoil(pp, RAIL_RECOIL_AMT);

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);

    PlaySound(DIGI_RAILFIRE, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    nz = pp->int_ppos().Z + pp->bob_z + Z(11);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), 1200);


    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;
    zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT + 17, 16);

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rail[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = RAIL_RADIUS;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;
    actorNew->spr.clipdist = 32L>>2;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 700);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    if (TestMissileSetPos(actorNew, DoRailStart, 1200, zvel))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 4));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    return 0;
}

int InitZillaRail(DSWActor* actor)
{
    int nx, ny, nz;
    int zvel;

    if (SW_SHAREWARE) return false; // JBF: verify

    PlaySound(DIGI_RAILFIRE, actor, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;

    nz = ActorZOfTop(actor);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], actor->sector(),
                    nx, ny, nz, actor->int_ang(), 1200);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;
    zvel = (100 * (HORIZ_MULT+17));

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rail[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = RAIL_RADIUS;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;
    actorNew->spr.clipdist = 32L>>2;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 700);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    if (TestMissileSetPos(actorNew, DoRailStart, 1200, zvel))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 4));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    return 0;
}

int InitRocket(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int zvel;

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT);

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);
    auto const WpnRocketHeat = pp->WpnRocketHeat;
    if (WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            pp->WpnRocketHeat--;
            break;
        }
    }

    PlaySound(DIGI_RIOTFIRE, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    // Spawn a shot
    // Inserting and setting up variables

    nz = pp->int_ppos().Z + pp->bob_z + Z(8);
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), ROCKET_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 90;
    actorNew->spr.xrepeat = 90;
    actorNew->spr.shade = -15;
    zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT + 35, 16);

    actorNew->spr.clipdist = 64L>>2;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rocket[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 2000;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // Set default palette
    actorNew->spr.pal = actorNew->user.spal = 17; // White

    if (WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            actorNew->user.Flags |= (SPR_FIND_PLAYER);
            actorNew->spr.pal = actorNew->user.spal = 20; // Yellow
            break;
        }
    }

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 900);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    // cancel smoke trail
    actorNew->user.Counter = 1;
    if (TestMissileSetPos(actorNew, DoRocket, 1200, zvel))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    // inable smoke trail
    actorNew->user.Counter = 0;

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 5));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    return 0;
}

int InitBunnyRocket(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int zvel;

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT);

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);
    auto const WpnRocketHeat = pp->WpnRocketHeat;
    if (WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            pp->WpnRocketHeat--;
            break;
        }
    }

    PlaySound(DIGI_BUNNYATTACK, pp, v3df_dontpan|v3df_doppler);

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    // Spawn a shot
    // Inserting and setting up variables

    //nz = pp->posz + pp->bob_z + Z(12);
    nz = pp->int_ppos().Z + pp->bob_z + Z(8);
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R4, &s_BunnyRocket[0][0], pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), ROCKET_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 64;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.shade = -15;
    zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT + 35, 16);

    actorNew->spr.clipdist = 64L>>2;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_BunnyRocket[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 2000;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    if (WpnRocketHeat)
    {
        switch (pp->WpnRocketType)
        {
        case 1:
            actorNew->user.Flags |= (SPR_FIND_PLAYER);
            break;
        }
    }

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 900);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    // cancel smoke trail
    actorNew->user.Counter = 1;
    if (TestMissileSetPos(actorNew, DoRocket, 1200, zvel))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    // inable smoke trail
    actorNew->user.Counter = 0;

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 5));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;
    actorNew->user.spal = actorNew->spr.pal = PALETTE_PLAYER1;

    return 0;
}

int InitNuke(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int zvel;

    if (pp->WpnRocketNuke > 0)
        pp->WpnRocketNuke = 0;  // Bye Bye little nukie.
    else
        return 0;

    DoPlayerBeginRecoil(pp, ROCKET_RECOIL_AMT*12);


    PlaySound(DIGI_RIOTFIRE, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    // Spawn a shot
    // Inserting and setting up variables

    nz = pp->int_ppos().Z + pp->bob_z + Z(8);
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), 700);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 128;
    actorNew->spr.xrepeat = 128;
    actorNew->spr.shade = -15;
    zvel = -MulScale(pp->horizon.horiz.asq16(), HORIZ_MULT + 36, 16);
    actorNew->spr.clipdist = 64L>>2;

    // Set to red palette
    actorNew->spr.pal = actorNew->user.spal = 19;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rocket[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = NUKE_RADIUS;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 900);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    // cancel smoke trail
    actorNew->user.Counter = 1;
    if (TestMissileSetPos(actorNew, DoRocket, 1200, zvel))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }
    // inable smoke trail
    actorNew->user.Counter = 0;

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 5));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    PlayerDamageSlide(pp, -40, NORM_ANGLE(pp->angle.ang.Buildang()+1024)); // Recoil slide

    return 0;
}

int InitEnemyNuke(DSWActor* actor)
{
    int nx, ny, nz;
    int zvel;

    PlaySound(DIGI_RIOTFIRE, actor, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;

    // Spawn a shot
    nz = actor->int_pos().Z + Z(40);
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], actor->sector(),
                    nx, ny, nz, actor->int_ang(), 700);

    if (actor->user.ID == ZOMBIE_RUN_R0)
        SetOwner(GetOwner(actor), actorNew);
    else
        SetOwner(actor, actorNew);

    actorNew->spr.yrepeat = 128;
    actorNew->spr.xrepeat = 128;
    actorNew->spr.shade = -15;
    zvel = (100 * (HORIZ_MULT-36));
    actorNew->spr.clipdist = 64L>>2;

    // Set to red palette
    actorNew->spr.pal = actorNew->user.spal = 19;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rocket[0]);

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = NUKE_RADIUS;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 500);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    // cancel smoke trail
    actorNew->user.Counter = 1;
    if (TestMissileSetPos(actorNew, DoRocket, 1200, zvel))
    {
        KillActor(actorNew);
        return 0;
    }

    // enable smoke trail
    actorNew->user.Counter = 0;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAim(actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 5));
    }
    else
        zvel = actorNew->spr.zvel;  // Let autoaiming set zvel now

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    return 0;
}

int InitMicro(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz, dist;
    short i,ang;
    TARGET_SORT* ts = TargetSort;
    DSWActor* picked = nullptr;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;

    const int MAX_MICRO = 1;

    DoPickTarget(pp->actor, 256, false);

    if (TargetSortCount > MAX_MICRO)
        TargetSortCount = MAX_MICRO;

    if (!pp->insector())
        return 0;

    for (i = 0; i < MAX_MICRO; i++)
    {
        if (ts < &TargetSort[TargetSortCount] && ts->actor != nullptr)
        {
            picked = ts->actor;

            ang = getangle(picked->int_pos().X - nx, picked->int_pos().Y - ny);

            ts++;
        }
        else
        {
            picked = nullptr;
            ang = pp->angle.ang.Buildang();
        }

        nz = pp->int_ppos().Z + pp->bob_z + Z(14);
        nz += Z(RandomRange(20)) - Z(10);

        // Spawn a shot
        // Inserting and setting up variables

        auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], pp->cursector,
                        nx, ny, nz, ang, 1200);

        SetOwner(pp->actor, actorNew);
        actorNew->spr.yrepeat = 24;
        actorNew->spr.xrepeat = 24;
        actorNew->spr.shade = -15;
        actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
        actorNew->spr.clipdist = 64L>>2;

        // randomize zvelocity
        actorNew->spr.zvel += RandomRange(Z(8)) - Z(5);

        actorNew->user.RotNum = 5;
        NewStateGroup(actorNew, &sg_Micro[0]);

        actorNew->user.WeaponNum = actor->user.WeaponNum;
        actorNew->user.Radius = 200;
        actorNew->user.ceiling_dist = (2);
        actorNew->user.floor_dist = (2);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        actorNew->user.WaitTics = 10 + RandomRange(40);

        // at certain angles the clipping box was big enough to block the
        // initial positioning of the fireball.
        auto oclipdist = actor->spr.clipdist;
        actor->spr.clipdist = 0;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
        const int MICRO_LATERAL = 5000;
        HelpMissileLateral(actorNew, 1000 + (RandomRange(MICRO_LATERAL) - (MICRO_LATERAL / 2)));
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

        if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
            actorNew->user.Flags |= (SPR_UNDERWATER);

        // cancel smoke trail
        actorNew->user.Counter = 1;
        if (MissileSetPos(actorNew, DoMicro, 700))
        {
            actor->spr.clipdist = oclipdist;
            KillActor(actorNew);
            continue;
        }
        // inable smoke trail
        actorNew->user.Counter = 0;

        actor->spr.clipdist = oclipdist;

        const int MICRO_ANG = 400;

        if (picked)
        {
			dist = DistanceI(actorNew->spr.pos, picked->spr.pos);
            if (dist != 0)
            {
                int zh;
                zh = ActorZOfTop(picked) + (ActorSizeZ(picked) >> 2);
                actorNew->spr.zvel = (actorNew->spr.xvel * (zh - actorNew->int_pos().Z)) / dist;
            }

            actorNew->user.WpnGoalActor = ts->actor;
            picked->user.Flags |= (SPR_TARGETED);
            picked->user.Flags |= (SPR_ATTACKED);
        }
        else
        {
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + (RandomRange(MICRO_ANG) - (MICRO_ANG / 2)) - 16));
        }

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;
    }

    return 0;
}

int InitRipperSlash(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_RIPPER2ATTACK, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            if ((unsigned)FindDistance3D(actor->int_pos() - itActor->int_pos()) > itActor->user.Radius + actor->user.Radius)
                continue;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 600) && FacingRange(itActor, actor,150))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int InitBunnySlash(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_BUNNYATTACK, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 600) && FacingRange(itActor, actor,150))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}


int InitSerpSlash(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_SERPSWORDATTACK, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 800) && FacingRange(itActor, actor,150))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

bool WallSpriteInsideSprite(DSWActor* wactor, DSWActor* actor)
{
    DVector2 out[2];
    GetWallSpritePosition(&wactor->spr, wactor->spr.pos, out);
    return IsCloseToLine(actor->spr.pos.XY(), out[0], out[1], (((int) actor->spr.clipdist) << 2) * inttoworld) != EClose::Outside;
}


int DoBladeDamage(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist > 2000)
                continue;

            dist = FindDistance3D(actor->int_pos() - itActor->int_pos());

            if (dist > 2000)
                continue;

            if (WallSpriteInsideSprite(actor, itActor))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int DoStaticFlamesDamage(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist > 2000)
                continue;

            dist = FindDistance3D(actor->int_pos() - itActor->int_pos());

            if (dist > 2000)
                continue;

            if (SpriteOverlap(actor, itActor))  // If sprites are overlapping, cansee will fail!
                DoDamage(itActor, actor);
            else if (actor->user.Radius > 200)
            {
                if (FAFcansee(actor->int_pos().X,actor->int_pos().Y,int_ActorZOfMiddle(actor),actor->sector(),itActor->int_pos().X,itActor->int_pos().Y,int_ActorZOfMiddle(itActor),itActor->sector()))
                    DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int InitCoolgBash(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_CGTHIGHBONE, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            // don't set off mine
            if (!(itActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
                continue;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 600) && FacingRange(itActor, actor,150))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int InitSkelSlash(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_SPBLADE, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 600) && FacingRange(itActor, actor,150))
            {
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int InitGoroChop(DSWActor* actor)
{
    int i;
    unsigned stat;
    int dist, a, b, c;

    PlaySound(DIGI_GRDSWINGAXE, actor, v3df_none);

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                break;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 700) && FacingRange(itActor, actor,150))
            {
                PlaySound(DIGI_GRDAXEHIT, actor, v3df_none);
                DoDamage(itActor, actor);
            }
        }
    }

    return 0;
}

int InitHornetSting(DSWActor* actor)
{
    DoDamage(actor->user.coll.actor(), actor);
    InitActorReposition(actor);
    return 0;
}

int InitSerpSpell(DSWActor* actor)
{
    int dist;
    short i;

    static const short lat_ang[] =
    {
        512, -512
    };

    static const short delta_ang[] =
    {
        -10, 10
    };

    for (i = 0; i < 2; i++)
    {
        actor->set_int_ang(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

        auto actorNew = SpawnActor(STAT_MISSILE, SERP_METEOR, &sg_SerpMeteor[0][0], actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 1500);

        actorNew->set_int_z(ActorZOfTop(actor));

        actorNew->user.RotNum = 5;
        NewStateGroup(actorNew, &sg_SerpMeteor[0]);
        actorNew->user.StateEnd = s_MirvMeteorExp;

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        PlaySound(DIGI_SERPMAGICLAUNCH, actor, v3df_none);
        actorNew->user.spal = actorNew->spr.pal = 27; // Bright Green
        actorNew->spr.xrepeat = 64;
        actorNew->spr.yrepeat = 64;
        actorNew->spr.clipdist = 32L >> 2;
        actorNew->spr.zvel = 0;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

        actorNew->user.ceiling_dist = (16);
        actorNew->user.floor_dist = (16);
        actorNew->user.Dist = 200;

        auto oclipdist = actor->spr.clipdist;
        actor->spr.clipdist = 1;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + lat_ang[i]));
        HelpMissileLateral(actorNew, 4200);
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - lat_ang[i]));

        // find the distance to the target (player)
		dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);
        if (dist != 0)
            actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + delta_ang[i]));

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;

        MissileSetPos(actorNew, DoMirvMissile, 400);
        actor->spr.clipdist = oclipdist;

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return 0;
}

int SpawnDemonFist(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_SUICIDE))
        return -1;

    auto expActor = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(),
                            actor->int_pos().X, actor->int_pos().Y, int_ActorZOfMiddle(actor), actor->int_ang(), 0);

    expActor->spr.hitag = LUMINOUS; //Always full brightness
    expActor->spr.shade = -40;
    expActor->spr.xrepeat = 32;
    expActor->spr.yrepeat = 32;
    expActor->user.spal = expActor->spr.pal = 25;

    expActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    expActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    expActor->user.Radius = DamageData[DMG_BASIC_EXP].radius;

    if (RANDOM_P2(1024<<8)>>8 > 600)
        expActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024<<8)>>8 > 600)
        expActor->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    return 0;
}

int InitSerpMonstSpell(DSWActor* actor)
{
    int dist;
    short i;

    static const short lat_ang[] =
    {
        512, -512
    };

    static const short delta_ang[] =
    {
        -10, 10
    };

    PlaySound(DIGI_MISSLFIRE, actor, v3df_none);

    for (i = 0; i < 1; i++)
    {
        actor->set_int_ang(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

        auto actorNew = SpawnActor(STAT_MISSILE, SERP_METEOR, &sg_SerpMeteor[0][0], actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 500);

        actorNew->user.spal = actorNew->spr.pal = 25; // Bright Red
        actorNew->set_int_z(ActorZOfTop(actor));

        actorNew->user.RotNum = 5;
        NewStateGroup(actorNew, &sg_SerpMeteor[0]);
        actorNew->user.StateEnd = s_TeleportEffect2;

        SetOwner(actor, actorNew);
        actorNew->spr.shade = -40;
        actorNew->spr.xrepeat = 122;
        actorNew->spr.yrepeat = 116;
        actorNew->spr.clipdist = 32L >> 2;
        actorNew->spr.zvel = 0;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

        actorNew->user.ceiling_dist = (16);
        actorNew->user.floor_dist = (16);

        actorNew->user.Dist = 200;

        auto oclipdist = actor->spr.clipdist;
        actor->spr.clipdist = 1;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + lat_ang[i]));
        HelpMissileLateral(actorNew, 4200);
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - lat_ang[i]));

        // find the distance to the target (player)
		dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);
        if (dist != 0)
            actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + delta_ang[i]));

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;

        MissileSetPos(actorNew, DoMirvMissile, 400);
        actor->spr.clipdist = oclipdist;

        if (actor->user.Flags & (SPR_UNDERWATER))
            actorNew->user.Flags |= (SPR_UNDERWATER);
    }

    return 0;
}

int DoTeleRipper(DSWActor* actor)
{
    PlaySound(DIGI_ITEM_SPAWN, actor, v3df_none);
    Ripper2Hatch(actor);

    return 0;
}


int InitEnemyRocket(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;

    PlaySound(DIGI_NINJARIOTATTACK, actor, v3df_none);

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - (ActorSizeZ(actor) >> 1)-Z(8);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R2, &s_Rocket[0][0], actor->sector(),
                    nx, ny, nz-Z(8), actor->user.targetActor->int_ang(), NINJA_BOLT_VELOCITY);

    // Set default palette
    actorNew->spr.pal = actorNew->user.spal = 17; // White

    if (actor->user.ID == ZOMBIE_RUN_R0)
        SetOwner(GetOwner(actor), actorNew);
    else
        SetOwner(actor, actorNew);

    actorNew->spr.yrepeat = 28;
    actorNew->spr.xrepeat = 28;
    actorNew->spr.shade = -15;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);
    actorNew->spr.clipdist = 64L>>2;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rocket[0]);
    actorNew->user.Radius = 200;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (actor->user.spal == PAL_XLAT_LT_TAN)
    {
        actorNew->user.Flags |= (SPR_FIND_PLAYER);
        actorNew->spr.pal = actorNew->user.spal = 20; // Yellow
    }

    MissileSetPos(actorNew, DoBoltThinMan, 400);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

    return 0;
}

int InitEnemyRail(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;
    short pnum=0;

    if (SW_SHAREWARE) return false; // JBF: verify

    // if co-op don't hurt teammate
    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE && actor->user.ID == ZOMBIE_RUN_R0)
    {
        PLAYER* pp;

        // Check all players
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];
            if (actor->user.targetActor == pp->actor)
                return 0;
        }
    }

    PlaySound(DIGI_RAILFIRE, actor, v3df_dontpan|v3df_doppler);

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

    // add a bit of randomness
    if (RANDOM_P2(1024) < 512)
        actor->set_int_ang(NORM_ANGLE(actor->int_ang() + RANDOM_P2(128) - 64));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - (ActorSizeZ(actor) >> 1)-Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], actor->sector(),
                    nx, ny, nz, actor->int_ang(), 1200);

    if (actor->user.ID == ZOMBIE_RUN_R0)
        SetOwner(GetOwner(actor), actorNew);
    else
        SetOwner(actor, actorNew);

    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;
    actorNew->spr.zvel = 0;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rail[0]);

    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->spr.clipdist = 64 >> 2;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (TestMissileSetPos(actorNew, DoRailStart, 600, actorNew->spr.zvel))
    {
        KillActor(actorNew);
        return 0;
    }

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

    return 0;
}


int InitZillaRocket(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;
    short w, i;

    static const MISSILE_PLACEMENT mp[] =
    {
        {600 * 6, 400, 512},
        {900 * 6, 400, 512},
        {1100 * 6, 400, 512},
        {600 * 6, 400, -512},
        {900 * 6, 400, -512},
        {1100 * 6, 400, -512},
    };

    PlaySound(DIGI_NINJARIOTATTACK, actor, v3df_none);

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y));

    for (i = 0; i < (int)SIZ(mp); i++)
    {
        nx = actor->int_pos().X;
        ny = actor->int_pos().Y;
        nz = actor->int_pos().Z - (ActorSizeZ(actor) >> 1)-Z(8);

        // Spawn a shot
        auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R2, &s_Rocket[0][0], actor->sector(),
                        nx, ny, nz-Z(8), actor->user.targetActor->int_ang(), NINJA_BOLT_VELOCITY);

        SetOwner(actor, actorNew);
        actorNew->spr.yrepeat = 28;
        actorNew->spr.xrepeat = 28;
        actorNew->spr.shade = -15;
        actorNew->spr.zvel = 0;
        actorNew->set_int_ang(nang);
        actorNew->spr.clipdist = 64 >>2;

        actorNew->user.RotNum = 5;
        NewStateGroup(actorNew, &sg_Rocket[0]);
        actorNew->user.Radius = 200;
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;

        // Zilla has seekers!
        if (i != 1 && i != 4)
            actorNew->spr.pal = actorNew->user.spal = 17; // White
        else
        {
            actorNew->user.Flags |= (SPR_FIND_PLAYER);
            actorNew->spr.pal = actorNew->user.spal = 20; // Yellow
        }

        if (mp[i].dist_over != 0)
        {
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + mp[i].ang));
            HelpMissileLateral(actorNew, mp[i].dist_over);
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - mp[i].ang));
        }

        MissileSetPos(actorNew, DoBoltThinMan, mp[i].dist_out);

        // find the distance to the target (player)
		dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

        if (dist != 0)
            actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;
    }

    return 0;
}

int InitEnemyStar(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = NORM_ANGLE(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y)));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = int_ActorZOfMiddle(actor);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, STAR1, s_Star, actor->sector(),
                                 nx, ny, nz, actor->user.targetActor->int_ang(), NINJA_STAR_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 16;
    actorNew->spr.xrepeat = 16;
    actorNew->spr.shade = -25;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);
    actorNew->spr.clipdist = 64L>>2;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoStar, 400);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

    PlaySound(DIGI_STAR, actor, v3df_none);
    return 0;
}

int InitEnemyCrossbow(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = NORM_ANGLE(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y)));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = int_ActorZOfMiddle(actor)-Z(14);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, CROSSBOLT, &s_CrossBolt[0][0], actor->sector(),
                                 nx, ny, nz, actor->user.targetActor->int_ang(), 800);

    SetOwner(actor, actorNew);
    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 26;
    actorNew->spr.shade = -25;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);
    actorNew->spr.clipdist = 64>>2;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_CrossBolt[0]);

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    MissileSetPos(actorNew, DoStar, 400);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->user.change.Z = actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - actorNew->int_pos().Z)) / dist;

    PlaySound(DIGI_STAR, actor, v3df_none);

    return 0;
}


int InitSkelSpell(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;

    PlaySound(DIGI_SPELEC, actor, v3df_none);

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = NORM_ANGLE(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y)));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - (ActorSizeZ(actor) >> 1);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, ELECTRO_ENEMY, s_Electro, actor->sector(),
                    nx, ny, nz, actor->user.targetActor->int_ang(), SKEL_ELECTRO_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.xrepeat -= 20;
    actorNew->spr.yrepeat -= 20;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);
    actorNew->spr.clipdist = 64L>>2;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - nz)) / dist;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    MissileSetPos(actorNew, DoElectro, 400);

    return 0;
}


int InitCoolgFire(DSWActor* actor)
{
    int nx, ny, nz, dist, nang;

    // get angle to player and also face player when attacking
    actor->set_int_ang(nang = NORM_ANGLE(getangle(actor->user.targetActor->int_pos().X - actor->int_pos().X, actor->user.targetActor->int_pos().Y - actor->int_pos().Y)));

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;

    nz = actor->int_pos().Z - Z(16);

    // Spawn a shot
    // Inserting and setting up variables

    PlaySound(DIGI_CGMAGIC, actor, v3df_follow);

    auto actorNew = SpawnActor(STAT_MISSILE, COOLG_FIRE, s_CoolgFire, actor->sector(),
                    nx, ny, nz, actor->user.targetActor->int_ang(), COOLG_FIRE_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.hitag = LUMINOUS;
    actorNew->spr.yrepeat = 18;
    actorNew->spr.xrepeat = 18;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->set_int_ang(nang);
    actorNew->spr.clipdist = 32>>2;
    actorNew->user.ceiling_dist = (4);
    actorNew->user.floor_dist = (4);
    if (actor->user.ID == RIPPER_RUN_R0)
        actorNew->user.spal = actorNew->spr.pal = 27; // Bright Green
    else
        actorNew->user.spal = actorNew->spr.pal = 25; // Bright Red

    PlaySound(DIGI_MAGIC1, actorNew, v3df_follow|v3df_doppler);

    // find the distance to the target (player)
	dist = DistanceI(actorNew->spr.pos, actor->user.targetActor->spr.pos);

    if (dist != 0)
        // (velocity * difference between the target and the throwing star) /
        // distance
        actorNew->spr.zvel = (actorNew->spr.xvel * (ActorUpperZ(actor->user.targetActor) - nz)) / dist;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    nx = MulScale(728, bcos(nang), 14);
    ny = MulScale(728, bsin(nang), 14);

    move_missile(actorNew, nx, ny, 0, actorNew->user.int_ceiling_dist(), actorNew->user.int_floor_dist(), 0, 3);

    return 0;
}

int DoCoolgDrip(DSWActor* actor)
{
    actor->user.Counter += 220;
    actor->add_int_z(actor->user.Counter);

    if (actor->int_pos().Z > actor->user.int_loz() - actor->user.int_floor_dist())
    {
        actor->set_int_z(actor->user.int_loz() - actor->user.int_floor_dist());
        actor->spr.yrepeat = actor->spr.xrepeat = 32;
        ChangeState(actor, s_GoreFloorSplash);
        if (actor->user.spal == PALETTE_BLUE_LIGHTING)
            PlaySound(DIGI_DRIP, actor, v3df_none);
    }
    return 0;
}

int InitCoolgDrip(DSWActor* actor)
{
    int nx, ny, nz;
    short w;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z;

    auto actorNew = SpawnActor(STAT_MISSILE, COOLG_DRIP, s_CoolgDrip, actor->sector(),
                    nx, ny, nz, actor->int_ang(), 0);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = actorNew->spr.xrepeat = 20;
    actorNew->spr.shade = -5;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 16L>>2;
    actorNew->user.ceiling_dist = (4);
    actorNew->user.floor_dist = (4);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    DoFindGroundPoint(actor);

    return 0;
}

int GenerateDrips(DSWActor* actor)
{
    int nx, ny, nz;
    short w = 0;

    if ((actor->user.WaitTics-=ACTORMOVETICS) <= 0)
    {
        if (actor->spr.lotag == 0)
            actor->user.WaitTics = RANDOM_P2(256<<8)>>8;
        else
            actor->user.WaitTics = (actor->spr.lotag * 120) + SEC(RandomRange(3<<8)>>8);

        if (TEST_BOOL2(actor))
        {
            auto ww = SpawnBubble(actor);
            return 1;
        }

        nx = actor->int_pos().X;
        ny = actor->int_pos().Y;
        nz = actor->int_pos().Z;

        auto actorNew = SpawnActor(STAT_SHRAP, COOLG_DRIP, s_CoolgDrip, actor->sector(),
                        nx, ny, nz, actor->int_ang(), 0);

        SetOwner(actor, actorNew);
        actorNew->spr.yrepeat = actorNew->spr.xrepeat = 20;
        actorNew->spr.shade = -10;
        actorNew->spr.zvel = 0;
        actorNew->spr.clipdist = 16L>>2;
        actorNew->user.ceiling_dist = (4);
        actorNew->user.floor_dist = (4);
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        if (TEST_BOOL1(actor))
            actorNew->user.spal = actorNew->spr.pal = PALETTE_BLUE_LIGHTING;

        DoFindGroundPoint(actor);
    }
    return 1;
}

int InitEelFire(DSWActor* actor)
{
    unsigned stat;
    int dist, a, b, c;

    for (stat = 0; stat < SIZ(StatDamageList); stat++)
    {
        SWStatIterator it(StatDamageList[stat]);
        while (auto itActor = it.Next())
        {
            if (itActor == actor)
                continue;

            if (itActor != actor->user.targetActor)
                continue;

            if ((unsigned)FindDistance3D(actor->int_pos() - itActor->int_pos()) > itActor->user.Radius + actor->user.Radius)
                continue;

            DISTANCE(itActor->int_pos().X, itActor->int_pos().Y, actor->int_pos().X, actor->int_pos().Y, dist, a, b, c);

            if (dist < CloseRangeDist(actor, itActor, 600) && FacingRange(itActor, actor,150))
            {
                PlaySound(DIGI_GIBS1, actor, v3df_none);
                DoDamage(itActor, actor);
            }
            else
                InitActorReposition(actor);
        }
    }

    return 0;
}

void InitFireballTrap(DSWActor* actor)
{
    int nx, ny, nz;
    short w;

    PlaySound(DIGI_FIREBALL1, actor, v3df_none);

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - ActorSizeZ(actor);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL, s_Fireball, actor->sector(), nx, ny, nz,
                    actor->int_ang(), FIREBALL_TRAP_VELOCITY);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(actor, actorNew);
    actorNew->spr.xrepeat -= 20;
    actorNew->spr.yrepeat -= 20;
    actorNew->spr.shade = -40;
    actorNew->spr.clipdist = 32>>2;
    actorNew->spr.zvel = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->user.WeaponNum = WPN_HOTHEAD;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;
}

void InitBoltTrap(DSWActor* actor)
{
    int nx, ny, nz;
    short w;

    PlaySound(DIGI_RIOTFIRE, actor, v3df_none);

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - ActorSizeZ(actor);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], actor->sector(), nx, ny, nz,
                    actor->int_ang(), BOLT_TRAP_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->spr.zvel = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rocket[0]);
    actorNew->user.Radius = 200;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;
}


void InitSpearTrap(DSWActor* actor)
{
    int nx, ny, nz;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = int_ActorZOfMiddle(actor);

    // Spawn a shot
    auto actorNew = SpawnActor(STAT_MISSILE, CROSSBOLT, &s_CrossBolt[0][0], actor->sector(), nx, ny, nz, actor->int_ang(), 750);

    SetOwner(actor, actorNew);
    actorNew->spr.xrepeat = 16;
    actorNew->spr.yrepeat = 26;
    actorNew->spr.shade = -25;
    actorNew->spr.clipdist = 64L>>2;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_CrossBolt[0]);

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    PlaySound(DIGI_STAR, actor, v3df_none);
}

int DoSuicide(DSWActor* actor)
{
    KillActor(actor);
    return 0;
}

int DoDefaultStat(DSWActor* actor)
{
    change_actor_stat(actor, STAT_DEFAULT);
    return 0;
}


int InitTracerUzi(PLAYER* pp)
{
    if (!pp->insector())
        return 0;

    DSWActor* actor = pp->actor;

    int nx, ny, nz;
    int oclipdist;

    short lat_dist[] = {800,-800};

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;
    nz = pp->int_ppos().Z + Z(8) + -MulScale(pp->horizon.horiz.asq16(), 72, 16);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, 0, s_Tracer, pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), TRACER_VELOCITY);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 10;
    actorNew->spr.xrepeat = 10;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    //actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
    actorNew->spr.clipdist = 32 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    DSWActor* plActor = pp->actor;
    oclipdist = plActor->spr.clipdist;
    plActor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    if (pp->Flags & (PF_TWO_UZI) && pp->WpnUziType == 0)
        HelpMissileLateral(actorNew, lat_dist[RANDOM_P2(2<<8)>>8]);
    else
        HelpMissileLateral(actorNew, lat_dist[0]);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (MissileSetPos(actorNew, DoTracerStart, 800))
    {
        plActor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }

    actorNew->spr.zvel = int(-pp->horizon.horiz.asbuildf() * actorNew->spr.xvel * (1. / 8.));

    plActor->spr.clipdist = oclipdist;

    WeaponAutoAim(pp->actor, actorNew, 32, false);

    // a bit of randomness
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + RandomRange(30) - 15));

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    return 0;
}

int InitTracerTurret(DSWActor* actor, DSWActor* Operator, fixed_t q16horiz)
{
    int nx, ny, nz;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z + -MulScale(q16horiz, 72, 16);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, 0, s_Tracer, actor->sector(),
                    nx, ny, nz, actor->int_ang(), TRACER_VELOCITY);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    if (Operator!= nullptr)
        SetOwner(Operator, actorNew);
    actorNew->spr.yrepeat = 10;
    actorNew->spr.xrepeat = 10;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 8 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    actorNew->spr.zvel = int(-MulScaleF(q16horiz, actorNew->spr.xvel * (1. / 8.), 16));

    WeaponAutoAim(actor, actorNew, 32, false);

    // a bit of randomness
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + RandomRange(30) - 15));

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    return 0;
}

int InitTracerAutoTurret(DSWActor* actor, int xchange, int ychange, int zchange)
{
    int nx, ny, nz;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z;

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, 0, s_Tracer, actor->sector(),
                    nx, ny, nz, actor->int_ang(), TRACER_VELOCITY);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    actorNew->spr.yrepeat = 10;
    actorNew->spr.xrepeat = 10;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 8 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    actorNew->user.change.X = xchange;
    actorNew->user.change.Y = ychange;
    actorNew->user.change.Z = zchange;

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    return 0;
}

int BulletHitSprite(DSWActor* actor, DSWActor* hitActor, const DVector3& hit_pos, short ID)
{
    short id;

    // hit a NPC or PC?
    if ((hitActor->spr.extra & SPRX_PLAYER_OR_ENEMY))
    {
        // spawn a red splotch
        // !FRANK! this if was incorrect - its not who is HIT, its who is SHOOTING
        //if(!hitActor->user.PlayerP)
        if (actor->user.PlayerP)
            id = UZI_SMOKE;
        else if (actor->user.Flags & (SPR_SO_ATTACHED))
            id = UZI_SMOKE;
        else // Spawn NPC uzi with less damage
            id = UZI_SMOKE+2;

        if (ID>0) id = ID;

        auto actorNew = SpawnActor(STAT_MISSILE, id, s_UziSmoke, &sector[0], hit_pos, actor->spr.angle, 0);
        actorNew->spr.shade = -40;

        if (hitActor->user.PlayerP)
            actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        switch (hitActor->user.ID)
        {
        case TRASHCAN:
        case PACHINKO1:
        case PACHINKO2:
        case PACHINKO3:
        case PACHINKO4:
        case 623:
        case ZILLA_RUN_R0:
            actorNew->spr.xrepeat = UZI_SMOKE_REPEAT;
            actorNew->spr.yrepeat = UZI_SMOKE_REPEAT;
            if (RANDOM_P2(1024) > 800)
                SpawnShrapX(hitActor);
            break;
        default:
            actorNew->spr.xrepeat = UZI_SMOKE_REPEAT/3;
            actorNew->spr.yrepeat = UZI_SMOKE_REPEAT/3;
            actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
            //actorNew->user.spal = actorNew->spr.pal = PALETTE_RED_LIGHTING;
            break;
        }

        SetOwner(actor, actorNew);
        actorNew->spr.angle = actor->spr.angle;

        SetActorZ(actorNew, hit_pos);
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

		int hit_x = int(hit_pos.X * worldtoint);
		int hit_y = int(hit_pos.Y * worldtoint);
		int hit_z = int(hit_pos.Z * zworldtoint);
        if ((RANDOM_P2(1024<<5)>>5) < 512+128)
        {
            if (!hitActor->user.PlayerP)
                SpawnBlood(hitActor, nullptr, actor->int_ang(), hit_x, hit_y, hit_z);
            else
                SpawnBlood(hitActor, nullptr, actor->int_ang(), hit_x, hit_y, hit_z+Z(20));

            // blood comes out the other side?
            if ((RANDOM_P2(1024<<5)>>5) < 256)
            {
                if (!hitActor->user.PlayerP)
                    SpawnBlood(hitActor, nullptr, NORM_ANGLE(actor->int_ang() + 1024),hit_x, hit_y, hit_z);
                if (hitActor->user.ID != TRASHCAN && hitActor->user.ID != ZILLA_RUN_R0)
                    QueueWallBlood(hitActor, actor->int_ang());  //QueueWallBlood needs bullet angle.
            }
        }

        DoHitscanDamage(actorNew, hitActor);

        return true;
    }

    return false;
}

bool HitscanSpriteAdjust(DSWActor* actor, walltype* hit_wall)
{
    int16_t ang;
    int xvect,yvect;

    if (hit_wall)
    {
        int16_t const wall_ang = NORM_ANGLE(getangle(hit_wall->delta()));
        actor->set_int_ang(NORM_ANGLE(wall_ang + 512));
    }
    ang = actor->int_ang();

    xvect = bcos(ang, 4);
    yvect = bsin(ang, 4);

    // must have this
    auto sect = actor->sector();

    Collision coll;
    clipmove(actor->spr.pos, &sect, xvect, yvect, 4, 4 << 8, 4 << 8, CLIPMASK_MISSILE, coll);

    if (actor->sector() != sect)
        ChangeActorSect(actor, sect);

    return true;
}

int InitUzi(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    short daang;
    HitInfo hit{};
    int daz, nz;
    int xvect,yvect,zvect;
    ESpriteFlags cstat = 0;
    uint8_t pal = 0;
    //static char alternate=0;
    static int uziclock=0;
    int clockdiff=0;
    bool FireSnd = false;
    const int UZIFIRE_WAIT = 20;

    void InitUziShell(PLAYER*);


    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);

    if (uziclock > PlayClock)
    {
        uziclock = PlayClock;
        FireSnd = true;
    }

    clockdiff = PlayClock - uziclock;
    if (clockdiff > UZIFIRE_WAIT)
    {
        uziclock = PlayClock;
        FireSnd = true;
    }

    if (FireSnd)
        PlaySound(DIGI_UZIFIRE, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (RANDOM_P2(1024) < 400)
        InitTracerUzi(pp);

    nz = pp->int_ppos().Z + pp->bob_z;
    daz = pp->int_ppos().Z + pp->bob_z;
    daang = 32;
    if (WeaponAutoAimHitscan(pp->actor, &daz, &daang, false) != nullptr)
    {
        daang += RandomRange(24) - 12;
        daang = NORM_ANGLE(daang);
        daz += RandomRange(10000) - 5000;
    }
    else
    {
        //daang = NORM_ANGLE(pp->angle.ang.Buildang() + (RandomRange(50) - 25));
        daang = NORM_ANGLE(pp->angle.ang.Buildang() + (RandomRange(24) - 12));
        daz = -MulScale(pp->horizon.horiz.asq16(), 2000, 16) + (RandomRange(24000) - 12000);
    }


    xvect = bcos(daang);
    yvect = bsin(daang);
    zvect = daz;
    FAFhitscan(pp->int_ppos().X, pp->int_ppos().Y, nz, pp->cursector,       // Start position
               xvect,yvect,zvect,
               hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
    {
        return 0;
    }

    SetVisHigh();

    // check to see what you hit
    if (hit.actor() == nullptr && hit.hitWall == nullptr)
    {
        if (abs(hit.hitpos.Z - hit.hitSector->ceilingz) <= 1)
        {
            hit.hitpos.Z += 16;
            cstat |= (CSTAT_SPRITE_YFLIP);

            if ((hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY))
                return 0;

            if (SectorIsUnderwaterArea(hit.hitSector))
            {
				WarpToSurface(hit.hitpos, &hit.hitSector);
                ContinueHitscan(pp, hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, xvect, yvect, zvect);
                return 0;
            }
        }
        else if (abs(hit.hitpos.Z - hit.hitSector->floorz) <= 1)
        {
            if ((hit.hitSector->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
            {
                SpawnSplashXY(hit.int_hitpos().X,hit.int_hitpos().Y,hit.int_hitpos().Z,hit.hitSector);

                if (SectorIsDiveArea(hit.hitSector))
                {
					WarpToUnderwater(hit.hitpos, &hit.hitSector);
                    ContinueHitscan(pp, hit.hitSector, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, xvect, yvect, zvect);
                    return 0;
                }

                return 0;
            }
        }
    }

    if (hit.hitWall != nullptr)
    {
        if (hit.hitWall->twoSided())
        {
            if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
            {
                if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                {
                    return 0;
                }
            }
        }


        if (hit.hitWall->lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, actor->user.ID);
            return 0;
        }

        QueueHole(hit.hitSector, hit.hitWall, hit.hitpos);
    }

    // hit a sprite?
    if (hit.actor() != nullptr)
    {
        auto hitActor = hit.actor();

        if (hitActor->hasU()) // JBF: added null check
            if (hitActor->user.ID == TRASHCAN)
            {
                extern STATE s_TrashCanPain[];

                PlaySound(DIGI_TRASHLID, hitActor, v3df_none);
                if (hitActor->user.WaitTics <= 0)
                {
                    hitActor->user.WaitTics = SEC(2);
                    ChangeState(hitActor,s_TrashCanPain);
                }
            }

        if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        {
            if (MissileHitMatch(nullptr, WPN_UZI, hitActor))
                return 0;
        }

        if ((hitActor->spr.extra & SPRX_BREAKABLE) && HitBreakSprite(hitActor,0))
        {
            return 0;
        }

        if (BulletHitSprite(pp->actor, hitActor, hit.hitpos, 0))
            return 0;

        // hit a switch?
        if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
        {
            ShootableSwitch(hitActor);
        }
    }


    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit.hitSector, hit.hitpos, DAngle::fromBuild(daang), 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SMOKE_REPEAT;
    actorNew->spr.yrepeat = UZI_SMOKE_REPEAT;
    SetOwner(pp->actor, actorNew);
    actorNew->spr.cstat |= (cstat | CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 8 >> 2;

    HitscanSpriteAdjust(actorNew, hit.hitWall);
    DoHitscanDamage(actorNew, hit.actor());

    actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit.hitSector, hit.hitpos, DAngle::fromBuild(daang), 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SPARK_REPEAT;
    actorNew->spr.yrepeat = UZI_SPARK_REPEAT;
    SetOwner(pp->actor, actorNew);
    actorNew->user.spal = actorNew->spr.pal = pal;
    actorNew->spr.cstat |= (cstat | CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 8 >> 2;

    HitscanSpriteAdjust(actorNew, hit.hitWall);

    if (RANDOM_P2(1024) < 100)
    {
        PlaySound(DIGI_RICHOCHET1,actorNew, v3df_none);
    }
    else if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET2,actorNew, v3df_none);

    return 0;
}


int InitTankShell(DSWActor* actor, PLAYER* pp)
{
    if (!SW_SHAREWARE)
        PlaySound(DIGI_CANNON, pp, v3df_dontpan|v3df_doppler);

    auto actorNew = SpawnActor(STAT_MISSILE, 0, s_TankShell, actor->sector(),
                    actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), TANK_SHELL_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 8;
    actorNew->spr.xrepeat = 8;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 32 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (4);
    actorNew->user.floor_dist = (4);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    actorNew->spr.zvel = int(-pp->horizon.horiz.asbuildf() * actorNew->spr.xvel * (1. / 8.));

    WeaponAutoAim(actor, actorNew, 64, false);
    // a bit of randomness
    actorNew->add_int_ang(RandomRange(30) - 15);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang()));

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);


    return 0;
}

int InitTurretMicro(DSWActor* actor, PLAYER* pp)
{
    DSWActor* plActor = pp->actor;
    int nx, ny, nz, dist;
    short i,ang;
    TARGET_SORT* ts = TargetSort;
    DSWActor* picked = nullptr;

    if (SW_SHAREWARE) return false; // JBF: verify


    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;

    const int MAX_TURRET_MICRO = 10;

    DoPickTarget(plActor, 256, false);

    if (TargetSortCount > MAX_TURRET_MICRO)
        TargetSortCount = MAX_TURRET_MICRO;

    for (i = 0; i < MAX_TURRET_MICRO; i++)
    {
        if (ts < &TargetSort[TargetSortCount] && ts->actor != nullptr)
        {
            picked = ts->actor;

            ang = getangle(picked->int_pos().X - nx, picked->int_pos().Y - ny);

            ts++;
        }
        else
        {
            picked = nullptr;
            ang = actor->int_ang();
        }

        nz = actor->int_pos().Z;
        nz += Z(RandomRange(20)) - Z(10);

        // Spawn a shot
        // Inserting and setting up variables

        auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Micro[0][0], actor->sector(),
                        nx, ny, nz, ang, 1200);

        SetOwner(plActor, actorNew);
        actorNew->spr.yrepeat = 24;
        actorNew->spr.xrepeat = 24;
        actorNew->spr.shade = -15;
        actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
        actorNew->spr.clipdist = 64L>>2;

        // randomize zvelocity
        actorNew->spr.zvel += RandomRange(Z(8)) - Z(5);

        actorNew->user.RotNum = 5;
        NewStateGroup(actorNew, &sg_Micro[0]);

        actorNew->user.WeaponNum = plActor->user.WeaponNum;
        actorNew->user.Radius = 200;
        actorNew->user.ceiling_dist = (2);
        actorNew->user.floor_dist = (2);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
        actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        actorNew->user.WaitTics = 10 + RandomRange(40);

        const int MICRO_ANG = 400;

        if (picked)
        {
			dist = DistanceI(actorNew->spr.pos, picked->spr.pos);
            if (dist != 0)
            {
                int zh;
                zh = ActorZOfTop(picked) + (ActorSizeZ(picked) >> 2);
                actorNew->spr.zvel = (actorNew->spr.xvel * (zh - actorNew->int_pos().Z)) / dist;
            }

            actorNew->user.WpnGoalActor = ts->actor;
            picked->user.Flags |= (SPR_TARGETED);
            picked->user.Flags |= (SPR_ATTACKED);
        }
        else
        {
            actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + (RandomRange(MICRO_ANG) - (MICRO_ANG / 2)) - 16));
        }

        actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
        actorNew->user.change.Z = actorNew->spr.zvel;
    }

    return 0;
}


int InitTurretRocket(DSWActor* actor, PLAYER* pp)
{
    if (SW_SHAREWARE) return false; // JBF: verify

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, &s_Rocket[0][0], actor->sector(),
                    actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), ROCKET_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 40;
    actorNew->spr.xrepeat = 40;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 32 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (4);
    actorNew->user.floor_dist = (4);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.zvel = int(-pp->horizon.horiz.asbuildf() * actorNew->spr.xvel * (1. / 8.));

    WeaponAutoAim(actor, actorNew, 64, false);
    // a bit of randomness
    //actorNew->spr.angle += RandomRange(30) - 15;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    return 0;
}

int InitTurretFireball(DSWActor* actor, PLAYER* pp)
{
    if (SW_SHAREWARE) return false; // JBF: verify

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL, s_Fireball, actor->sector(),
                    actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), FIREBALL_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 40;
    actorNew->spr.xrepeat = 40;
    actorNew->spr.shade = -40;
    actorNew->spr.zvel = 0;
    actorNew->spr.clipdist = 32 >> 2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 50;
    actorNew->user.ceiling_dist = (4);
    actorNew->user.floor_dist = (4);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.zvel = int(-pp->horizon.horiz.asbuildf() * actorNew->spr.xvel * (1. / 8.));

    WeaponAutoAim(actor, actorNew, 64, false);
    // a bit of randomness
    actorNew->add_int_ang(RandomRange(30) - 15);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang()));

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    return 0;
}

int InitTurretRail(DSWActor* actor, PLAYER* pp)
{
    int nx, ny, nz;

    if (SW_SHAREWARE) return false; // JBF: verify

    if (!pp->insector())
        return 0;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z;

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R1, &s_Rail[0][0], pp->cursector,
                    nx, ny, nz, actor->int_ang(), 1200);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Rail[0]);

    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER|CSTAT_SPRITE_INVISIBLE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    actorNew->spr.clipdist = 64L>>2;

    if (WeaponAutoAim(pp->actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang()));
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    return 0;
}

int InitTurretLaser(DSWActor* actor, PLAYER* pp)
{
    int nx, ny, nz;

    if (SW_SHAREWARE) return false; // JBF: verify

    if (!pp->insector())
        return 0;

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z;

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, BOLT_THINMAN_R0, s_Laser, pp->cursector,
                    nx, ny, nz, actor->int_ang(), 300);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 52;
    actorNew->spr.xrepeat = 52;
    actorNew->spr.shade = -15;

    // the slower the missile travels the less of a zvel it needs
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 11;

    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->user.Flags2 |= (SPR2_SO_MISSILE);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->spr.clipdist = 64L>>2;

    if (WeaponAutoAim(actor, actorNew, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang()));
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    return 0;
}

int InitSobjMachineGun(DSWActor* actor, PLAYER* pp)
{
    short daang;
    HitInfo hit{};
    int daz;
    int nx,ny,nz;
    short cstat = 0;
    DSWActor* spark;

    PlaySound(DIGI_BOATFIRE, pp, v3df_dontpan|v3df_doppler);

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    daz = nz = actor->int_pos().Z;

    if (RANDOM_P2(1024) < 200)
        InitTracerTurret(actor, pp->actor, pp->horizon.horiz.asq16());

    daang = 64;
    if (WeaponAutoAimHitscan(actor, &daz, &daang, false) != nullptr)
    {
        daz += RandomRange(Z(30)) - Z(15);
        //daz += 0;
    }
    else
    {
        fixed_t q16horiz = pp->horizon.horiz.asq16();
        fixed_t horizmin = IntToFixed(-25);
        if (q16horiz < horizmin)
            q16horiz = horizmin;

        daz = -MulScale(q16horiz, 2000, 16) + (RandomRange(Z(80)) - Z(40));
        daang = actor->int_ang();
    }

    FAFhitscan(nx, ny, nz, actor->sector(),       // Start position
               bcos(daang),      // X vector of 3D ang
               bsin(daang),      // Y vector of 3D ang
               daz,              // Z vector of 3D ang
               hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
    {
        return 0;
    }

    if (hit.actor() == nullptr && hit.hitWall == nullptr)
    {
        if (abs(hit.hitpos.Z - hit.hitSector->ceilingz) <= 1)
        {
            hit.hitpos.Z += 16;
            cstat |= (CSTAT_SPRITE_YFLIP);

            if ((hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY))
                return 0;
        }
        else if (abs(hit.hitpos.Z - hit.hitSector->floorz) <= 1)
        {
            if ((hit.hitSector->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
            {
                SpawnSplashXY(hit.int_hitpos().X,hit.int_hitpos().Y,hit.int_hitpos().Z,hit.hitSector);
                return 0;
            }
        }

    }

    // hit a sprite?
    if (hit.actor() != nullptr)
    {
        auto hitActor = hit.actor();
        if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        {
            // spawn sparks here and pass the sprite as SO_MISSILE
            spark = SpawnBoatSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
            spark->user.Flags2 |= SPR2_SO_MISSILE;
            if (MissileHitMatch(spark, -1, hit.actor()))
                return 0;
            return 0;
        }

        if ((hitActor->spr.extra & SPRX_BREAKABLE))
        {
            HitBreakSprite(hit.actor(), 0);
            return 0;
        }

        if (BulletHitSprite(pp->actor, hit.actor(), hit.hitpos, 0))
            return 0;

        // hit a switch?
        if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
        {
            ShootableSwitch(hit.actor());
        }
    }

    spark = SpawnBoatSparks(pp, hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
    DoHitscanDamage(spark, hit.actor());

    return 0;
}

int InitSobjGun(PLAYER* pp)
{
    short i;
    bool first = false;

    for (i = 0; pp->sop->so_actors[i] != nullptr; i++)
    {
        DSWActor* actor = pp->sop->so_actors[i];
        if (!actor) continue;
        if (actor->spr.statnum == STAT_SO_SHOOT_POINT)
        {
            // match when firing
            if (SP_TAG2(actor))
            {
                DoMatchEverything(pp, SP_TAG2(actor), -1);
                if (TEST_BOOL1(actor))
                {
                    SP_TAG2(actor) = 0;
                }
            }

            // inert shoot point
            if ((uint8_t)SP_TAG3(actor) == 255)
                return 0;

            if (!first)
            {
                first = true;
                if (SP_TAG6(actor))
                    DoSoundSpotMatch(SP_TAG6(actor), 1, SOUND_OBJECT_TYPE);
            }

            switch (SP_TAG3(actor))
            {
            case 32:
            case 0:
                SpawnVis(actor, nullptr, {}, 8);
                SpawnBigGunFlames(actor, pp->actor, pp->sop, false);
                SetGunQuake(actor);
                InitTankShell(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 80;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 1:
                SpawnVis(actor, nullptr, {}, 32);
                SpawnBigGunFlames(actor, pp->actor, pp->sop, true);
                InitSobjMachineGun(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 10;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 2:
                if (SW_SHAREWARE) break;
                SpawnVis(actor, nullptr, {}, 32);
                InitTurretLaser(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 120;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 3:
                if (SW_SHAREWARE) break;
                SpawnVis(actor, nullptr, {}, 32);
                InitTurretRail(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 120;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 4:
                if (SW_SHAREWARE) break;
                SpawnVis(actor, nullptr, {}, 32);
                InitTurretFireball(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 20;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 5:
                if (SW_SHAREWARE) break;
                SpawnVis(actor, nullptr, {}, 32);
                InitTurretRocket(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 100;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            case 6:
                if (SW_SHAREWARE) break;
                SpawnVis(actor, nullptr, {}, 32);
                InitTurretMicro(actor, pp);
                if (!SP_TAG5(actor))
                    pp->FirePause = 100;
                else
                    pp->FirePause = SP_TAG5(actor);
                break;
            }
        }
    }

    return 0;
}

DSWActor* SpawnBoatSparks(PLAYER* pp, sectortype* hit_sect, walltype* hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SMOKE_REPEAT + 12;
    actorNew->spr.yrepeat = UZI_SMOKE_REPEAT + 12;
    SetOwner(pp->actor, actorNew);
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    actorNew->spr.clipdist = 32 >> 2;

    HitscanSpriteAdjust(actorNew, hit_wall);

    actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SPARK_REPEAT + 10;
    actorNew->spr.yrepeat = UZI_SPARK_REPEAT + 10;
    SetOwner(pp->actor, actorNew);
    actorNew->user.spal = actorNew->spr.pal = PALETTE_DEFAULT;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.clipdist = 32 >> 2;

    HitscanSpriteAdjust(actorNew, hit_wall);

    if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET1, actorNew, v3df_none);

    return actorNew;
}

int SpawnSwordSparks(PLAYER* pp, sectortype* hit_sect, walltype* hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    DSWActor* actor = pp->actor;

    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = actorNew->spr.yrepeat = 20;
    SetOwner(pp->actor, actorNew);
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    actorNew->spr.clipdist = 32 >> 2;

    if (hit_wall != nullptr)
        HitscanSpriteAdjust(actorNew, hit_wall);

    actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = actorNew->spr.yrepeat = 20;
    SetOwner(pp->actor, actorNew);
    actorNew->user.spal = actorNew->spr.pal = PALETTE_DEFAULT;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    if (actor->user.WeaponNum == WPN_FIST)
        actorNew->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

    actorNew->spr.clipdist = 32 >> 2;

    if (hit_wall != nullptr)
        HitscanSpriteAdjust(actorNew, hit_wall);

    return 0;
}

DSWActor* SpawnTurretSparks(sectortype* hit_sect, walltype* hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SMOKE_REPEAT + 12;
    actorNew->spr.yrepeat = UZI_SMOKE_REPEAT + 12;
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);
    actorNew->spr.hitag = LUMINOUS; //Always full brightness

    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    actorNew->spr.clipdist = 32 >> 2;
    HitscanSpriteAdjust(actorNew, hit_wall);

    actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SPARK_REPEAT + 10;
    actorNew->spr.yrepeat = UZI_SPARK_REPEAT + 10;
    actorNew->user.spal = actorNew->spr.pal = PALETTE_DEFAULT;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.clipdist = 32 >> 2;
    HitscanSpriteAdjust(actorNew, hit_wall);

    if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET1, actorNew, v3df_none);

    return actorNew;
}

DSWActor* SpawnShotgunSparks(PLAYER* pp, sectortype* hit_sect, walltype* hit_wall, int hit_x, int hit_y, int hit_z, short hit_ang)
{
    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SPARK_REPEAT;
    actorNew->spr.yrepeat = UZI_SPARK_REPEAT;
    SetOwner(pp->actor, actorNew);
    actorNew->user.spal = actorNew->spr.pal = PALETTE_DEFAULT;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.clipdist = 32 >> 2;

    HitscanSpriteAdjust(actorNew, hit_wall);

    actorNew = SpawnActor(STAT_MISSILE, SHOTGUN_SMOKE, s_ShotgunSmoke, hit_sect, hit_x, hit_y, hit_z, hit_ang, 0);
    actorNew->spr.xrepeat = SHOTGUN_SMOKE_REPEAT;
    actorNew->spr.yrepeat = SHOTGUN_SMOKE_REPEAT;
    SetOwner(pp->actor, actorNew);
    actorNew->spr.cstat |= (CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_YCENTER);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    // Sprite starts out with center exactly on wall.
    // This moves it back enough to see it at all angles.

    actorNew->spr.clipdist = 32 >> 2;

    HitscanSpriteAdjust(actorNew, hit_wall);

    return actorNew;
}

int InitTurretMgun(SECTOR_OBJECT* sop)
{
    short daang, i;
    HitInfo hit{};
    int daz;
    int nx,ny,nz;
    short cstat = 0;
    short delta;
    int xvect,yvect,zvect;

    PlaySound(DIGI_BOATFIRE, sop->pmid, v3df_dontpan|v3df_doppler);

    for (i = 0; sop->so_actors[i] != nullptr; i++)
    {
        DSWActor* actor = sop->so_actors[i];
        if (!actor) continue;
        if (actor->spr.statnum == STAT_SO_SHOOT_POINT)
        {
            nx = actor->int_pos().X;
            ny = actor->int_pos().Y;
            daz = nz = actor->int_pos().Z;

            // if its not operated by a player
            if (sop->Animator)
            {
                // only auto aim for Z
                daang = 512;
                auto hitt = WeaponAutoAimHitscan(actor, &daz, &daang, false);
                hit.hitActor = hitt;
                if (hitt != nullptr)
                {
                    delta = short(abs(getincangle(actor->int_ang(), daang)));
                    if (delta > 128)
                    {
                        // don't shoot if greater than 128
                        return 0;
                    }
                    else if (delta > 24)
                    {
                        // always shoot the ground when tracking
                        // and not close
                        WeaponHitscanShootFeet(actor, hitt, &daz);

                        daang = actor->int_ang();
                        daang = NORM_ANGLE(daang + RANDOM_P2(32) - 16);
                    }
                    else
                    {
                        // randomize the z for shots
                        daz += RandomRange(Z(120)) - Z(60);
                        // never auto aim the angle
                        daang = actor->int_ang();
                        daang = NORM_ANGLE(daang + RANDOM_P2(64) - 32);
                    }
                }
            }
            else
            {
                daang = 64;
                if (WeaponAutoAimHitscan(actor, &daz, &daang, false) != nullptr)
                {
                    daz += RandomRange(Z(30)) - Z(15);
                }
            }

            xvect = bcos(daang);
            yvect = bsin(daang);
            zvect = daz;

            FAFhitscan(nx, ny, nz, actor->sector(),       // Start position
                       xvect, yvect, zvect,
                       hit, CLIPMASK_MISSILE);

            if (RANDOM_P2(1024) < 400)
            {
                InitTracerAutoTurret(sop->so_actors[i],
                                     xvect>>4, yvect>>4, zvect>>4);
            }

            if (hit.hitSector == nullptr)
                continue;

            if (hit.actor() == nullptr && hit.hitWall == nullptr)
            {
                if (abs(hit.hitpos.Z - hit.hitSector->ceilingz) <= 1)
                {
                    hit.hitpos.Z += 16;
                    cstat |= (CSTAT_SPRITE_YFLIP);

                    if ((hit.hitSector->ceilingstat & CSTAT_SECTOR_SKY))
                        continue;
                }
                else if (abs(hit.hitpos.Z - hit.hitSector->floorz) <= 1)
                {
                    if ((hit.hitSector->extra & SECTFX_LIQUID_MASK) != SECTFX_LIQUID_NONE)
                    {
                        SpawnSplashXY(hit.int_hitpos().X,hit.int_hitpos().Y,hit.int_hitpos().Z,hit.hitSector);
                        continue;
                    }
                }

            }

            if (hit.hitWall != nullptr)
            {
                if (hit.hitWall->twoSided())
                {
                    if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
                    {
                        if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                        {
                            return 0;
                        }
                    }
                }

                if (hit.hitWall->lotag == TAG_WALL_BREAK)
                {
                    HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, 0);
                    continue;
                }

                QueueHole(hit.hitSector, hit.hitWall, hit.hitpos);
            }

            // hit a sprite?
            if (hit.actor() != nullptr)
            {
                auto hitActor = hit.actor();

                if (hitActor->spr.lotag == TAG_SPRITE_HIT_MATCH)
                {
                    if (MissileHitMatch(nullptr, WPN_UZI, hit.actor()))
                        continue;
                }

                if ((hitActor->spr.extra & SPRX_BREAKABLE))
                {
                    HitBreakSprite(hit.actor(), 0);
                    continue;
                }

                if (BulletHitSprite(actor, hit.actor(), hit.hitpos, 0))
                    continue;

                // hit a switch?
                if ((hitActor->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && (hitActor->spr.lotag || hitActor->spr.hitag))
                {
                    ShootableSwitch(hit.actor());
                }
            }


            auto j = SpawnTurretSparks(hit.hitSector, hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang);
            DoHitscanDamage(j, hit.actor());
        }
    }

    return 0;
}


int InitEnemyUzi(DSWActor* actor)
{
    short daang;
    HitInfo hit{};
    int daz;
    int zh;
    void InitUziShell(PLAYER*);
    static short alternate;


    // Make sprite shade brighter
    actor->user.Vis = 128;

    SetActorZ(actor, actor->int_pos());

    if (actor->user.ID == ZILLA_RUN_R0)
    {
        zh = ActorZOfTop(actor);
        zh += Z(20);
    }
    else
    {
        zh = ActorSizeZ(actor);
        zh -= (zh >> 2);
    }
    daz = actor->int_pos().Z - zh;

    if (AimHitscanToTarget(actor, &daz, &daang, 200) != nullptr)
    {
        // set angle to player and also face player when attacking
        actor->set_int_ang(daang);
        daang += RandomRange(24) - 12;
        daang = NORM_ANGLE(daang);
        daz += RandomRange(Z(40)) - Z(20);
    }
    else
    {
        // couldn't shoot target for some reason

        // don't bother wasting processing 50% of the time
        if (RANDOM_P2(1024) < 512)
            return 0;

        daz = 0;
        daang = NORM_ANGLE(actor->int_ang() + (RANDOM_P2(128)) - 64);
    }

    FAFhitscan(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - zh, actor->sector(),      // Start position
               bcos(daang),      // X vector of 3D ang
               bsin(daang),      // Y vector of 3D ang
               daz,              // Z vector of 3D ang
               hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
        return 0;

    if (RANDOM_P2(1024<<4)>>4 > 700)
    {
        if (actor->user.ID == TOILETGIRL_R0 || actor->user.ID == WASHGIRL_R0 || actor->user.ID == CARGIRL_R0)
            SpawnShell(actor,-3);
        else
            SpawnShell(actor,-2); // Enemy Uzi shell
    }

    if ((alternate++)>2) alternate = 0;
    if (!alternate)
    {
        if (actor->spr.pal == PALETTE_PLAYER3 || actor->spr.pal == PALETTE_PLAYER5 ||
            actor->spr.pal == PAL_XLAT_LT_GREY || actor->spr.pal == PAL_XLAT_LT_TAN)
            PlaySound(DIGI_M60, actor, v3df_none);
        else
            PlaySound(DIGI_NINJAUZIATTACK, actor, v3df_none);
    }

    if (hit.hitWall != nullptr)
    {
        if (hit.hitWall->twoSided())
        {
            if ((hit.hitWall->nextSector()->ceilingstat & CSTAT_SECTOR_SKY))
            {
                if (hit.hitpos.Z < hit.hitWall->nextSector()->ceilingz)
                {
                    return 0;
                }
            }
        }

        if (hit.hitWall->lotag == TAG_WALL_BREAK)
        {
            HitBreakWall(hit.hitWall, hit.int_hitpos().X, hit.int_hitpos().Y, hit.int_hitpos().Z, daang, actor->user.ID);
            return 0;
        }

        QueueHole(hit.hitSector, hit.hitWall, hit.hitpos);
    }

    if (hit.actor() != nullptr)
    {
        if (BulletHitSprite(actor, hit.actor(), hit.hitpos, 0))
            return 0;
    }

    auto actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE+2, s_UziSmoke, hit.hitSector, hit.hitpos, DAngle::fromBuild(daang), 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SMOKE_REPEAT;
    actorNew->spr.yrepeat = UZI_SMOKE_REPEAT;

    if (actor->user.ID == ZOMBIE_RUN_R0)
        SetOwner(GetOwner(actor), actorNew);
    else
        SetOwner(actor, actorNew);

    actorNew->user.WaitTics = 63;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);

    actorNew->spr.clipdist = 32L >> 2;

    actorNew = SpawnActor(STAT_MISSILE, UZI_SMOKE, s_UziSmoke, hit.hitSector, hit.hitpos, DAngle::fromBuild(daang), 0);
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SMOKE_REPEAT;
    actorNew->spr.yrepeat = UZI_SMOKE_REPEAT;
    SetOwner(actor, actorNew);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 8 >> 2;

    HitscanSpriteAdjust(actorNew, hit.hitWall);
    DoHitscanDamage(actorNew, hit.actor());

    actorNew = SpawnActor(STAT_MISSILE, UZI_SPARK, s_UziSpark, hit.hitSector, hit.hitpos, DAngle::fromBuild(daang), 0);

    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = UZI_SPARK_REPEAT;
    actorNew->spr.yrepeat = UZI_SPARK_REPEAT;
    SetOwner(actor, actorNew);
    actorNew->user.spal = actorNew->spr.pal;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.clipdist = 8 >> 2;

    HitscanSpriteAdjust(actorNew, hit.hitWall);

    if (RANDOM_P2(1024) < 100)
    {
        PlaySound(DIGI_RICHOCHET1,actorNew, v3df_none);
    }
    else if (RANDOM_P2(1024) < 100)
        PlaySound(DIGI_RICHOCHET2,actorNew, v3df_none);

    return 0;
}


int InitGrenade(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int zvel;
    bool auto_aim = false;

    DoPlayerBeginRecoil(pp, GRENADE_RECOIL_AMT);

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);

    PlaySound(DIGI_30MMFIRE, pp, v3df_dontpan|v3df_doppler);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;
    nz = pp->int_ppos().Z + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, GRENADE, &s_Grenade[0][0], pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), GRENADE_VELOCITY);

    // don't throw it as far if crawling
    if (pp->Flags & (PF_CRAWLING))
    {
        actorNew->spr.xvel -= (actorNew->spr.xvel >> 2);
    }

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Grenade[0]);
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    //actorNew->spr.clipdist = 80L>>2;
    actorNew->spr.clipdist = 32L>>2;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK);

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;

    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 800);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    // don't do smoke for this movement
    actorNew->user.Flags |= (SPR_BOUNCE);
    MissileSetPos(actorNew, DoGrenade, 1000);
    actorNew->user.Flags &= ~(SPR_BOUNCE);

    actor->spr.clipdist = oclipdist;

    zvel = actorNew->spr.zvel;
    if (WeaponAutoAim(pp->actor, actorNew, 32, false) >= 0)
    {
        auto_aim = true;
    }
    actorNew->spr.zvel = zvel;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    if (!auto_aim)
    {
        // adjust xvel according to player velocity
        actorNew->user.change.X += pp->vect.X>>14;
        actorNew->user.change.Y += pp->vect.Y>>14;
    }

    actorNew->user.Counter2 = true;  // Phosphorus Grenade

    return 0;
}

int InitSpriteGrenade(DSWActor* actor)
{
    int nx, ny, nz;

    PlaySound(DIGI_30MMFIRE, actor, v3df_dontpan|v3df_doppler);

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - Z(40);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_MISSILE, GRENADE, &s_Grenade[0][0], actor->sector(),
                    nx, ny, nz, actor->int_ang(), GRENADE_VELOCITY);

    actorNew->user.RotNum = 5;
    NewStateGroup(actorNew, &sg_Grenade[0]);
    actorNew->user.Flags |= (SPR_XFLIP_TOGGLE);

    if (actor->user.ID == ZOMBIE_RUN_R0)
        SetOwner(GetOwner(actor), actorNew);
    else
        SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    //actorNew->spr.clipdist = 80L>>2;
    actorNew->spr.clipdist = 32L>>2;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (3);
    actorNew->user.floor_dist = (3);
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat |= (CSTAT_SPRITE_BLOCK);


    //actorNew->spr.zvel = (-RandomRange(100) * HORIZ_MULT);
    actorNew->spr.zvel = -2000;

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = actorNew->spr.zvel;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 800);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    // don't do smoke for this movement
    actorNew->user.Flags |= (SPR_BOUNCE);
    MissileSetPos(actorNew, DoGrenade, 400);
    actorNew->user.Flags &= ~(SPR_BOUNCE);

    return 0;
}

int InitMine(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx, ny, nz;
    int dot;

    PlayerUpdateAmmo(pp, actor->user.WeaponNum, -1);

    PlaySound(DIGI_MINETHROW, pp, v3df_dontpan|v3df_doppler);

    if (!pp->insector())
        return 0;

    nx = pp->int_ppos().X;
    ny = pp->int_ppos().Y;
    nz = pp->int_ppos().Z + pp->bob_z + Z(8);

    // Spawn a shot
    // Inserting and setting up variables

    auto actorNew = SpawnActor(STAT_MISSILE, MINE, s_Mine, pp->cursector,
                    nx, ny, nz, pp->angle.ang.Buildang(), MINE_VELOCITY);

    SetOwner(pp->actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->spr.clipdist = 128L>>2;
    actorNew->spr.zvel = -pp->horizon.horiz.asq16() >> 9;
    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (5);
    actorNew->user.floor_dist = (5);
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.spal = actorNew->spr.pal = actor->user.spal; // Set sticky color

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    MissileSetPos(actorNew, DoMine, 800);

    actorNew->user.change.Z = actorNew->spr.zvel>>1;
    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());

    dot = DOT_PRODUCT_2D(pp->vect.X, pp->vect.Y, pp->angle.ang.Cos() * (1 << 14), pp->angle.ang.Sin() * (1 << 14));

    // don't adjust for strafing
    if (labs(dot) > 10000)
    {
        // adjust xvel according to player velocity
        actorNew->user.change.X += pp->vect.X>>13;
        actorNew->user.change.Y += pp->vect.Y>>13;
    }

    return 0;
}

int InitEnemyMine(DSWActor* actor)
{
    int nx, ny, nz;

    PlaySound(DIGI_MINETHROW, actor, v3df_dontpan|v3df_doppler);

    nx = actor->int_pos().X;
    ny = actor->int_pos().Y;
    nz = actor->int_pos().Z - Z(40);

    // Spawn a shot
    // Inserting and setting up variables
    auto actorNew = SpawnActor(STAT_MISSILE, MINE, s_Mine, actor->sector(),
                    nx, ny, nz, actor->int_ang(), MINE_VELOCITY);

    SetOwner(actor, actorNew);
    actorNew->spr.yrepeat = 32;
    actorNew->spr.xrepeat = 32;
    actorNew->spr.shade = -15;
    actorNew->spr.clipdist = 128L>>2;

    actorNew->user.WeaponNum = actor->user.WeaponNum;
    actorNew->user.Radius = 200;
    actorNew->user.ceiling_dist = (5);
    actorNew->user.floor_dist = (5);
    actorNew->user.Counter = 0;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actorNew->user.spal = actorNew->spr.pal = actor->user.spal; // Set sticky color

    if (SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    MissileSetPos(actorNew, DoMine, 300);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));
    MissileSetPos(actorNew, DoMine, 300);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));

    actorNew->user.change.Z = -5000;
    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());

    return 0;
}

int HelpMissileLateral(DSWActor* actor, int dist)
{
    int xchange, ychange;

    auto old_xvel = actor->spr.xvel;
    auto old_clipdist = actor->spr.clipdist;

    actor->spr.xvel = dist;
    xchange = MOVEx(actor->spr.xvel, actor->int_ang());
    ychange = MOVEy(actor->spr.xvel, actor->int_ang());

    actor->spr.clipdist = 32L >> 2;

    actor->user.coll = move_missile(actor, xchange, ychange, 0, Z(16), Z(16), 0, 1);

    actor->spr.xvel = old_xvel;
    actor->spr.clipdist = old_clipdist;

    actor->backuppos();
    return 0;
}


int InitFireball(PLAYER* pp)
{
    DSWActor* actor = pp->actor;
    int nx = 0, ny = 0, nz;
    int zvel;

    PlayerUpdateAmmo(pp, WPN_HOTHEAD, -1);

    PlaySound(DIGI_HEADFIRE, pp, v3df_none);

    // Make sprite shade brighter
    actor->user.Vis = 128;

    if (!pp->insector())
        return 0;

    nx += pp->int_ppos().X;
    ny += pp->int_ppos().Y;

    nz = pp->int_ppos().Z + pp->bob_z + Z(15);

    auto actorNew = SpawnActor(STAT_MISSILE, FIREBALL1, s_Fireball, pp->cursector, nx, ny, nz, pp->angle.ang.Buildang(), FIREBALL_VELOCITY);

    actorNew->spr.hitag = LUMINOUS; //Always full brightness
    actorNew->spr.xrepeat = 40;
    actorNew->spr.yrepeat = 40;
    actorNew->spr.shade = -40;
    actorNew->spr.clipdist = 32>>2;
    SetOwner(pp->actor, actorNew);
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->user.Radius = 100;

    actorNew->user.ceiling_dist = (6);
    actorNew->user.floor_dist = (6);
    zvel = -MulScale(pp->horizon.horiz.asq16(), 240, 16);

    // at certain angles the clipping box was big enough to block the
    // initial positioning of the fireball.
    auto oclipdist = actor->spr.clipdist;
    actor->spr.clipdist = 0;

    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + 512));
    HelpMissileLateral(actorNew, 2100);
    actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 512));

    if (pp->Flags & (PF_DIVING) || SpriteInUnderwaterArea(actorNew))
        actorNew->user.Flags |= (SPR_UNDERWATER);

    if (TestMissileSetPos(actorNew, DoFireball, 1200, MulScale(zvel,44000, 16)))
    {
        actor->spr.clipdist = oclipdist;
        KillActor(actorNew);
        return 0;
    }

    actor->spr.clipdist = oclipdist;

    actorNew->spr.zvel = zvel >> 1;
    if (WeaponAutoAimZvel(pp->actor, actorNew, &zvel, 32, false) == -1)
    {
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - 9));
    }

    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Z = zvel;

    return 0;
}

int InitEnemyFireball(DSWActor* actor)
{
    int nz, dist;
    int size_z;
    int i, targ_z, xchange, ychange;

    static short lat_ang[] =
    {
        512, -512
    };

    auto targetActor = actor->user.targetActor;
    if (!targetActor) return 0;

    PlaySound(DIGI_FIREBALL1, actor, v3df_none);

    // get angle to player and also face player when attacking
    actor->set_int_ang(NORM_ANGLE(getangle(targetActor->int_pos().X - actor->int_pos().X, targetActor->int_pos().Y - actor->int_pos().Y)));

    size_z = Z(ActorSizeY(actor));
    nz = actor->int_pos().Z - size_z + (size_z >> 2) + (size_z >> 3) + Z(4);

    xchange = MOVEx(GORO_FIREBALL_VELOCITY, actor->int_ang());
    ychange = MOVEy(GORO_FIREBALL_VELOCITY, actor->int_ang());

    int lastvel = 0;
    for (i = 0; i < 2; i++)
    {
        auto actorNew = SpawnActor(STAT_MISSILE, GORO_FIREBALL, s_Fireball, actor->sector(),
                        actor->int_pos().X, actor->int_pos().Y, nz, actor->int_ang(), GORO_FIREBALL_VELOCITY);

        actorNew->spr.hitag = LUMINOUS; //Always full brightness
        actorNew->spr.xrepeat = 20;
        actorNew->spr.yrepeat = 20;
        actorNew->spr.shade = -40;

        SetOwner(actor, actorNew);
        actorNew->spr.zvel = 0;
        actorNew->spr.clipdist = 16>>2;

        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() + lat_ang[i]));
        HelpMissileLateral(actorNew, 500);
        actorNew->set_int_ang(NORM_ANGLE(actorNew->int_ang() - lat_ang[i]));

        actorNew->user.change.X = xchange;
        actorNew->user.change.Y = ychange;

        MissileSetPos(actorNew, DoFireball, 700);

        if (i == 0)
        {
            // find the distance to the target (player)
            dist = ksqrt(SQ(actorNew->int_pos().X - targetActor->int_pos().X) + SQ(actorNew->int_pos().Y - targetActor->int_pos().Y));

            // Determine target Z value
            targ_z = targetActor->int_pos().Z - (Z(ActorSizeY(actor)) >> 1);

            // (velocity * difference between the target and the throwing star) /
            // distance
            if (dist != 0)
                actorNew->user.change.Z = actorNew->spr.zvel = (GORO_FIREBALL_VELOCITY * (targ_z - actorNew->int_pos().Z)) / dist;

            // back up first one
            lastvel = actorNew->spr.zvel;
        }
        else
        {
            // use the first calculations so the balls stay together
            actorNew->user.change.Z = actorNew->spr.zvel = lastvel;
        }
    }

    return 0;

}

///////////////////////////////////////////////////////////////////////////////
// for hitscans or other uses
///////////////////////////////////////////////////////////////////////////////

bool WarpToUnderwater(sectortype** psectu, int *x, int *y, int *z)
{
    int i;
    auto sectu = *psectu;
    bool Found = false;
    int sx, sy;
    DSWActor* overActor = nullptr;
    DSWActor* underActor = nullptr;

    // 0 not valid for water match tags
    if (!sectu->hasU() || sectu->number == 0)
        return false;

    // search for DIVE_AREA "over" sprite for reference point
    SWStatIterator it(STAT_DIVE_AREA);
    while ((overActor = it.Next()))
    {
        if ((overActor->sector()->extra & SECTFX_DIVE_AREA) &&
            overActor->sector()->hasU() &&
            overActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);
    Found = false;

    // search for UNDERWATER "under" sprite for reference point
    it.Reset(STAT_UNDERWATER);
    while ((underActor = it.Next()))
    {
        if ((underActor->sector()->extra & SECTFX_UNDERWATER) &&
            underActor->sector()->hasU() &&
            underActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);

    // get the offset from the sprite
    sx = overActor->int_pos().X - *x;
    sy = overActor->int_pos().Y - *y;

    // update to the new x y position
    *x = underActor->int_pos().X - sx;
    *y = underActor->int_pos().Y - sy;

    auto over = overActor->sector();
    auto under = underActor->sector();

    if (GetOverlapSector(*x, *y, &over, &under) == 2)
    {
        *psectu = under;
    }
    else
    {
        *psectu = under;
    }

    *z = underActor->sector()->int_ceilingz() + Z(1);

    return true;
}

bool WarpToSurface(sectortype** psectu, int *x, int *y, int *z)
{
    int i;
    auto sectu = *psectu;
    int sx, sy;
    DSWActor* overActor = nullptr;
    DSWActor* underActor = nullptr;
    bool Found = false;

    // 0 not valid for water match tags
    if (!sectu->hasU() || sectu->number == 0)
        return false;

    // search for UNDERWATER "under" sprite for reference point
    SWStatIterator it(STAT_UNDERWATER);
    while ((underActor = it.Next()))
    {
        if ((underActor->sector()->extra & SECTFX_UNDERWATER) &&
            underActor->sector()->hasU() &&
            underActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);
    Found = false;

    // search for DIVE_AREA "over" sprite for reference point
    it.Reset(STAT_DIVE_AREA);
    while ((overActor = it.Next()))
    {
        if ((overActor->sector()->extra & SECTFX_DIVE_AREA) &&
            overActor->sector()->hasU() &&
            overActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);

    // get the offset from the under sprite
    sx = underActor->int_pos().X - *x;
    sy = underActor->int_pos().Y - *y;

    // update to the new x y position
    *x = overActor->int_pos().X - sx;
    *y = overActor->int_pos().Y - sy;

    auto over = overActor->sector();
    auto under = underActor->sector();

    if (GetOverlapSector(*x, *y, &over, &under))
    {
        *psectu = over;
    }

    *z = overActor->sector()->int_floorz() - Z(2);

    return true;
}


bool SpriteWarpToUnderwater(DSWActor* actor)
{
    int i;
    auto sectu = actor->sector();
    bool Found = false;
    int sx, sy;
    DSWActor* overActor = nullptr;
    DSWActor* underActor = nullptr;

    // 0 not valid for water match tags
    if (!sectu->hasU() || sectu->number == 0)
        return false;

    // search for DIVE_AREA "over" sprite for reference point
    SWStatIterator it(STAT_DIVE_AREA);
    while ((overActor = it.Next()))
    {
        if ((overActor->sector()->extra & SECTFX_DIVE_AREA) &&
            overActor->sector()->hasU() &&
            overActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);
    Found = false;

    // search for UNDERWATER "under" sprite for reference point
    it.Reset(STAT_UNDERWATER);
    while ((underActor = it.Next()))
    {
        if ((underActor->sector()->extra & SECTFX_UNDERWATER) &&
            underActor->sector()->hasU() &&
            underActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    ASSERT(Found);

    // get the offset from the sprite
    sx = overActor->int_pos().X - actor->int_pos().X;
    sy = overActor->int_pos().Y - actor->int_pos().Y;

    // update to the new x y position
    actor->set_int_xy(underActor->int_pos().X - sx, underActor->int_pos().Y - sy);

    auto over = overActor->sector();
    auto under = underActor->sector();

    if (GetOverlapSector(actor->int_pos().X, actor->int_pos().Y, &over, &under) == 2)
    {
        ChangeActorSect(actor, under);
    }
    else
    {
        ChangeActorSect(actor, over);
    }

    actor->set_int_z(underActor->sector()->int_ceilingz() + actor->user.int_ceiling_dist() + Z(1));

    actor->backuppos();

    return true;
}

bool SpriteWarpToSurface(DSWActor* actor)
{
    auto sectu = actor->sector();
    int sx, sy;
    DSWActor* overActor = nullptr;
    DSWActor* underActor = nullptr;
    bool Found = false;

    // 0 not valid for water match tags
    if (!sectu->hasU() || sectu->number == 0)
        return false;

    // search for UNDERWATER "under" sprite for reference point
    SWStatIterator it(STAT_UNDERWATER);
    while ((underActor = it.Next()))
    {
        if ((underActor->sector()->extra & SECTFX_UNDERWATER) &&
            underActor->sector()->hasU() &&
            underActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    if (!Found) return false;

    if (underActor->spr.lotag == 0)
        return false;

    Found = false;

    // search for DIVE_AREA "over" sprite for reference point
    it.Reset(STAT_DIVE_AREA);
    while ((overActor = it.Next()))
    {
        if ((overActor->sector()->extra & SECTFX_DIVE_AREA) &&
            overActor->sector()->hasU() &&
            overActor->sector()->number == sectu->number)
        {
            Found = true;
            break;
        }
    }

    if (!Found) return false;

    // get the offset from the under sprite
    sx = underActor->int_pos().X - actor->int_pos().X;
    sy = underActor->int_pos().Y - actor->int_pos().Y;

    // update to the new x y position
    actor->set_int_xy(overActor->int_pos().X - sx, overActor->int_pos().Y - sy);

    auto over = overActor->sector();
    auto under = underActor->sector();

    if (GetOverlapSector(actor->int_pos().X, actor->int_pos().Y, &over, &under))
    {
        ChangeActorSect(actor, over);
    }

    actor->spr.pos.Z = overActor->sector()->floorz - 2;

    // set z range and wade depth so we know how high to set view
    DoActorZrange(actor);
    MissileWaterAdjust(actor);


    actor->backuppos();

    return true;
}


int SpawnSplash(DSWActor* actor)
{
    auto sectu = actor->sector();
    sectortype* sectp = actor->sector();

    if (Prediction)
        return 0;

    if (sectu && ((sectp->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_NONE))
        return 0;

    if (sectu && (sectp->floorstat & CSTAT_SECTOR_SKY))
        return 0;

    PlaySound(DIGI_SPLASH1, actor, v3df_none);

    DoActorZrange(actor);
    MissileWaterAdjust(actor);

    auto actorNew = SpawnActor(STAT_MISSILE, SPLASH, s_Splash, actor->sector(), DVector3(actor->spr.pos.XY(), actor->user.loz), actor->spr.angle, 0);

    if (sectu && (sectp->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        actorNew->user.spal = actorNew->spr.pal = PALETTE_RED_LIGHTING;

    actorNew->spr.xrepeat = 45;
    actorNew->spr.yrepeat = 42;
    actorNew->spr.shade = actor->sector()->floorshade - 10;

    return 0;
}

int SpawnSplashXY(int hit_x, int hit_y, int hit_z, sectortype* sectp)
{
    if (Prediction)
        return 0;

    if (sectp->hasU() && ((sectp->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_NONE))
        return 0;

    if (sectp->hasU() && (sectp->floorstat & CSTAT_SECTOR_SKY))
        return 0;

    auto actorNew = SpawnActor(STAT_MISSILE, SPLASH, s_Splash, sectp, hit_x, hit_y, hit_z, 0, 0);

    if (sectp->hasU() && (sectp->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        actorNew->user.spal = actorNew->spr.pal = PALETTE_RED_LIGHTING;

    actorNew->spr.xrepeat = 45;
    actorNew->spr.yrepeat = 42;
    actorNew->spr.shade = actorNew->sector()->floorshade - 10;

    return 0;
}

bool MissileHitDiveArea(DSWActor* actor)
{
    // correctly set underwater bit for missiles
    // in Stacked water areas.
    if (FAF_ConnectArea(actor->sector()))
    {
        if (SectorIsUnderwaterArea(actor->sector()))
            actor->user.Flags |= (SPR_UNDERWATER);
        else
            actor->user.Flags &= ~(SPR_UNDERWATER);
    }

    if (actor->user.coll.type == kHitSector)
    {
        auto hit_sect = actor->user.coll.hitSector;

        if (SpriteInDiveArea(actor))
        {
            // make sure you are close to the floor
            if (actor->spr.pos.Z < (actor->user.hiz + actor->user.loz) * 0.5)
                return false;

            // Check added by Jim because of sprite bridge over water
            if (actor->spr.pos.Z < hit_sect->floorz - 20)
                return false;

            actor->user.Flags |= (SPR_UNDERWATER);
            SpawnSplash(actor);
            SpriteWarpToUnderwater(actor);
            actor->user.coll.setNone();
            PlaySound(DIGI_PROJECTILEWATERHIT, actor, v3df_none);
            return true;
        }
        else if (SpriteInUnderwaterArea(actor))
        {
            // make sure you are close to the ceiling
            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                return false;

            actor->user.Flags &= ~(SPR_UNDERWATER);
            if (!SpriteWarpToSurface(actor))
            {
                return false;
            }
            SpawnSplash(actor);
            actor->user.coll.setNone();
            return true;
        }
    }
    return false;
}

DSWActor* SpawnBubble(DSWActor* actor)
{
    if (Prediction)
        return nullptr;

    auto actorNew = SpawnActor(STAT_MISSILE, BUBBLE, s_Bubble, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    actorNew->spr.xrepeat = 8 + (RANDOM_P2(8 << 8) >> 8);
    actorNew->spr.yrepeat = actorNew->spr.xrepeat;
    actorNew->user.pos.X = actorNew->spr.xrepeat;
    actorNew->user.pos.Y = actorNew->spr.yrepeat;
    actorNew->user.ceiling_dist = (1);
    actorNew->user.floor_dist = (1);
    actorNew->spr.shade = actor->sector()->floorshade - 10;
    actorNew->user.WaitTics = 120 * 120;
    actorNew->spr.zvel = 512;
    actorNew->spr.clipdist = 12 >> 2;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->user.Flags |= (SPR_UNDERWATER);
    actorNew->spr.shade = -60; // Make em brighter

    return actorNew;
}

int DoVehicleSmoke(DSWActor* actor)
{
    actor->add_int_pos({ actor->user.change.X, actor->user.change.Y, -actor->spr.zvel });
    return false;
}

int DoWaterSmoke(DSWActor* actor)
{
    actor->add_int_z(-actor->spr.zvel);
    return false;
}

int SpawnVehicleSmoke(DSWActor* actor)
{
    if (MoveSkip2 != 0)
        return false;

    auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_VehicleSmoke, actor->sector(),
                      actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - RANDOM_P2(Z(8)), actor->int_ang(), 0);

    actorNew->user.WaitTics = 1*120;
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.yrepeat = 64;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    actorNew->set_int_ang(RANDOM_P2(2048));
    actorNew->spr.xvel = RANDOM_P2(32);
    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->spr.zvel = Z(4) + RANDOM_P2(Z(4));

    return false;
}

int SpawnSmokePuff(DSWActor* actor)
{
    auto actorNew = SpawnActor(STAT_MISSILE, PUFF, s_WaterSmoke, actor->sector(),
                      actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - RANDOM_P2(Z(8)), actor->int_ang(), 0);

    actorNew->user.WaitTics = 1*120;
    actorNew->spr.shade = -40;
    actorNew->spr.xrepeat = 64;
    actorNew->spr.yrepeat = 64;
    actorNew->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    if (RANDOM_P2(1024) < 512)
        actorNew->spr.cstat |= (CSTAT_SPRITE_YFLIP);

    actorNew->set_int_ang(RANDOM_P2(2048));
    actorNew->spr.xvel = RANDOM_P2(32);
    actorNew->user.change.X = MOVEx(actorNew->spr.xvel, actorNew->int_ang());
    actorNew->user.change.Y = MOVEy(actorNew->spr.xvel, actorNew->int_ang());
    //actorNew->spr.zvel = Z(4) + RANDOM_P2(Z(4));
    actorNew->spr.zvel = Z(1) + RANDOM_P2(Z(2));

    return false;
}


int DoBubble(DSWActor* actor)
{
    actor->add_int_z(-actor->spr.zvel);
    actor->spr.zvel += 32;

    if (actor->spr.zvel > 768)
        actor->spr.zvel = 768;

    actor->user.pos.X += 1;
    actor->user.pos.Y += 1;

    if (actor->user.pos.X > 32)
    {
        actor->user.pos.X = 32;
        actor->user.pos.Y = 32;
    }

    actor->spr.xrepeat = actor->user.pos.X + (RANDOM_P2(8 << 8) >> 8) - 4;
    actor->spr.yrepeat = actor->user.pos.Y + (RANDOM_P2(8 << 8) >> 8) - 4;

    if (actor->spr.pos.Z < actor->sector()->ceilingz)
    {
        if (SectorIsUnderwaterArea(actor->user.hi_sectp))
        {
            if (!SpriteWarpToSurface(actor))
            {
                KillActor(actor);
                return true;
            }

            actor->user.Flags &= ~(SPR_UNDERWATER);
            // stick around above water for this long
            actor->user.WaitTics = (RANDOM_P2(64 << 8) >> 8);
        }
        else
        {
            KillActor(actor);
            return true;
        }
    }

    if (!(actor->user.Flags & SPR_UNDERWATER))
    {
        if ((actor->user.WaitTics -= MISSILEMOVETICS) <= 0)
        {
            KillActor(actor);
            return true;
        }
    }
    else
    // just in case its stuck somewhere kill it after a while
    {
        if ((actor->user.WaitTics -= MISSILEMOVETICS) <= 0)
        {
            KillActor(actor);
            return true;
        }
    }

    return false;
}

// this needs to be called before killsprite
// whenever killing a sprite that you aren't completely sure what it is, like
// with the drivables, copy sectors, break sprites, etc
void SpriteQueueDelete(DSWActor* actor)
{
    size_t i;

    for (i = 0; i < MAX_STAR_QUEUE; i++)
        if (StarQueue[i] == actor)
            StarQueue[i] = nullptr;

    for (i = 0; i < MAX_HOLE_QUEUE; i++)
        if (HoleQueue[i] == actor)
            HoleQueue[i] = nullptr;

    for (i = 0; i < MAX_WALLBLOOD_QUEUE; i++)
        if (WallBloodQueue[i] == actor)
            WallBloodQueue[i] = nullptr;

    for (i = 0; i < MAX_FLOORBLOOD_QUEUE; i++)
        if (FloorBloodQueue[i] == actor)
            FloorBloodQueue[i] = nullptr;

    for (i = 0; i < MAX_GENERIC_QUEUE; i++)
        if (GenericQueue[i] == actor)
            GenericQueue[i] = nullptr;

    for (i = 0; i < MAX_LOWANGS_QUEUE; i++)
        if (LoWangsQueue[i] == actor)
            LoWangsQueue[i] = nullptr;
}


void QueueReset(void)
{
    size_t i;
    StarQueueHead=0;
    HoleQueueHead=0;
    WallBloodQueueHead=0;
    FloorBloodQueueHead=0;
    GenericQueueHead=0;
    LoWangsQueueHead=0;


    for (i = 0; i < MAX_STAR_QUEUE; i++)
        StarQueue[i] = nullptr;

    for (i = 0; i < MAX_HOLE_QUEUE; i++)
        HoleQueue[i] = nullptr;

    for (i = 0; i < MAX_WALLBLOOD_QUEUE; i++)
        WallBloodQueue[i] = nullptr;

    for (i = 0; i < MAX_FLOORBLOOD_QUEUE; i++)
        FloorBloodQueue[i] = nullptr;

    for (i = 0; i < MAX_GENERIC_QUEUE; i++)
        GenericQueue[i] = nullptr;

    for (i = 0; i < MAX_LOWANGS_QUEUE; i++)
        LoWangsQueue[i] = nullptr;
}

bool TestDontStick(DSWActor* actor, walltype* hit_wall)
{
    if (hit_wall == nullptr)
    {
        ASSERT(actor != nullptr);
        if (actor->user.coll.type != kHitWall) return true; // ain't got a wall here.
        hit_wall = actor->user.coll.hitWall;
    }


    if ((hit_wall->extra & WALLFX_DONT_STICK))
        return true;

    // if blocking red wallo
    if ((hit_wall->cstat & CSTAT_WALL_BLOCK) && hit_wall->twoSided())
        return true;

    return false;
}

bool TestDontStickSector(sectortype* hit_sect)
{
    if ((hit_sect->extra & (SECTFX_DYNAMIC_AREA|SECTFX_SECTOR_OBJECT)))
        return true;

    return false;
}

int QueueStar(DSWActor* actor)
{
    if (TestDontStick(actor, nullptr))
    {
        KillActor(actor);
        return -1;
    }

    // can and should kill the user portion of the star
    if (StarQueue[StarQueueHead] == nullptr)
    {
        // new star
        actor->clearUser();
        change_actor_stat(actor, STAT_STAR_QUEUE);
        StarQueue[StarQueueHead] = actor;
    }
    else
    {
        // move old star to new stars place
        auto osp = StarQueue[StarQueueHead];
        osp->spr.pos = actor->spr.pos;
        ChangeActorSect(osp, actor->sector());
        KillActor(actor);
        actor = osp;
    }

    StarQueueHead = (StarQueueHead+1) & (MAX_STAR_QUEUE-1);

    return 0;
}

void QueueHole(sectortype* hit_sect, walltype* hit_wall, const DVector3& pos)
{
    short w,nw,wall_ang;
    DSWActor* spawnedActor;
    int nx,ny;

    if (TestDontStick(nullptr, hit_wall))
        return;

    if (HoleQueue[HoleQueueHead] == nullptr)
        HoleQueue[HoleQueueHead] = spawnedActor = insertActor(hit_sect, STAT_HOLE_QUEUE);
    else
        spawnedActor = HoleQueue[HoleQueueHead];

    HoleQueueHead = (HoleQueueHead+1) & (MAX_HOLE_QUEUE-1);

    spawnedActor->spr.xrepeat = spawnedActor->spr.yrepeat = 16;
    spawnedActor->spr.cstat = 0;
    spawnedActor->spr.pal = 0;
    spawnedActor->spr.shade = 0;
    spawnedActor->spr.extra = 0;
    spawnedActor->spr.clipdist = 0;
    spawnedActor->spr.xoffset = spawnedActor->spr.yoffset = 0;
    spawnedActor->spr.pos = pos;
    spawnedActor->spr.picnum = 2151;
    ChangeActorSect(spawnedActor, hit_sect);

    ASSERT(spawnedActor->spr.statnum != MAXSTATUS);

    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ONE_SIDE);
    spawnedActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    wall_ang = NORM_ANGLE(getangle(hit_wall->delta())+512);
    spawnedActor->set_int_ang(wall_ang);

    // move it back some
    nx = bcos(spawnedActor->int_ang(), 4);
    ny = bsin(spawnedActor->int_ang(), 4);

    auto sect = spawnedActor->sector();

    Collision coll;
    clipmove(spawnedActor->spr.pos, &sect, nx, ny, 0, 0, 0, CLIPMASK_MISSILE, coll, 1);

    if (spawnedActor->sector() != sect)
        ChangeActorSect(spawnedActor, sect);

}

enum { FLOORBLOOD_RATE = 30 };
ANIMATOR DoFloorBlood;
STATE s_FloorBlood1[] =
{
    {FLOORBLOOD1, SF_QUICK_CALL,   DoFloorBlood, &s_FloorBlood1[1]},
    {FLOORBLOOD1, FLOORBLOOD_RATE, NullAnimator, &s_FloorBlood1[0]},
};

int QueueFloorBlood(DSWActor* actor)
{
    sectortype* sectp = actor->sector();
    DSWActor* spawnedActor = nullptr;


    if ((sectp->extra & SECTFX_SINK)||(sectp->extra & SECTFX_CURRENT))
        return -1;   // No blood in water or current areas

    if (actor->user.Flags & (SPR_UNDERWATER) || SpriteInUnderwaterArea(actor) || SpriteInDiveArea(actor))
        return -1;   // No blood underwater!

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
        return -1;   // No prints liquid areas!

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        return -1;   // Not in lave either

    if (TestDontStickSector(actor->sector()))
        return -1;   // Not on special sectors you don't

    if (FloorBloodQueue[FloorBloodQueueHead] != nullptr)
        KillActor(FloorBloodQueue[FloorBloodQueueHead]);

    FloorBloodQueue[FloorBloodQueueHead] = spawnedActor =
                                               SpawnActor(STAT_SKIP4, FLOORBLOOD1, s_FloorBlood1, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    FloorBloodQueueHead = (FloorBloodQueueHead+1) & (MAX_FLOORBLOOD_QUEUE-1);

    // Stupid hack to fix the blood under the skull to not show through
    // x,y repeat of floor blood MUST be smaller than the sprite above it or clipping probs.
    if (actor->user.ID == GORE_Head)
        spawnedActor->spr.hitag = 9995;
    else
        spawnedActor->spr.hitag = 0;
    spawnedActor->spr.xrepeat = spawnedActor->spr.yrepeat = 8;
    spawnedActor->spr.cstat = 0;
    spawnedActor->spr.pal = 0;
    spawnedActor->spr.shade = 0;
    spawnedActor->spr.extra = 0;
    spawnedActor->spr.clipdist = 0;
    spawnedActor->spr.xoffset = spawnedActor->spr.yoffset = 0;
    spawnedActor->spr.pos = actor->spr.pos.plusZ(1);
    spawnedActor->set_int_ang(RANDOM_P2(2048)); // Just make it any old angle
    spawnedActor->spr.shade -= 5;  // Brighten it up just a bit

    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_FLOOR);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ONE_SIDE);
    spawnedActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->user.Flags &= ~(SPR_SHADOW);

    return 0;
}

enum
{
    FOOTPRINT1 = 2490,
    FOOTPRINT2 = 2491,
    FOOTPRINT3 = 2492,
    FOOTPRINT_RATE = 30,
};
ANIMATOR DoFootPrint;
STATE s_FootPrint1[] =
{
    {FOOTPRINT1, FOOTPRINT_RATE, NullAnimator, &s_FootPrint1[0]},
};
STATE s_FootPrint2[] =
{
    {FOOTPRINT2, FOOTPRINT_RATE, NullAnimator, &s_FootPrint2[0]},
};
STATE s_FootPrint3[] =
{
    {FOOTPRINT3, FOOTPRINT_RATE, NullAnimator, &s_FootPrint3[0]},
};

int QueueFootPrint(DSWActor* actor)
{
    DSWActor* spawnedActor;
    short rnd_num=0;
    bool Found=false;
    sectortype* sectp = actor->sector();


    if ((sectp->extra & SECTFX_SINK)||(sectp->extra & SECTFX_CURRENT))
        return -1;   // No blood in water or current areas

    if (actor->user.PlayerP)
    {
        if ((actor->user.PlayerP->Flags & PF_DIVING))
            Found = true;

        // Stupid masked floor stuff!  Damn your weirdness!
        if ((actor->user.PlayerP->cursector->ceilingstat & CSTAT_SECTOR_SKY))
            Found = true;
        if ((actor->user.PlayerP->cursector->floorstat & CSTAT_SECTOR_SKY))
            Found = true;
    }

    if (actor->user.Flags & (SPR_UNDERWATER) || SpriteInUnderwaterArea(actor) || Found || SpriteInDiveArea(actor))
        return -1;   // No prints underwater!

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
        return -1;   // No prints liquid areas!

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
        return -1;   // Not in lave either

    if (TestDontStickSector(actor->sector()))
        return -1;   // Not on special sectors you don't

    // So, are we like, done checking now!?
    if (FloorBloodQueue[FloorBloodQueueHead] != nullptr)
        KillActor(FloorBloodQueue[FloorBloodQueueHead]);

    rnd_num = RandomRange(1024);

    if (rnd_num > 683)
        FloorBloodQueue[FloorBloodQueueHead] = spawnedActor =
                                                   SpawnActor(STAT_WALLBLOOD_QUEUE, FOOTPRINT1, s_FootPrint1, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    else if (rnd_num > 342)
        FloorBloodQueue[FloorBloodQueueHead] = spawnedActor =
                                                   SpawnActor(STAT_WALLBLOOD_QUEUE, FOOTPRINT2, s_FootPrint2, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    else
        FloorBloodQueue[FloorBloodQueueHead] = spawnedActor =
                                                   SpawnActor(STAT_WALLBLOOD_QUEUE, FOOTPRINT3, s_FootPrint3, actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);

    FloorBloodQueueHead = (FloorBloodQueueHead+1) & (MAX_FLOORBLOOD_QUEUE-1);

    // Decrease footprint count
    if (actor->user.PlayerP)
        actor->user.PlayerP->NumFootPrints--;


    spawnedActor->spr.hitag = 0;
    spawnedActor->spr.xrepeat = 48;
    spawnedActor->spr.yrepeat = 54;
    spawnedActor->spr.cstat = 0;
    spawnedActor->spr.pal = 0;
    spawnedActor->spr.shade = 0;
    spawnedActor->spr.extra = 0;
    spawnedActor->spr.clipdist = 0;
    spawnedActor->spr.xoffset = spawnedActor->spr.yoffset = 0;
    spawnedActor->spr.pos = actor->spr.pos;
    spawnedActor->spr.angle = actor->spr.angle;
    spawnedActor->user.Flags &= ~(SPR_SHADOW);
    switch (FootMode)
    {
    case BLOOD_FOOT:
        spawnedActor->user.spal = spawnedActor->spr.pal = PALETTE_PLAYER3; // Turn blue to blood red
        break;
    default:
        spawnedActor->user.spal = spawnedActor->spr.pal = PALETTE_PLAYER1; // Gray water
        break;
    }


    // Alternate the feet
    left_foot = !left_foot;
    if (left_foot)
        spawnedActor->spr.cstat |= (CSTAT_SPRITE_XFLIP);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_FLOOR);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ONE_SIDE);
    spawnedActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    return 0;
}

enum
{
    WALLBLOOD1 = 2500,
    WALLBLOOD2 = 2501,
    WALLBLOOD3 = 2502,
    WALLBLOOD4 = 2503,
    WALLBLOOD_RATE = 30,
};
ANIMATOR DoWallBlood;
STATE s_WallBlood1[] =
{
    {WALLBLOOD1, SF_QUICK_CALL, DoWallBlood, &s_WallBlood1[1]},
    {WALLBLOOD1, WALLBLOOD_RATE, NullAnimator, &s_WallBlood1[0]},
};
STATE s_WallBlood2[] =
{
    {WALLBLOOD2, SF_QUICK_CALL, DoWallBlood, &s_WallBlood2[1]},
    {WALLBLOOD2, WALLBLOOD_RATE, NullAnimator, &s_WallBlood2[0]},
};
STATE s_WallBlood3[] =
{
    {WALLBLOOD3, SF_QUICK_CALL, DoWallBlood, &s_WallBlood3[1]},
    {WALLBLOOD3, WALLBLOOD_RATE, NullAnimator, &s_WallBlood3[0]},
};
STATE s_WallBlood4[] =
{
    {WALLBLOOD4, SF_QUICK_CALL, DoWallBlood, &s_WallBlood4[1]},
    {WALLBLOOD4, WALLBLOOD_RATE, NullAnimator, &s_WallBlood4[0]},
};


DSWActor* QueueWallBlood(DSWActor* actor, short ang)
{
    short w,nw,wall_ang,dang;
    DSWActor* spawnedActor;
    int nx,ny;
    short rndnum;
    int daz;
    HitInfo hit{};
	DAngle bang = DAngle::fromBuild(ang);

    if (actor->user.Flags & (SPR_UNDERWATER) || SpriteInUnderwaterArea(actor) || SpriteInDiveArea(actor))
        return nullptr;   // No blood underwater!

    daz = Z(RANDOM_P2(128))<<3;
    daz -= (Z(128)<<2);
    dang = (ang+(RANDOM_P2(128<<5) >> 5)) - (64);

    FAFhitscan(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - Z(30), actor->sector(),    // Start position
               bcos(dang),      // X vector of 3D ang
               bsin(dang),      // Y vector of 3D ang
               daz,              // Z vector of 3D ang
               hit, CLIPMASK_MISSILE);

    if (hit.hitSector == nullptr)
        return nullptr;

    const int WALLBLOOD_DIST_MAX = 2500;
    if (DistanceI(hit.hitpos, actor->spr.pos) > WALLBLOOD_DIST_MAX)
        return nullptr;

    // hit a sprite?
    if (hit.actor() != nullptr)
        return nullptr;   // Don't try to put blood on a sprite

    if (hit.hitWall != nullptr)   // Don't check if blood didn't hit a wall, otherwise the ASSERT fails!
    {
        if (TestDontStick(nullptr, hit.hitWall))
            return nullptr;
    }
    else
        return nullptr;


    if (WallBloodQueue[WallBloodQueueHead] != nullptr)
        KillActor(WallBloodQueue[WallBloodQueueHead]);

    // Randomly choose a wall blood sprite
    rndnum = RandomRange(1024);
    if (rndnum > 768)
    {
        WallBloodQueue[WallBloodQueueHead] = spawnedActor =
                                                 SpawnActor(STAT_WALLBLOOD_QUEUE, WALLBLOOD1, s_WallBlood1, hit.hitSector, hit.hitpos, bang, 0);
    }
    else if (rndnum > 512)
    {
        WallBloodQueue[WallBloodQueueHead] = spawnedActor =
                                                 SpawnActor(STAT_WALLBLOOD_QUEUE, WALLBLOOD2, s_WallBlood2, hit.hitSector, hit.hitpos, bang, 0);
    }
    else if (rndnum > 128)
    {
        WallBloodQueue[WallBloodQueueHead] = spawnedActor =
                                                 SpawnActor(STAT_WALLBLOOD_QUEUE, WALLBLOOD3, s_WallBlood3, hit.hitSector, hit.hitpos, bang, 0);
    }
    else
    {
        WallBloodQueue[WallBloodQueueHead] = spawnedActor =
                                                 SpawnActor(STAT_WALLBLOOD_QUEUE, WALLBLOOD4, s_WallBlood4, hit.hitSector, hit.hitpos, bang, 0);
    }

    WallBloodQueueHead = (WallBloodQueueHead+1) & (MAX_WALLBLOOD_QUEUE-1);

    spawnedActor->spr.xrepeat = 30;
    spawnedActor->spr.yrepeat = 40;   // yrepeat will grow towards 64, it's default size
    spawnedActor->spr.cstat = 0;
    spawnedActor->spr.pal = 0;
    spawnedActor->spr.shade = 0;
    spawnedActor->spr.extra = 0;
    spawnedActor->spr.clipdist = 0;
    spawnedActor->spr.xoffset = spawnedActor->spr.yoffset = 0;
    spawnedActor->spr.pos = hit.hitpos;
    spawnedActor->spr.shade -= 5;  // Brighten it up just a bit
    spawnedActor->tempwall = hit.hitWall; // pass hitinfo.wall

    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_ONE_SIDE);
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_YCENTER);
    spawnedActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);

    wall_ang = NORM_ANGLE(getangle(hit.hitWall->delta()) + 512);
    spawnedActor->set_int_ang(wall_ang);

    // move it back some
    nx = bcos(spawnedActor->int_ang(), 4);
    ny = bsin(spawnedActor->int_ang(), 4);

    auto sect = spawnedActor->sector();

    Collision coll;
    clipmove(spawnedActor->spr.pos, &sect, nx, ny, 0, 0, 0, CLIPMASK_MISSILE, coll, 1);

    if (spawnedActor->sector() != sect)
        ChangeActorSect(spawnedActor, sect);

    return spawnedActor;
}

int DoFloorBlood(DSWActor* actor)
{
    const int FEET_IN_BLOOD_DIST = 300;

    int dist, near_dist = FEET_IN_BLOOD_DIST, a,b,c;
    short pnum;
    PLAYER* pp;
    short xsiz,ysiz;


    if (actor->spr.hitag == 9995)
    {
        xsiz = 12;
        ysiz = 12;
    }
    else
    {
        xsiz = 40;
        ysiz = 40;
    }

    // Make pool of blood seem to grow
    if (actor->spr.xrepeat < xsiz && actor->spr.xrepeat != 4)
    {
        actor->spr.xrepeat++;
    }

    if (actor->spr.yrepeat < ysiz && actor->spr.xrepeat != xsiz && actor->spr.xrepeat != 4)
    {
        actor->spr.yrepeat++;
    }

    // See if any players stepped in blood
    if (actor->spr.xrepeat != 4 && actor->spr.yrepeat > 4)
    {
        TRAVERSE_CONNECT(pnum)
        {
            pp = &Player[pnum];

            DISTANCE(actor->int_pos().X, actor->int_pos().Y, pp->int_ppos().X, pp->int_ppos().Y, dist, a, b, c);

            if (dist < near_dist)
            {
                if (pp->NumFootPrints <= 0 || FootMode != BLOOD_FOOT)
                {
                    pp->NumFootPrints = RandomRange(10)+3;
                    FootMode = BLOOD_FOOT;
                }

                // If blood has already grown to max size, we can shrink it
                if (actor->spr.xrepeat == 40 && actor->spr.yrepeat > 10)
                {
                    actor->spr.yrepeat -= 10;
                    if (actor->spr.yrepeat <= 10)  // Shrink it down and don't use it anymore
                        actor->spr.xrepeat = actor->spr.yrepeat = 4;
                }
            }
        }
    }

    return 0;
}

int DoWallBlood(DSWActor* actor)
{
    // Make blood drip down the wall
    if (actor->spr.yrepeat < 80)
    {
        actor->spr.yrepeat++;
        actor->spr.pos.Z += 0.5;
    }

    return 0;
}

// This is the FAST queue, it doesn't call any animator functions or states
void QueueGeneric(DSWActor* actor, short pic)
{
    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        KillActor(actor);
        return;
    }

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        KillActor(actor);
        return;
    }

    if (TestDontStickSector(actor->sector()))
    {
        KillActor(actor);
        return;
    }

    auto xrepeat = actor->spr.xrepeat;
    auto yrepeat = actor->spr.yrepeat;

    // can and should kill the user portion
    if (GenericQueue[GenericQueueHead] == nullptr)
    {
        actor->clearUser();
        change_actor_stat(actor, STAT_GENERIC_QUEUE);
        GenericQueue[GenericQueueHead] = actor;
    }
    else
    {
        // move old sprite to new sprite's place
        auto osp = GenericQueue[GenericQueueHead];
        osp->spr.pos = actor->spr.pos;
        ChangeActorSect(osp, actor->sector());
        KillActor(actor);
        actor = GenericQueue[GenericQueueHead];
        ASSERT(actor->spr.statnum != MAXSTATUS);
    }

    actor->spr.picnum = pic;
    actor->spr.xrepeat = xrepeat;
    actor->spr.yrepeat = yrepeat;
    actor->spr.cstat = 0;
    switch (actor->spr.picnum)
    {
    case 900:
    case 901:
    case 902:
    case 915:
    case 916:
    case 917:
    case 930:
    case 931:
    case 932:
    case GORE_Head:
        change_actor_stat(actor,STAT_DEFAULT); // Breakable
        actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
        actor->spr.extra |= (SPRX_BREAKABLE);
        break;
    default:
        actor->spr.cstat &= ~(CSTAT_SPRITE_BREAKABLE);
        actor->spr.extra &= ~(SPRX_BREAKABLE);
        break;
    }

    GenericQueueHead = (GenericQueueHead+1) & (MAX_GENERIC_QUEUE-1);
}

#if 0
int DoShellShrap(DSWActor* actor)
{
    // If the shell doesn't fall in the allowable range, kill it.
    if (actor->user.ShellNum < (ShellCount-MAXSHELLS))
    {
        KillActor(actor);
        return 0;
    }

    // Get rid of shell if they fall in non-divable liquid areas
    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        KillActor(actor);
        return 0;
    }

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        KillActor(actor);
        return 0;
    }

    return 0;
}
#endif

int DoShrapVelocity(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER) || SpriteInUnderwaterArea(actor))
    {
        ScaleSpriteVector(actor, 20000);

        actor->user.Counter += 8*4;      // These are MoveSkip4 now
        actor->user.change.Z += actor->user.Counter;
    }
    else
    {
        actor->user.Counter += 60*4;
        actor->user.change.Z += actor->user.Counter;
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitVoid:
            KillActor(actor);
            return true;
        case kHitSprite:
        {
            short wall_ang;

            auto hit_sprite = actor->user.coll.actor();

            wall_ang = NORM_ANGLE(hit_sprite->int_ang());
            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 32000);

            break;
        }

        case kHitWall:
        {
            int wall_ang = NORM_ANGLE(getangle(actor->user.coll.hitWall->delta())+512);

            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 32000);
            break;
        }

        case kHitSector:
        {
            bool did_hit_wall;

            if (SlopeBounce(actor, &did_hit_wall))
            {
                if (did_hit_wall)
                {
                    // hit a wall
                    ScaleSpriteVector(actor, 28000);
                    actor->user.coll.setNone();
                    actor->user.Counter = 0;
                }
                else
                {
                    // hit a sector
                    if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                    {
                        // hit a floor
                        if (!(actor->user.Flags & SPR_BOUNCE))
                        {
                            actor->user.Flags |= (SPR_BOUNCE);
                            ScaleSpriteVector(actor, 18000);
                            actor->user.coll.setNone();
                            actor->user.Counter = 0;
                        }
                        else
                        {
                            if (actor->user.ID == GORE_Drip)
                                ChangeState(actor, s_GoreFloorSplash);
                            else
                                ShrapKillSprite(actor);
                            return true;
                        }
                    }
                    else
                    {
                        // hit a ceiling
                        ScaleSpriteVector(actor, 22000);
                    }
                }
            }
            else
            {
                // hit floor
                if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
                {
                    actor->spr.pos.Z = actor->user.loz;
                    if (actor->user.Flags & (SPR_UNDERWATER))
                        actor->user.Flags |= (SPR_BOUNCE); // no bouncing underwater

                    if (actor->user.lo_sectp && actor->sector()->hasU() && FixedToInt(actor->sector()->depth_fixed))
                        actor->user.Flags |= (SPR_BOUNCE); // no bouncing on shallow water

                    if (!(actor->user.Flags & SPR_BOUNCE))
                    {
                        actor->user.Flags |= (SPR_BOUNCE);
                        actor->user.coll.setNone();
                        actor->user.Counter = 0;
                        actor->user.change.Z = -actor->user.change.Z;
                        ScaleSpriteVector(actor, 18000);
                        switch (actor->user.ID)
                        {
                        case UZI_SHELL:
                            PlaySound(DIGI_SHELL, actor, v3df_none);
                            break;
                        case SHOT_SHELL:
                            PlaySound(DIGI_SHOTSHELLSPENT, actor, v3df_none);
                            break;
                        }
                    }
                    else
                    {
                        if (actor->user.ID == GORE_Drip)
                            ChangeState(actor, s_GoreFloorSplash);
                        else
                            ShrapKillSprite(actor);
                        return true;
                    }
                }
                else
                // hit something above
                {
                    actor->user.change.Z = -actor->user.change.Z;
                    ScaleSpriteVector(actor, 22000);
                }
            }
            break;
        }
        }
    }

    // just outright kill it if its boucing around alot
    if (actor->user.bounce > 10)
    {
        KillActor(actor);
        return true;
    }

    return false;
}


int ShrapKillSprite(DSWActor* actor)
{
    short rnd_num;

    rnd_num = RandomRange(1024);

    ASSERT(actor->hasU());

    switch (actor->user.ID)
    {
    case UZI_SHELL:
        if (rnd_num > 854)
        {
            QueueGeneric(actor,UZI_SHELL + 0);
        }
        else if (rnd_num > 684)
        {
            QueueGeneric(actor,UZI_SHELL + 1);
        }
        else if (rnd_num > 514)
        {
            QueueGeneric(actor,UZI_SHELL + 2);
        }
        else if (rnd_num > 344)
        {
            QueueGeneric(actor,UZI_SHELL + 3);
        }
        else if (rnd_num > 174)
        {
            QueueGeneric(actor,UZI_SHELL + 4);
        }
        else
        {
            QueueGeneric(actor,UZI_SHELL + 5);
        }

        return 0;
        break;
    case SHOT_SHELL:
        if (rnd_num > 683)
        {
            QueueGeneric(actor,SHOT_SHELL + 1);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(actor,SHOT_SHELL + 3);
        }
        else
        {
            QueueGeneric(actor,SHOT_SHELL + 7);
        }
        return 0;
        break;
    case GORE_Lung:
        if (RandomRange(1000) > 500) break;
        actor->spr.clipdist = ActorSizeX(actor);
        SpawnFloorSplash(actor);
        if (RandomRange(1000) < 500)
            PlaySound(DIGI_GIBS1, actor, v3df_none);
        else
            PlaySound(DIGI_GIBS2, actor, v3df_none);
        if (rnd_num > 683)
        {
            QueueGeneric(actor,900);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(actor,901);
        }
        else
        {
            QueueGeneric(actor,902);
        }
        return 0;
        break;
    case GORE_Liver:
        if (RandomRange(1000) > 500) break;
        actor->spr.clipdist = ActorSizeX(actor);
        SpawnFloorSplash(actor);
        if (RandomRange(1000) < 500)
            PlaySound(DIGI_GIBS1, actor, v3df_none);
        else
            PlaySound(DIGI_GIBS2, actor, v3df_none);
        if (rnd_num > 683)
        {
            QueueGeneric(actor,915);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(actor,916);
        }
        else
        {
            QueueGeneric(actor,917);
        }
        return 0;
        break;
    case GORE_SkullCap:
        if (RandomRange(1000) > 500) break;
        actor->spr.clipdist = ActorSizeX(actor);
        SpawnFloorSplash(actor);
        if (rnd_num > 683)
        {
            QueueGeneric(actor,930);
        }
        else if (rnd_num > 342)
        {
            QueueGeneric(actor,931);
        }
        else
        {
            QueueGeneric(actor,932);
        }
        return 0;
        break;
    case GORE_Head:
        if (RandomRange(1000) > 500) break;
        actor->spr.clipdist = ActorSizeX(actor);
        QueueFloorBlood(actor);
        QueueGeneric(actor,GORE_Head);
        return 0;
        break;
    }

    // If it wasn't in the switch statement, kill it.
    KillActor(actor);

    return 0;
}

bool CheckBreakToughness(BREAK_INFO* break_info, int ID)
{
    if ((break_info->flags & BF_TOUGH))
    {
        switch (ID)
        {
        case LAVA_BOULDER:
        case MIRV_METEOR:
        case SERP_METEOR:
        case BOLT_THINMAN_R0:
        case BOLT_THINMAN_R1:
        case BOLT_THINMAN_R2:
        case BOLT_EXP:
        case TANK_SHELL_EXP:
        case GRENADE_EXP:
        case MICRO_EXP:
        case MINE_EXP:
        case NAP_EXP:
        case SKULL_R0:
        case BETTY_R0:
        case SKULL_SERP:
        case FIREBALL1:
        case GORO_FIREBALL:
            return true;   // All the above stuff will break tough things
        }
        return false;   // False means it won't break with current weapon
    }

    return true;   // It wasn't tough, go ahead and break it
}

int DoItemFly(DSWActor* actor)
{
    if (actor->user.Flags & (SPR_UNDERWATER))
    {
        ScaleSpriteVector(actor, 50000);

        actor->user.Counter += 20*2;
        actor->user.change.Z += actor->user.Counter;
    }
    else
    {
        actor->user.Counter += 60*2;
        actor->user.change.Z += actor->user.Counter;
    }

    actor->user.coll = move_missile(actor, actor->user.change.X, actor->user.change.Y, actor->user.change.Z,
                          actor->user.int_ceiling_dist(), actor->user.int_floor_dist(), CLIPMASK_MISSILE, MISSILEMOVETICS*2);

    MissileHitDiveArea(actor);

    {
        switch (actor->user.coll.type)
        {
        case kHitSprite:
        {
            short wall_ang;
            auto hit_sprite = actor->user.coll.actor();

            if ((hit_sprite->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL))
            {
                wall_ang = NORM_ANGLE(hit_sprite->int_ang());
                WallBounce(actor, wall_ang);
                ScaleSpriteVector(actor, 32000);
            }
            else
            {
                actor->user.change.X = -actor->user.change.X;
                actor->user.change.Y = -actor->user.change.Y;
            }

            break;
        }

        case kHitWall:
        {
            int wall_ang = NORM_ANGLE(getangle(actor->user.coll.hitWall->delta())+512);
            WallBounce(actor, wall_ang);
            ScaleSpriteVector(actor, 32000);
            break;
        }

        case kHitSector:
        {
            // hit floor
            if (actor->spr.pos.Z > ((actor->user.hiz + actor->user.loz) * 0.5))
            {
                actor->spr.pos.Z = actor->user.loz;
                actor->user.Counter = 0;
                actor->spr.xvel = 0;
                actor->user.change.Z = actor->user.change.X = actor->user.change.Y = 0;
                return false;
            }
            else
            // hit something above
            {
                actor->user.change.Z = -actor->user.change.Z;
                ScaleSpriteVector(actor, 22000);
            }
            break;
        }
        }
    }

    return true;
}

// This is the FAST queue, it doesn't call any animator functions or states
void QueueLoWangs(DSWActor* actor)
{
    DSWActor* spawnedActor;

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_WATER)
    {
        return;
    }

    if ((actor->sector()->extra & SECTFX_LIQUID_MASK) == SECTFX_LIQUID_LAVA)
    {
        return;
    }

    if (TestDontStickSector(actor->sector()))
    {
        return;
    }

    if (LoWangsQueue[LoWangsQueueHead] == nullptr)
    {
        LoWangsQueue[LoWangsQueueHead] = spawnedActor =
                                             SpawnActor(STAT_GENERIC_QUEUE, actor->spr.picnum, s_DeadLoWang, actor->sector(),
                                                         actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->int_ang(), 0);
    }
    else
    {
        // move old sprite to new sprite's place
        SetActorZ(LoWangsQueue[LoWangsQueueHead], actor->int_pos());
        spawnedActor = LoWangsQueue[LoWangsQueueHead];
        ASSERT(spawnedActor->spr.statnum != MAXSTATUS);
    }

    // Point passed in sprite to ps
    spawnedActor->spr.cstat = 0;
    spawnedActor->spr.xrepeat = actor->spr.xrepeat;
    spawnedActor->spr.yrepeat = actor->spr.yrepeat;
    spawnedActor->spr.shade = actor->spr.shade;
    spawnedActor->user.spal = spawnedActor->spr.pal = actor->spr.pal;
    change_actor_stat(spawnedActor, STAT_DEFAULT); // Breakable
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
    spawnedActor->spr.extra |= SPRX_BREAKABLE;
    spawnedActor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN);

    LoWangsQueueHead = (LoWangsQueueHead+1) & (MAX_LOWANGS_QUEUE-1);
}


#include "saveable.h"

static saveable_code saveable_weapon_code[] =
{
    SAVE_CODE(SpawnShrapX),
    SAVE_CODE(DoLavaErupt),
    SAVE_CODE(DoVomit),
    SAVE_CODE(DoVomitSplash),
    SAVE_CODE(DoFastShrapJumpFall),
    SAVE_CODE(DoTracerShrap),
    SAVE_CODE(DoShrapJumpFall),
    SAVE_CODE(DoShrapDamage),
    SAVE_CODE(DoUziSmoke),
    SAVE_CODE(DoShotgunSmoke),
    SAVE_CODE(DoMineSpark),
    SAVE_CODE(DoFireballFlames),
    SAVE_CODE(DoBreakFlames),
    SAVE_CODE(DoActorScale),
    SAVE_CODE(DoRipperGrow),
    SAVE_CODE(DoDamageTest),
    SAVE_CODE(DoStar),
    SAVE_CODE(DoCrossBolt),
    SAVE_CODE(DoPlasmaDone),
    SAVE_CODE(DoPlasmaFountain),
    SAVE_CODE(DoPlasma),
    SAVE_CODE(DoCoolgFire),
    SAVE_CODE(DoEelFire),
    SAVE_CODE(DoGrenade),
    SAVE_CODE(DoVulcanBoulder),
    SAVE_CODE(DoMineStuck),
    SAVE_CODE(DoMine),
    SAVE_CODE(DoPuff),
    SAVE_CODE(DoRailPuff),
    SAVE_CODE(DoBoltThinMan),
    SAVE_CODE(DoTracer),
    SAVE_CODE(DoEMP),
    SAVE_CODE(DoEMPBurst),
    SAVE_CODE(DoTankShell),
    SAVE_CODE(DoTracerStart),
    SAVE_CODE(DoLaser),
    SAVE_CODE(DoLaserStart),
    SAVE_CODE(DoRail),
    SAVE_CODE(DoRailStart),
    SAVE_CODE(DoRocket),
    SAVE_CODE(DoMicroMini),
    SAVE_CODE(SpawnExtraMicroMini),
    SAVE_CODE(DoMicro),
    SAVE_CODE(DoUziBullet),
    SAVE_CODE(DoBoltSeeker),
    SAVE_CODE(DoBoltShrapnel),
    SAVE_CODE(DoBoltFatMan),
    SAVE_CODE(DoElectro),
    SAVE_CODE(DoLavaBoulder),
    SAVE_CODE(DoSpear),
    SAVE_CODE(SpawnGrenadeSmallExp),
    SAVE_CODE(SpawnGrenadeExp),
    SAVE_CODE(SpawnMineExp),
    SAVE_CODE(DoSectorExp),
    SAVE_CODE(SpawnSectorExp),
    SAVE_CODE(SpawnLargeExp),
    SAVE_CODE(SpawnMeteorExp),
    SAVE_CODE(SpawnLittleExp),
    SAVE_CODE(DoFireball),
    SAVE_CODE(DoFindGround),
    SAVE_CODE(DoFindGroundPoint),
    SAVE_CODE(DoNapalm),
    SAVE_CODE(DoBloodWorm),
    SAVE_CODE(DoMeteor),
    SAVE_CODE(DoSerpMeteor),
    SAVE_CODE(DoMirvMissile),
    SAVE_CODE(DoMirv),
    SAVE_CODE(DoRing),
    SAVE_CODE(DoSerpRing),
    SAVE_CODE(InitLavaFlame),
    SAVE_CODE(InitLavaThrow),
    SAVE_CODE(InitVulcanBoulder),
    SAVE_CODE(InitSerpRing),
    SAVE_CODE(InitSerpRing),
    SAVE_CODE(InitSpellNapalm),
    SAVE_CODE(InitEnemyNapalm),
    SAVE_CODE(InitSpellMirv),
    SAVE_CODE(InitEnemyMirv),
    SAVE_CODE(InitSwordAttack),
    SAVE_CODE(InitFistAttack),
    SAVE_CODE(InitSumoNapalm),
    SAVE_CODE(InitSumoSkull),
    SAVE_CODE(InitSumoStompAttack),
    SAVE_CODE(InitMiniSumoClap),
    SAVE_CODE(InitZillaRail),
    SAVE_CODE(InitEnemyNuke),
    SAVE_CODE(InitRipperSlash),
    SAVE_CODE(InitBunnySlash),
    SAVE_CODE(InitSerpSlash),
    SAVE_CODE(InitCoolgBash),
    SAVE_CODE(InitSkelSlash),
    SAVE_CODE(InitGoroChop),
    SAVE_CODE(InitHornetSting),
    SAVE_CODE(InitSerpSpell),
    SAVE_CODE(InitSerpMonstSpell),
    SAVE_CODE(DoTeleRipper),
    SAVE_CODE(InitEnemyRocket),
    SAVE_CODE(InitEnemyRail),
    SAVE_CODE(InitZillaRocket),
    SAVE_CODE(InitEnemyStar),
    SAVE_CODE(InitEnemyCrossbow),
    SAVE_CODE(InitSkelSpell),
    SAVE_CODE(InitCoolgFire),
    SAVE_CODE(InitCoolgDrip),
    SAVE_CODE(GenerateDrips),
    SAVE_CODE(InitEelFire),
    SAVE_CODE(InitFireballTrap),
    SAVE_CODE(InitBoltTrap),
    SAVE_CODE(InitEnemyCrossbow),
    SAVE_CODE(InitSpearTrap),
    SAVE_CODE(DoSuicide),
    SAVE_CODE(DoDefaultStat),
    SAVE_CODE(InitEnemyUzi),
    SAVE_CODE(InitSpriteGrenade),
    SAVE_CODE(InitEnemyMine),
    SAVE_CODE(InitEnemyFireball),
    SAVE_CODE(DoVehicleSmoke),
    SAVE_CODE(DoWaterSmoke),
    SAVE_CODE(SpawnVehicleSmoke),
    SAVE_CODE(SpawnSmokePuff),
    SAVE_CODE(DoBubble),
    SAVE_CODE(DoFloorBlood),
    SAVE_CODE(DoWallBlood),
    SAVE_CODE(DoItemFly),
};

static saveable_data saveable_weapon_data[] =
{
    SAVE_DATA(s_NotRestored),
    SAVE_DATA(s_Suicide),
    SAVE_DATA(s_DeadLoWang),
    SAVE_DATA(s_BreakLight),
    SAVE_DATA(s_BreakBarrel),
    SAVE_DATA(s_BreakPedistal),
    SAVE_DATA(s_BreakBottle1),
    SAVE_DATA(s_BreakBottle2),
    SAVE_DATA(s_Puff),
    SAVE_DATA(s_RailPuff),
    SAVE_DATA(sg_RailPuff),
    SAVE_DATA(s_LaserPuff),
    SAVE_DATA(s_Tracer),
    SAVE_DATA(s_EMP),
    SAVE_DATA(s_EMPBurst),
    SAVE_DATA(s_EMPShrap),
    SAVE_DATA(s_TankShell),
    SAVE_DATA(s_VehicleSmoke),
    SAVE_DATA(s_WaterSmoke),
    SAVE_DATA(s_UziSmoke),
    SAVE_DATA(s_ShotgunSmoke),
    SAVE_DATA(s_UziBullet),
    SAVE_DATA(s_UziSpark),
    SAVE_DATA(s_UziPowerSpark),
    SAVE_DATA(s_Bubble),
    SAVE_DATA(s_Splash),
    SAVE_DATA(s_CrossBolt),
    SAVE_DATA(sg_CrossBolt),
    SAVE_DATA(s_Star),
    SAVE_DATA(s_StarStuck),
    SAVE_DATA(s_StarDown),
    SAVE_DATA(s_StarDownStuck),
    SAVE_DATA(s_LavaBoulder),
    SAVE_DATA(s_LavaShard),
    SAVE_DATA(s_VulcanBoulder),
    SAVE_DATA(s_Grenade),
    SAVE_DATA(s_Grenade),
    SAVE_DATA(sg_Grenade),
    SAVE_DATA(s_MineStuck),
    SAVE_DATA(s_Mine),
    SAVE_DATA(s_MineSpark),
    SAVE_DATA(s_Meteor),
    SAVE_DATA(sg_Meteor),
    SAVE_DATA(s_MeteorExp),
    SAVE_DATA(s_MirvMeteor),
    SAVE_DATA(sg_MirvMeteor),
    SAVE_DATA(s_MirvMeteorExp),
    SAVE_DATA(s_SerpMeteor),
    SAVE_DATA(sg_SerpMeteor),
    SAVE_DATA(s_SerpMeteorExp),
    SAVE_DATA(s_Spear),
    SAVE_DATA(sg_Spear),
    SAVE_DATA(s_Rocket),
    SAVE_DATA(sg_Rocket),
    SAVE_DATA(s_BunnyRocket),
    SAVE_DATA(sg_BunnyRocket),
    SAVE_DATA(s_Rail),
    SAVE_DATA(sg_Rail),
    SAVE_DATA(s_Laser),
    //SAVE_DATA(s_MicroPuff),
    SAVE_DATA(s_Micro),
    SAVE_DATA(sg_Micro),
    SAVE_DATA(s_MicroMini),
    SAVE_DATA(sg_MicroMini),
    SAVE_DATA(s_BoltThinMan),
    SAVE_DATA(sg_BoltThinMan),
    SAVE_DATA(s_BoltSeeker),
    SAVE_DATA(sg_BoltSeeker),
    SAVE_DATA(s_BoltFatMan),
    SAVE_DATA(s_BoltShrapnel),
    SAVE_DATA(s_CoolgFire),
    SAVE_DATA(s_CoolgFireDone),
    SAVE_DATA(s_CoolgDrip),
    SAVE_DATA(s_GoreFloorSplash),
    SAVE_DATA(s_GoreSplash),
    SAVE_DATA(s_Plasma),
    SAVE_DATA(s_PlasmaFountain),
    SAVE_DATA(s_PlasmaDrip),
    SAVE_DATA(s_PlasmaDone),
    SAVE_DATA(s_TeleportEffect),
    SAVE_DATA(s_TeleportEffect2),
    SAVE_DATA(s_Electro),
    SAVE_DATA(s_ElectroShrap),
    SAVE_DATA(s_GrenadeExp),
    SAVE_DATA(s_GrenadeSmallExp),
    SAVE_DATA(s_GrenadeExp),
    SAVE_DATA(s_MineExp),
    SAVE_DATA(s_BasicExp),
    SAVE_DATA(s_MicroExp),
    SAVE_DATA(s_BigGunFlame),
    SAVE_DATA(s_BoltExp),
    SAVE_DATA(s_TankShellExp),
    SAVE_DATA(s_TracerExp),
    SAVE_DATA(s_SectorExp),
    SAVE_DATA(s_FireballExp),
    SAVE_DATA(s_NapExp),
    SAVE_DATA(s_FireballFlames),
    SAVE_DATA(s_BreakFlames),
    SAVE_DATA(s_Fireball),
    SAVE_DATA(s_Fireball),
    //SAVE_DATA(sg_Fireball),
    SAVE_DATA(s_Ring),
    //SAVE_DATA(sg_Ring),
    SAVE_DATA(s_Ring),
    SAVE_DATA(s_Ring2),
    SAVE_DATA(s_Napalm),
    SAVE_DATA(s_BloodWorm),
    SAVE_DATA(s_BloodWorm),
    SAVE_DATA(s_PlasmaExp),
    SAVE_DATA(s_PlasmaExp),
    SAVE_DATA(s_Mirv),
    SAVE_DATA(s_MirvMissile),
    SAVE_DATA(s_Vomit1),
    SAVE_DATA(s_Vomit2),
    SAVE_DATA(s_VomitSplash),
    SAVE_DATA(s_GoreHead),
    SAVE_DATA(s_GoreLeg),
    SAVE_DATA(s_GoreEye),
    SAVE_DATA(s_GoreTorso),
    SAVE_DATA(s_GoreArm),
    SAVE_DATA(s_GoreLung),
    SAVE_DATA(s_GoreLiver),
    SAVE_DATA(s_GoreSkullCap),
    SAVE_DATA(s_GoreChunkS),
    SAVE_DATA(s_GoreDrip),
    SAVE_DATA(s_FastGoreDrip),
    SAVE_DATA(s_GoreFlame),
    SAVE_DATA(s_TracerShrap),
    SAVE_DATA(s_UziShellShrap),
    SAVE_DATA(s_UziShellShrapStill1),
    SAVE_DATA(s_UziShellShrapStill2),
    SAVE_DATA(s_UziShellShrapStill3),
    SAVE_DATA(s_UziShellShrapStill4),
    SAVE_DATA(s_UziShellShrapStill5),
    SAVE_DATA(s_UziShellShrapStill6),
    SAVE_DATA(s_ShotgunShellShrap),
    SAVE_DATA(s_ShotgunShellShrapStill1),
    SAVE_DATA(s_ShotgunShellShrapStill2),
    SAVE_DATA(s_ShotgunShellShrapStill3),
    SAVE_DATA(s_GoreFlameChunkA),
    SAVE_DATA(s_GoreFlameChunkB),
    SAVE_DATA(s_CoinShrap),
    SAVE_DATA(s_Marbel),
    SAVE_DATA(s_GlassShrapA),
    SAVE_DATA(s_GlassShrapB),
    SAVE_DATA(s_GlassShrapC),
    SAVE_DATA(s_WoodShrapA),
    SAVE_DATA(s_WoodShrapB),
    SAVE_DATA(s_WoodShrapC),
    SAVE_DATA(s_StoneShrapA),
    SAVE_DATA(s_StoneShrapB),
    SAVE_DATA(s_StoneShrapC),
    SAVE_DATA(s_MetalShrapA),
    SAVE_DATA(s_MetalShrapB),
    SAVE_DATA(s_MetalShrapC),
    SAVE_DATA(s_PaperShrapA),
    SAVE_DATA(s_PaperShrapB),
    SAVE_DATA(s_PaperShrapC),
    SAVE_DATA(s_FloorBlood1),
    SAVE_DATA(s_FootPrint1),
    SAVE_DATA(s_FootPrint2),
    SAVE_DATA(s_FootPrint3),
    SAVE_DATA(s_WallBlood1),
    SAVE_DATA(s_WallBlood2),
    SAVE_DATA(s_WallBlood3),
    SAVE_DATA(s_WallBlood4),
};

saveable_module saveable_weapon =
{
    // code
    saveable_weapon_code,
    SIZ(saveable_weapon_code),

    // data
    saveable_weapon_data,
    SIZ(saveable_weapon_data)
};
END_SW_NS
