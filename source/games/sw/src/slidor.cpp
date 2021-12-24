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
#include "game.h"
#include "network.h"
#include "tags.h"
#include "sector.h"
#include "interpolate.h"
#include "misc.h"
#include "sprite.h"
#include "quotemgr.h"

BEGIN_SW_NS

void ReverseSlidor(DSWActor* actor)
{
    USERp u = actor->u();
    ROTATORp r;

    r = u->rotator.Data();

    // if paused go ahead and start it up again
    if (u->Tics)
    {
        u->Tics = 0;
        SetSlidorActive(actor);
        return;
    }

    // moving toward to OFF pos
    if (r->tgt == 0)
    {
        r->tgt = r->open_dest;
    }
    else if (r->tgt == r->open_dest)
    {
        r->tgt = 0;
    }

    r->vel = -r->vel;
}


bool SlidorSwitch(short match, short setting)
{
    SPRITEp sp;
    bool found = false;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        sp = &actor->s();

        if (sp->lotag == TAG_SPRITE_SWITCH_VATOR && sp->hitag == match)
        {
            found = true;
            AnimateSwitch(sp, setting);
        }
    }

    return found;
}

void SetSlidorActive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;

    r = u->rotator.Data();

    DoSlidorInterp(actor, StartInterpolation);

    // play activate sound
    DoSoundSpotMatch(SP_TAG2(sp), 1, SOUND_OBJECT_TYPE);

    SET(u->Flags, SPR_ACTIVE);
    u->Tics = 0;

    // moving to the OFF position
    if (r->tgt == 0)
        VatorSwitch(SP_TAG2(sp), OFF);
    else
        VatorSwitch(SP_TAG2(sp), ON);
}

void SetSlidorInactive(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();

    DoSlidorInterp(actor, StopInterpolation);

    // play inactivate sound
    DoSoundSpotMatch(SP_TAG2(sp), 2, SOUND_OBJECT_TYPE);

    RESET(u->Flags, SPR_ACTIVE);
}

// called for operation from the space bar
void DoSlidorOperate(PLAYERp pp, sectortype* sect)
{
    short match;

    match = sect->hitag;


    if (match > 0)
    {
        if (!TestSlidorMatchActive(match))
            DoSlidorMatch(pp, match, true);
    }
}

// called from switches and triggers
// returns first vator found
void DoSlidorMatch(PLAYERp pp, short match, bool manual)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_SLIDOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(actor) == SECT_SLIDOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // single play only vator
            // bool 8 must be set for message to display
            if (TEST_BOOL4(fsp) && (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS))
            {
                if (pp && TEST_BOOL11(fsp)) PutStringInfo(pp, GStrings("TXTS_SPONLY"));
                continue;
            }

            // switch trigger only
            if (SP_TAG3(fsp) == 1)
            {
                // tried to manually operat a switch/trigger only
                if (manual)
                    continue;
            }

            auto sect = fsp->sector();

            if (pp && sect->hasU() && sect->stag == SECT_LOCK_DOOR && sect->number)
            {
                int key_num = sect->number;

                {
                    PutStringInfo(pp, quoteMgr.GetQuote(QUOTE_DOORMSG + key_num - 1));
                    return;
                }
            }

            if (TEST(fu->Flags, SPR_ACTIVE))
            {
                ReverseSlidor(actor);
                continue;
            }

            SetSlidorActive(actor);
        }
    }
}


bool TestSlidorMatchActive(short match)
{
    USERp fu;
    SPRITEp fsp;

    SWStatIterator it(STAT_SLIDOR);
    while (auto actor = it.Next())
    {
        fsp = &actor->s();

        if (SP_TAG1(actor) == SECT_SLIDOR && SP_TAG2(fsp) == match)
        {
            fu = actor->u();

            // Does not have to be inactive to be operated
            if (TEST_BOOL6(fsp))
                continue;

            if (TEST(fu->Flags, SPR_ACTIVE) || fu->Tics)
                return true;
        }
    }

    return false;
}

void DoSlidorInterp(DSWActor* actor, INTERP_FUNC interp_func)
{
    auto sp = actor->spr.sector();

    // this code is just weird.
    auto startWall = sp->firstWall();
    auto endWall = sp->lastWall();
    auto wal = startWall;
    do
    {
        EInterpolationType type = Interp_Invalid;;
        switch (wal->lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:
        case TAG_WALL_SLIDOR_RIGHT:
            type = Interp_Wall_X;
            break;
        case TAG_WALL_SLIDOR_UP:
        case TAG_WALL_SLIDOR_DOWN:
            type = Interp_Wall_Y;
            break;
        }
        if (type != Interp_Invalid)
        {
            auto pwal = wal - 1;
            if (wal < startWall) // original code - this makes no sense as in a correctly formed sector this condition is never true.
                pwal = endWall;

            // prev wall
            if (!wal->twoSided())
            {
                // white wall - move 4 points
                interp_func(wal, type);
                interp_func(pwal, type);
                interp_func(wal->point2Wall(), type);
                interp_func(wal->point2Wall()->point2Wall(), type);
            }
            else
            {
                // red wall - move 2 points
                interp_func(wal, type);
                interp_func(wal->nextWall()->point2Wall(), type);
                interp_func(wal->point2Wall(), type);
                if (wal->point2Wall()->twoSided())
                    interp_func(wal->point2Wall()->nextWall()->point2Wall(), type);
            }
        }
        wal = wal->point2Wall();
    }
    while (wal != startWall);
}

int DoSlidorMoveWalls(DSWActor* actor, int amt)
{
    auto sp = actor->spr.sector();

    // this code is just weird.
    auto startWall = sp->firstWall();
    auto endWall = sp->lastWall();
    auto wal = startWall;
    walltype* pwal;

    do
    {
        switch (wal->lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:

            // prev wall
            pwal = wal - 1;
            if (wal < startWall) // original code - this makes no sense as in a correctly formed sector this condition is never true.
                pwal = endWall;

            if (!wal->twoSided())
            {
                // white wall - move 4 points
                wal->move(wal->pos.X - amt, wal->pos.Y);
                pwal->move(pwal->pos.X - amt, pwal->pos.Y);
                wal->point2Wall()->move(wal->point2Wall()->pos.X - amt, wal->point2Wall()->pos.Y);
                auto pwal2 = wal->point2Wall()->point2Wall();
                pwal2->move(pwal2->pos.X - amt, pwal2->pos.Y);
            }
            else
            {
                // red wall - move 2 points
                dragpoint(wal, wal->pos.X - amt, wal->pos.Y);
                dragpoint(wal->point2Wall(), wal->point2Wall()->pos.X - amt, wal->point2Wall()->pos.Y);
            }

            break;

        case TAG_WALL_SLIDOR_RIGHT:

            // prev wall
            pwal = wal - 1;
            if (wal < startWall) // original code - this makes no sense as in a correctly formed sector this condition is never true.
                pwal = endWall;

            if (!wal->twoSided())
            {
                // white wall - move 4 points
                wal->move(wal->pos.X + amt, wal->pos.Y);
                pwal->move(pwal->pos.X + amt, pwal->pos.Y);
                wal->point2Wall()->move(wal->point2Wall()->pos.X + amt, wal->point2Wall()->pos.Y);
                auto pwal2 = wal->point2Wall()->point2Wall();
                pwal2->move(pwal2->pos.X + amt, pwal2->pos.Y);
            }
            else
            {
                // red wall - move 2 points
                dragpoint(wal, wal->pos.X + amt, wal->pos.Y);
                dragpoint(wal->point2Wall(), wal->point2Wall()->pos.X + amt, wal->point2Wall()->pos.Y);
            }

            break;

        case TAG_WALL_SLIDOR_UP:

            // prev wall
            pwal = wal - 1;
            if (wal < startWall) // original code - this makes no sense as in a correctly formed sector this condition is never true.
                pwal = endWall;

            if (!wal->twoSided())
            {
                wal->move(wal->pos.X, wal->pos.Y - amt);
                pwal->move(pwal->pos.X, pwal->pos.Y - amt);
                wal->point2Wall()->move(wal->point2Wall()->pos.X, wal->point2Wall()->pos.Y - amt);
                auto pwal2 = wal->point2Wall()->point2Wall();
                pwal2->move(pwal2->pos.X, pwal2->pos.Y - amt);
            }
            else
            {
                dragpoint(wal, wal->pos.X, wal->pos.Y - amt);
                dragpoint(wal->point2Wall(), wal->point2Wall()->pos.X, wal->point2Wall()->pos.Y - amt);
            }

            break;

        case TAG_WALL_SLIDOR_DOWN:

            // prev wall
            pwal = wal - 1;
            if (wal < startWall) // original code - this makes no sense as in a correctly formed sector this condition is never true.
                pwal = endWall;

            if (!wal->twoSided())
            {
                wal->move(wal->pos.X, wal->pos.Y + amt);
                pwal->move(pwal->pos.X, pwal->pos.Y + amt);
                wal->point2Wall()->move(wal->point2Wall()->pos.X, wal->point2Wall()->pos.Y + amt);
                auto pwal2 = wal->point2Wall()->point2Wall();
                pwal2->move(pwal2->pos.X, pwal2->pos.Y + amt);
            }
            else
            {
                dragpoint(wal, wal->pos.X, wal->pos.Y + amt);
                dragpoint(wal->point2Wall(), wal->point2Wall()->pos.X, wal->point2Wall()->pos.Y + amt);
            }


            break;
        }


        wal = wal->point2Wall();
    }
    while (wal != startWall);

    return 0;
}

int DoSlidorInstantClose(DSWActor* actor)
{
    SPRITEp sp = &actor->s();
    int diff;

    auto startwall = sp->sector()->firstWall();
    auto wal = startwall;

    do
    {
        switch (wal->lotag)
        {
        case TAG_WALL_SLIDOR_LEFT:
            diff = wal->pos.X - sp->pos.X;
            DoSlidorMoveWalls(actor, diff);
            break;

        case TAG_WALL_SLIDOR_RIGHT:
            diff = wal->pos.X - sp->pos.X;
            DoSlidorMoveWalls(actor, -diff);
            break;

        case TAG_WALL_SLIDOR_UP:
            diff = wal->pos.Y - sp->pos.Y;
            DoSlidorMoveWalls(actor, diff);
            break;

        case TAG_WALL_SLIDOR_DOWN:
            diff = wal->pos.Y - sp->pos.Y;
            DoSlidorMoveWalls(actor, -diff);
            break;
        }

        wal = wal->point2Wall();
    }
    while (wal != startwall);

    return 0;
}


int DoSlidor(DSWActor* actor)
{
    USERp u = actor->u();
    SPRITEp sp = &actor->s();
    ROTATORp r;
    int old_pos;
    bool kill = false;

    r = u->rotator.Data();

    // Example - ang pos moves from 0 to 512 <<OR>> from 0 to -512

    old_pos = r->pos;

    // control SPEED of swinging
    if (r->pos < r->tgt)
    {
        // Increment swing angle
        r->pos += r->speed;
        r->speed += r->vel;

        // if the other way make it equal
        if (r->pos > r->tgt)
            r->pos = r->tgt;
    }

    if (r->pos > r->tgt)
    {
        // Increment swing angle
        r->pos -= r->speed;
        r->speed += r->vel;

        // if the other way make it equal
        if (r->pos < r->tgt)
            r->pos = r->tgt;
    }

    if (r->pos == r->tgt)
    {
        // If ang is OPEN
        if (r->pos == r->open_dest)
        {
            // new tgt is CLOSED (0)
            r->tgt = 0;
            r->vel = -r->vel;
            SetSlidorInactive(actor);

            if (SP_TAG6(sp) && !TEST_BOOL8(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);

            // wait a bit and close it
            if (u->WaitTics)
                u->Tics = u->WaitTics;
        }
        else
        // If ang is CLOSED then
        if (r->pos == 0)
        {
            short match = SP_TAG2(sp);

            // new tgt is OPEN (open)
            r->speed = r->orig_speed;
            r->vel = labs(r->vel);

            r->tgt = r->open_dest;
            SetSlidorInactive(actor);

            RESET_BOOL8(sp);

            // set Owner swith back to OFF
            // only if ALL vators are inactive
            if (!TestSlidorMatchActive(match))
            {
                //SlidorSwitch(match, OFF);
            }

            if (SP_TAG6(sp) && TEST_BOOL8(sp))
                DoMatchEverything(nullptr, SP_TAG6(sp), -1);
        }

        if (TEST_BOOL2(sp))
            kill = true;
    }
    else
    {
        // if heading for the OFF (original) position and should NOT CRUSH
        if (TEST_BOOL3(sp) && r->tgt == 0)
        {
            SPRITEp bsp;
            USERp bu;
            bool found = false;

            SWSectIterator it(sp->sector());
            while (auto itActor = it.Next())
            {
                bsp = &itActor->s();
                bu = itActor->u();

                if (bu && TEST(bsp->cstat, CSTAT_SPRITE_BLOCK) && TEST(bsp->extra, SPRX_PLAYER_OR_ENEMY))
                {
                    // found something blocking so reverse to ON position
                    ReverseSlidor(actor);
                    SET_BOOL8(sp); // tell vator that something blocking door
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                short pnum;
                PLAYERp pp;
                // go ahead and look for players clip box bounds
                TRAVERSE_CONNECT(pnum)
                {
                    pp = Player + pnum;

                    if (pp->lo_sectp == sp->sector() ||
                        pp->hi_sectp == sp->sector())
                    {
                        ReverseSlidor(actor);

                        u->vel_rate = -u->vel_rate;
                        found = true;
                    }
                }
            }
        }
    }


    DoSlidorMoveWalls(actor, r->pos - old_pos);

    if (kill)
    {
        SetSlidorInactive(actor);
        KillActor(actor);
        return 0;
    }

    return 0;
}

#include "saveable.h"

static saveable_code saveable_slidor_code[] =
{
    SAVE_CODE(DoSlidor),
};

saveable_module saveable_slidor =
{
    // code
    saveable_slidor_code,
    SIZ(saveable_slidor_code),

    // data
    nullptr,0
};
END_SW_NS
