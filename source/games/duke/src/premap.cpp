//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

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

#include "ns.h"
#include "global.h"
#include "mapinfo.h"
#include "secrets.h"
#include "statistics.h"
#include "gamestate.h"
#include "automap.h"
#include "dukeactor.h"
#include "interpolate.h"
#include "precache.h"
#include "render.h"
#include "screenjob_.h"

BEGIN_DUKE_NS  

int which_palookup = 9;

void premapcontroller(DDukeActor* ac)
{
	CallStaticSetup(ac);
	if (iseffector(ac))
	{
		ac->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_ALIGNMENT_MASK);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void pickrandomspot(int snum)
{
	DDukePlayer* p;
	int i;

	p = getPlayer(snum);

	if( ud.multimode > 1 && ud.coop != 1)
		i = krand()%numplayersprites;
	else i = snum;

	p->GetActor()->spr.pos = po[i].opos;
	p->GetActor()->backuppos();
	p->setbobpos();
	p->GetActor()->PrevAngles.Yaw = p->GetActor()->spr.Angles.Yaw = po[i].oa;
	p->setCursector(po[i].os);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetplayerstats(int snum)
{
	DDukePlayer* p;

	p = getPlayer(snum);

	gFullMap = 0; 
	p->pnum             = 0;
	p->dead_flag        = 0;
	p->wackedbyactor    = nullptr;
	p->falling_counter  = 0;
	p->quick_kick       = 0;
	p->subweapon        = 0;
	p->last_full_weapon = 0;
	p->ftq              = 0;
	p->otipincs = p->tipincs = 0;
	p->buttonpalette    = 0;
	p->actorsqu         =nullptr;
	p->invdisptime      = 0;
	p->refresh_inventory= 0;
	p->last_pissed_time = 0;
	p->holster_weapon   = 0;
	p->pycount          = 0;
	p->pyoff            = 0;
	p->opyoff           = 0;
	p->oloogcnt = p->loogcnt = 0;
	p->psectlotag       = 0;
	p->weapon_sway      = 0;
//    p->select_dir       = 0;
	p->extra_extra8     = 0;
	p->show_empty_weapon= 0;
	p->dummyplayersprite=nullptr;
	p->crack_time       = 0;
	p->hbomb_hold_delay = 0;
	p->transporter_hold = 0;
	p->wantweaponfire  = -1;
	p->hurt_delay       = 0;
	p->hurt_delay2      = 0;
	p->footprintcount   = 0;
	p->footprintpal     = 0;
	p->footprintshade   = 0;
	p->jumping_toggle   = 0;
	p->bobcounter       = 0;
	p->on_ground        = 0;
	p->player_par       = 0;
	p->cmd.ucmd.actions |= SB_CENTERVIEW;
	p->airleft          = 15*26;
	p->rapid_fire_hold  = 0;
	p->toggle_key_flag  = 0;
	p->access_spritenum = nullptr;
	if(ud.multimode > 1 && ud.coop != 1 )
		p->got_access = 7;
	else p->got_access      = 0;
	p->random_club_frame= 0;
	p->on_warping_sector = 0;
	p->spritebridge      = 0;

	if(p->steroids_amount < 400 )
	{
		p->steroids_amount = 0;
		p->inven_icon = 0;
	}
	p->heat_on =            0;
	p->jetpack_on =         0;
	p->holoduke_on =       nullptr;
	p->newOwner          =nullptr;
	p->jumping_counter   = 0;
	p->hard_landing      = 0;
	p->vel.Zero();
	p->fric.X            = 0;
	p->fric.Y            = 0;
	p->somethingonplayer = nullptr;

	p->on_crane          = nullptr;

	if(p->curr_weapon == PISTOL_WEAPON)
		p->okickback_pic = p->kickback_pic  = isRR()? 22 : 5;
	else p->okickback_pic = p->kickback_pic = 0;

	p->oweapon_pos = p->weapon_pos        = 6;
	p->walking_snd_toggle= 0;
	p->weapon_ang        = 0;

	p->knuckle_incs      = 1;
	p->ofist_incs = p->fist_incs = 0;
	p->oknee_incs = p->knee_incs = 0;
	p->stairs = 0;
	p->noise.X = 0;
	p->noise.Y = 0;
	p->donoise = 0;
	p->noise_radius = 0;
	if (isRR() && ud.multimode > 1 && ud.coop != 1)
	{
		p->keys[0] = 1;
		p->keys[1] = 1;
		p->keys[2] = 1;
		p->keys[3] = 1;
		p->keys[4] = 1;
	}
	else
	{
		p->keys[0] = 0;
		p->keys[1] = 0;
		p->keys[2] = 0;
		p->keys[3] = 0;
		p->keys[4] = 0;
	}
	wupass = 0;
	//p->at582 = 0;
	p->drunkang = 1647;
	p->eatang = 1647;
	p->drink_amt = 0;
	p->eat = 0;
	p->drink_timer = 4096;
	p->eat_timer = 4096;
	p->shotgun_state[0] = 0;
	p->shotgun_state[1] = 0;
	p->detonate_time = 0;
	p->detonate_count = 0;
	p->recoil = 0;
	p->yehaa_timer = 0;
	chickenphase = 0;
	if (p->OnMotorcycle)
	{
		p->OnMotorcycle = 0;
		p->gotweapon[MOTORCYCLE_WEAPON] = false;
		p->curr_weapon = isRRRA()? SLINGBLADE_WEAPON : KNEE_WEAPON;	// just in case this is made available for the other games
	}
	p->lotag800kill = 0;
	p->moto_do_bump = 0;
	p->MotoOnGround = 1;
	p->moto_underwater = 0;
	p->MotoSpeed = 0;
	p->TiltStatus = nullAngle;
	p->moto_drink = 0;
	p->VBumpTarget = 0;
	p->VBumpNow  =0;
	p->moto_bump_fast = 0;
	p->TurbCount = 0;
	p->moto_on_mud = 0;
	p->moto_on_oil = 0;
	if (p->OnBoat)
	{
		p->OnBoat = 0;
		p->gotweapon[BOAT_WEAPON] = false;
		p->curr_weapon = isRRRA()? SLINGBLADE_WEAPON : KNEE_WEAPON;	// just in case this is made available for the other games
	}
	p->NotOnWater = 0;
	p->SeaSick = 0;
	p->nocheat = 0;
	p->DrugMode = 0;
	p->drug_stat[0] = 0;
	p->drug_stat[1] = 0;
	p->drug_stat[2] = 0;
	p->drug_aspect = 0;
	resetlanepics();

	if (numplayers < 2)
	{
		ufospawn = isRRRA()? 3 : min(ud.player_skill*4+1, 32);
		ufocnt = 0;
		hulkspawn = ud.player_skill + 1;
	}
	else
	{
		ufospawn = isRRRA()? 0 :32;
		ufocnt = 0;
		hulkspawn = isRRRA()? 0 :2;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetweapons(DDukePlayer* p)
{
	for (int weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
	{
		p->ammo_amount[weapon] = 0;
	}

	memset(p->gotweapon, 0, MAX_WEAPONS);
	p->oweapon_pos = p->weapon_pos = 6;
	p->okickback_pic = p->kickback_pic = 5;
	p->curr_weapon = PISTOL_WEAPON;
	p->gotweapon[PISTOL_WEAPON] = true;
	p->gotweapon[KNEE_WEAPON] = true;
	p->ammo_amount[PISTOL_WEAPON] = min<int16_t>(gs.max_ammo_amount[PISTOL_WEAPON], 48);
	p->gotweapon[HANDREMOTE_WEAPON] = true;
	p->last_weapon = -1;

	p->show_empty_weapon= 0;
	p->last_pissed_time = 0;
	p->holster_weapon = 0;

	// Always clear these, even for non-RRRA
	p->OnMotorcycle = 0;
	p->moto_underwater = 0;
	p->OnBoat = 0;
	p->lotag800kill = 0;

	if (isRRRA())
	{
		chickenphase = 0;
		p->ammo_amount[KNEE_WEAPON] = 1;
		p->gotweapon[SLINGBLADE_WEAPON] = true;
		p->ammo_amount[SLINGBLADE_WEAPON] = 1;
	}
	OnEvent(EVENT_RESETWEAPONS, p->pnum, nullptr, -1);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetinventory(DDukePlayer* p)
{
	p->inven_icon = 0;
	p->boot_amount = 0;
	p->scuba_on = 0;
	p->scuba_amount = 0;
	p->heat_amount = 0;
	p->heat_on = 0;
	p->jetpack_on = 0;
	p->jetpack_amount = 0;
	p->shield_amount = gs.max_armour_amount;
	p->holoduke_on = nullptr;
	p->holoduke_amount = 0;
	p->firstaid_amount = 0;
	p->steroids_amount = 0;
	p->inven_icon = 0;

	if (isRR() && ud.multimode > 1 && ud.coop != 1)
	{
		p->keys[0] = 1;
		p->keys[1] = 1;
		p->keys[2] = 1;
		p->keys[3] = 1;
		p->keys[4] = 1;
	}
	else
	{
		p->keys[0] = 0;
		p->keys[1] = 0;
		p->keys[2] = 0;
		p->keys[3] = 0;
		p->keys[4] = 0;
	}

	p->drunkang = 1647;
	p->eatang = 1647;
	p->drink_amt = 0;
	p->eat = 0;
	p->drink_timer = 0;
	p->eat_timer = 0;
	p->shotgun_state[0] = 0;
	p->shotgun_state[1] = 0;
	p->detonate_time = 0;
	p->detonate_count = 0;
	p->recoil = 0;
	p->yehaa_timer = 0;
	resetlanepics();

	if (numplayers < 2)
	{
		ufospawn = min(ud.player_skill*4+1, 32);
		ufocnt = 0;
		hulkspawn = ud.player_skill + 1;
	}
	else
	{
		ufospawn = 32;
		ufocnt = 0;
		hulkspawn = 2;
	}
	OnEvent(EVENT_RESETINVENTORY, p->pnum, p->GetActor());
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetprestat(int snum,int g)
{
	DDukePlayer* p;

	p = getPlayer(snum);

	spriteqloc = 0;
	for(auto& p : spriteq) p = nullptr;

	p->hbomb_on          = 0;
	p->pals.a         = 0;
	p->toggle_key_flag   = 0;
	p->lastrandomspot = 0;
	p->oweapon_pos = p->weapon_pos = 6;
	p->okickback_pic = p->kickback_pic = 5;
	p->last_weapon = -1;
	p->weapreccnt = 0;
	p->show_empty_weapon= 0;
	p->holster_weapon = 0;
	p->last_pissed_time = 0;

	p->one_parallax_sectnum = nullptr;
	p->visibility = ud.const_visibility;

	screenpeek              = myconnectindex;
	numanimwalls            = 0;
	cyclers.Clear();
	animatecnt              = 0;
	randomseed              = 17L;
	paused             = 0;
	ud.cameraactor =nullptr;
	mspos.Clear();
	animates.Clear();
	camsprite               =nullptr;
	ud.earthquaketime          = 0;
	ud.joe9000 = false;

	WindTime = 0;
	WindDir = nullAngle;
	fakebubba_spawn = 0;
	RRRA_ExitedLevel = 0;

	if(p->curr_weapon == HANDREMOTE_WEAPON)
	{
		p->ammo_amount[HANDBOMB_WEAPON]++;
		p->curr_weapon = HANDBOMB_WEAPON;
	}

	p->timebeforeexit   = 0;
	p->customexitsound  = 0;

	p->stairs = 0;
	//if (!isRRRA()) p->fogtype = 0;
	p->noise.X = 8192;
	p->noise.Y = 8192;
	p->donoise = 0;
	p->noise_radius = 0;

	if (isRR() && ud.multimode > 1 && ud.coop != 1)
	{
		p->keys[0] = 1;
		p->keys[1] = 1;
		p->keys[2] = 1;
		p->keys[3] = 1;
		p->keys[4] = 1;
	}
	else
	{
		p->keys[0] = 0;
		p->keys[1] = 0;
		p->keys[2] = 0;
		p->keys[3] = 0;
		p->keys[4] = 0;
	}

	p->drunkang = 1647;
	p->eatang = 1647;
	p->drink_amt = 0;
	p->eat = 0;
	p->drink_timer = 0;
	p->eat_timer = 0;
	p->shotgun_state[0] = 0;
	p->shotgun_state[1] = 0;
	p->detonate_time = 0;
	p->detonate_count = 0;
	p->recoil = 0;
	p->yehaa_timer = 0;
	resetlanepics();

	if (numplayers < 2)
	{
		ufospawn = min(ud.player_skill*4+1, 32);
		ufocnt = 0;
		hulkspawn = ud.player_skill + 1;
	}
	else
	{
		ufospawn = 32;
		ufocnt = 0;
		hulkspawn = 2;
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetpspritevars(int g, const DVector3& startpos, const DAngle startang)
{
	int i, j;
	int circ;
	int aimmode[MAXPLAYERS];
	STATUSBARTYPE tsbar[MAXPLAYERS];

	auto newActor = CreateActor(getPlayer(0)->cursector, startpos,
		DukePlayerPawnClass /*fixme for RR later!*/, 0, DVector2(0, 0), startang, 0., 0., nullptr, 10);

	newActor->spr.Angles.Pitch = DAngle::fromDeg(-17.354);
	newActor->backuploc();

	if (ud.recstat != 2) for (i = 0; i < MAXPLAYERS; i++)
	{
		aimmode[i] = getPlayer(i)->aim_mode;
		if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
		{
			for (j = 0; j < MAX_WEAPONS; j++)
			{
				tsbar[i].ammo_amount[j] = getPlayer(i)->ammo_amount[j];
				tsbar[i].gotweapon[j] = getPlayer(i)->gotweapon[j];
			}

			tsbar[i].shield_amount = getPlayer(i)->shield_amount;
			tsbar[i].curr_weapon = getPlayer(i)->curr_weapon;
			tsbar[i].inven_icon = getPlayer(i)->inven_icon;

			tsbar[i].firstaid_amount = getPlayer(i)->firstaid_amount;
			tsbar[i].steroids_amount = getPlayer(i)->steroids_amount;
			tsbar[i].holoduke_amount = getPlayer(i)->holoduke_amount;
			tsbar[i].jetpack_amount = getPlayer(i)->jetpack_amount;
			tsbar[i].heat_amount = getPlayer(i)->heat_amount;
			tsbar[i].scuba_amount = getPlayer(i)->scuba_amount;
			tsbar[i].boot_amount = getPlayer(i)->boot_amount;
		}
	}

	resetplayerstats(0);
	const auto firstp = getPlayer(0);

	for (i = 1; i < MAXPLAYERS; i++)
	{
		const auto thisp = getPlayer(i);
		*thisp = *firstp;
		thisp->pnum = i;
	}

	if (ud.recstat != 2) for (i = 0; i < MAXPLAYERS; i++)
	{
		getPlayer(i)->aim_mode = aimmode[i];
		if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
		{
			for (j = 0; j < MAX_WEAPONS; j++)
			{
				getPlayer(i)->ammo_amount[j] = tsbar[i].ammo_amount[j];
				getPlayer(i)->gotweapon[j] = tsbar[i].gotweapon[j];
			}
			getPlayer(i)->shield_amount = tsbar[i].shield_amount;
			getPlayer(i)->curr_weapon = tsbar[i].curr_weapon;
			getPlayer(i)->inven_icon = tsbar[i].inven_icon;

			getPlayer(i)->firstaid_amount = tsbar[i].firstaid_amount;
			getPlayer(i)->steroids_amount = tsbar[i].steroids_amount;
			getPlayer(i)->holoduke_amount = tsbar[i].holoduke_amount;
			getPlayer(i)->jetpack_amount = tsbar[i].jetpack_amount;
			getPlayer(i)->heat_amount = tsbar[i].heat_amount;
			getPlayer(i)->scuba_amount = tsbar[i].scuba_amount;
			getPlayer(i)->boot_amount = tsbar[i].boot_amount;
		}
	}

	numplayersprites = 0;
	circ = 2048 / ud.multimode;

	which_palookup = 9;
	j = connecthead;
	DukeStatIterator it(STAT_PLAYER);
	while (auto act = it.Next())
	{
		if (numplayersprites == MAXPLAYERS)
			I_Error("Too many player sprites (max 16.)");

		po[numplayersprites].opos = act->spr.pos;
		po[numplayersprites].oa = act->spr.Angles.Yaw;
		po[numplayersprites].os = act->sector();

		numplayersprites++;
		if (j >= 0)
		{
			act->SetOwner(act);
			act->spr.shade = 0;
			if (isRR()) act->spr.scale = DVector2(0.375, 0.265625);
			else act->spr.scale = DVector2(0.65625, 0.5625);
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			act->spr.xoffset = 0;
			act->clipdist = 16;

			if (getPlayer(j)->last_extra == 0)
			{
				getPlayer(j)->last_extra = gs.max_player_health;
				act->spr.extra = gs.max_player_health;
			}
			else act->spr.extra = getPlayer(j)->last_extra;

			act->spr.yint = j;

			if (ud.last_level == -1)
			{
				if (act->spr.pal == 0)
				{
					act->spr.pal = getPlayer(j)->palookup = which_palookup;
					ud.user_pals[j] = which_palookup;
					which_palookup++;
					if (which_palookup == 17) which_palookup = 9;
				}
				else ud.user_pals[j] = getPlayer(j)->palookup = act->spr.pal;
			}
			else
				act->spr.pal = getPlayer(j)->palookup = ud.user_pals[j];

			getPlayer(j)->actor = act;
			getPlayer(j)->Angles.initialize(getPlayer(j)->GetActor(), (currentLevel->levelNumber & 1)? DAngle90 : -DAngle90);
			getPlayer(j)->frag_ps = j;
			act->SetOwner(act);

			getPlayer(j)->setbobpos();

			updatesector(act->spr.pos, &getPlayer(j)->cursector);

			j = connectpoint2[j];

		}
		else act->Destroy();
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
void lava_cleararrays();

void prelevel_common(int g)
{
	Level.clearStats();
	if (isRRRA()) ud.mapflags = MFLAG_ALLSECTORTYPES;
	else if (isRR()) ud.mapflags = MFLAG_SECTORTYPE800;
	auto p = getPlayer(screenpeek);
	p->sea_sick_stat = 0;
	ud.ufospawnsminion = 0;
	ud.pistonsound = 0;
	p->SlotWin = 0;
	enemysizecheat = 0;
	p->MamaEnd = 0;
	banjosound = 0;
	RRRA_ExitedLevel = 0;

	lava_cleararrays();
	geocnt = 0;
	ambienttags.Clear();
	thunderon = 0;
	ud.chickenplant = 0;
	WindTime = 0;
	WindDir = nullAngle;
	fakebubba_spawn = 0;
	RRRA_ExitedLevel = 0;
	mamaspawn_count = currentLevel->rr_mamaspawn;

	// RRRA E2L1 fog handling.
	ud.fogactive = 0;

	resetprestat(0, g);
	numclouds = 0;

	memset(geosectorwarp, -1, sizeof(geosectorwarp));
	memset(geosectorwarp2, -1, sizeof(geosectorwarp2));

	for(auto&sec: sector)
	{
		auto sectp = &sec;
		sectp->extra = 256;

		switch (sectp->lotag)
		{
		case 20:
		case 22:
			if (sectp->floorz > sectp->ceilingz)
				sectp->lotag |= 32768;
			continue;
		}

		if (sectp->ceilingstat & CSTAT_SECTOR_SKY)
		{
			if (tilesurface(sectp->ceilingtexture) == TSURF_SCROLLSKY && numclouds < 127)
				clouds[numclouds++] = sectp;

			if (getPlayer(0)->one_parallax_sectnum == nullptr)
				getPlayer(0)->one_parallax_sectnum = sectp;
		}

		if (sectp->lotag == 32767) //Found a secret room
		{
			Level.addSecretCount();
			continue;
		}

		if (sectp->lotag == -1)
		{
			getPlayer(0)->Exit = sectp->walls[0].pos;
			continue;
		}
	}

	mirrorcnt = 0;
	numanimwalls = 0;
	for (auto& wal : wall)
	{
		if (wal.overtexture == mirrortex && (wal.cstat & CSTAT_WALL_1WAY) != 0)
		{
			auto sectp = wal.nextSector();

			if (mirrorcnt > 63)
				I_Error("Too many mirrors (64 max.)");
			if (sectp && sectp->ceilingtexture != mirrortex)
			{
				sectp->setceilingtexture(mirrortex);
				sectp->setfloortexture(mirrortex);
				mirrorwall[mirrorcnt] = &wal;
				mirrorsector[mirrorcnt] = sectp;
				mirrorcnt++;
				continue;
			}
		}

		if (tileflags(wal.overtexture) & (TFLAG_FORCEFIELD | TFLAG_ANIMFORCEFIELD))
		{
			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = 0;
			animwall[numanimwalls].origtex = wal.overtexture;
			animwall[numanimwalls].overpic = true;
			numanimwalls++;

			if (tileflags(wal.overtexture) & TFLAG_ANIMFORCEFIELD)
			{
				if (wal.shade > 31)
					wal.cstat = 0;
				else wal.cstat |= CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN | CSTAT_WALL_YFLIP;

				if (wal.lotag && wal.twoSided())
					wal.nextWall()->lotag = wal.lotag;
			}
		}
		if (tileflags(wal.walltexture) & (TFLAG_ANIMSCREEN | TFLAG_ANIMSCREENNOISE))
		{
			animwall[numanimwalls].wall = &wal;
			animwall[numanimwalls].tag = -1;
			animwall[numanimwalls].origtex = wal.walltexture;
			animwall[numanimwalls].overpic = false;
			numanimwalls++;
		}

		if (numanimwalls >= MAXANIMWALLS)
			I_Error("Too many 'anim' walls (max 512.)");
	}

	//Invalidate textures in sector behind mirror
	for (int i = 0; i < mirrorcnt; i++)
	{
		for (auto& wal : mirrorsector[i]->walls)
		{
			wal.setwalltexture(mirrortex);
			wal.setovertexture(mirrortex);
		}
	}
	thunder_brightness = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void resettimevars(void)
{
	cloudclock = 0;
	PlayClock = 0;
	if (camsprite != nullptr)
		camsprite->counter = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void donewgame(MapRecord* map, int sk)
{
	auto p = getPlayer(0);
	show_shareware = 26 * 34;

	ud.player_skill = sk;
	ud.secretlevel = 0;
	ud.from_bonus = 0;

	ud.last_level = -1;

	M_ClearMenus();
	ResetGameVars();

	if (m_coop != 1)
	{
		if (isWW2GI())
		{
			for (int i = 0; i < 12/*MAX_WEAPONS*/; i++) // aboive 12 have no data defined and would crash.
			{
				if (aplWeaponWorksLike(i, 0) == PISTOL_WEAPON)
				{
					p->curr_weapon = i;
					p->gotweapon[i] = true;
					p->ammo_amount[i] = 48;
				}
				else if (aplWeaponWorksLike(i, 0) == KNEE_WEAPON || aplWeaponWorksLike(i, 0) == HANDREMOTE_WEAPON)
				{
					p->gotweapon[i] = true;
				}
			}
		}
		else
		{
			p->curr_weapon = PISTOL_WEAPON;
			p->gotweapon[PISTOL_WEAPON] = true;
			p->gotweapon[KNEE_WEAPON] = true;
			p->ammo_amount[PISTOL_WEAPON] = 48;
			p->gotweapon[HANDREMOTE_WEAPON] = true;
			p->last_weapon = -1;
		}

		p->last_weapon = -1;
	}

	display_mirror = 0;

	if (ud.multimode > 1)
	{
		if (numplayers < 2)
		{
			connecthead = 0;
			for (int i = 0; i < MAXPLAYERS; i++) connectpoint2[i] = i + 1;
			connectpoint2[ud.multimode - 1] = -1;
		}
	}
	else
	{
		connecthead = 0;
		connectpoint2[0] = -1;
	}
}

//---------------------------------------------------------------------------
//
// the setup here is very, very sloppy, because mappings are not 1:1.
// Each portal can have multiple sectors, and even extends to unmarked
// neighboring sectors if they got the portal tile as floor or ceiling
//
//---------------------------------------------------------------------------

static void SpawnPortals()
{
	for (auto& wal : wall)
	{
		if (wal.overtexture == mirrortex && (wal.cstat & CSTAT_WALL_1WAY)) wal.portalflags |= PORTAL_WALL_MIRROR;
	}

	portalClear();
	int tag;
	if (!isRR()) tag = 40;
	else if (isRRRA()) tag = 150;
	else return;

	TArray<int> processedTags;
	DukeStatIterator it(STAT_RAROR);
	while (auto act = it.Next())
	{
		if (iseffector(act) && act->spr.lotag == tag)
		{
			int hitag = act->spr.hitag;
			if (processedTags.Find(hitag) == processedTags.Size())
			{
				DukeStatIterator it2(STAT_RAROR);
				while (auto act2 = it2.Next())
				{
					if (iseffector(act2) && act2->spr.lotag == tag + 1 && act2->spr.hitag == hitag)
					{
						if (processedTags.Find(act->spr.hitag) == processedTags.Size())
						{
							sectortype* s1 = act->sector(), *s2 = act2->sector();
							s1->portalflags = PORTAL_SECTOR_FLOOR;
							s2->portalflags = PORTAL_SECTOR_CEILING;
							DVector2 diff = act->spr.pos.XY() - act2->spr.pos.XY();
							s1->portalnum = portalAdd(PORTAL_SECTOR_FLOOR, sectindex(s2), DVector3(-diff, hitag)); // uses delta.Z as temporary storage, not a real coordinate.
							s2->portalnum = portalAdd(PORTAL_SECTOR_CEILING, sectindex(s1), DVector3(diff, hitag));
							processedTags.Push(act->spr.hitag);
						}
						else
						{
							for (auto& p : allPortals)
							{
								if (p.type == PORTAL_SECTOR_FLOOR && p.delta.Z == hitag)
								{
									p.targets.Push(act2->sectno());
								}
							}
						}
					}
				}
			}
			else
			{
				for (auto& p : allPortals)
				{
					if (p.type == PORTAL_SECTOR_CEILING && p.delta.Z == hitag)
					{
						p.targets.Push(act->sectno());
					}
				}
			}
		}
	}
	// Unfortunately the above still isn't enough. We got to do one more check to add stuff to the portals.
	// There is one map where a sector neighboring a portal is not marked as part of the portal itself.
	for (unsigned i = 0; i < sector.Size(); i++)
	{
		auto sectp = &sector[i];
		if (sectp->floortexture == foftex && sectp->portalflags != PORTAL_SECTOR_FLOOR)
		{
			for (auto& pt : allPortals)
			{
				if (pt.type == PORTAL_SECTOR_CEILING)
				{
					for (auto& t : pt.targets)
					{
						if (sectorsConnected(i, t))
						{
							sectp->portalflags = PORTAL_SECTOR_FLOOR;
							sectp->portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
							pt.targets.Push(i);
							goto nexti;
						}
					}
				}
			}
		}
		else if (sectp->ceilingtexture == foftex && sectp->portalflags != PORTAL_SECTOR_CEILING)
		{
			for (auto& pt : allPortals)
			{
				if (pt.type == PORTAL_SECTOR_FLOOR)
				{
					for (auto t : pt.targets)
					{
						if (sectorsConnected(i, t))
						{
							sectp->portalflags = PORTAL_SECTOR_CEILING;
							sectp->portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
							pt.targets.Push(i);
							goto nexti;
						}
					}
				}
			}
		}
	nexti:;
	}
	// clean out the tags we stored in delta.Z
	for (auto& p : allPortals) p.delta.Z = 0;
	mergePortals();
}

//---------------------------------------------------------------------------
//
// this is just a dummy for now to provide the intended setup.
//
//---------------------------------------------------------------------------

static TArray<DDukeActor*> spawnactors(SpawnSpriteDef& sprites)
{
	TArray<DDukeActor*> spawns(sprites.sprites.Size(), true);
	InitSpriteLists();
	int j = 0;
	for (unsigned i = 0; i < sprites.sprites.Size(); i++)
	{
		if (sprites.sprites[i].statnum == MAXSTATUS)
		{
			spawns.Pop();
			continue;
		}

		auto sprt = &sprites.sprites[i];

		auto info = spawnMap.CheckKey(sprt->picnum);
		auto cls = info ? info->cls : nullptr;;
		auto actor = static_cast<DDukeActor*>(InsertActor(cls? cls : RUNTIME_CLASS(DDukeActor), sprt->sectp, sprt->statnum));
		if (actor)
		{
			// for consistency with the original setup we should not eliminate filtered objects here but merely flag them for handling in spawninit.
			if (cls && cls != RUNTIME_CLASS(DDukeActor))
			{
				if (!(info->flags & 0x8000)) actor->flags1 |= SFLAG_SKILLFILTER;
			}

			spawns[j++] = actor;
			actor->initFromSprite(&sprites.sprites[i]);
			setFromSpawnRec(actor, info);
			actor->time = i;
			if (sprites.sprext.Size()) actor->sprext = sprites.sprext[i];
			else actor->sprext = {};
			actor->spsmooth = {};
		}
	}
	leveltimer = sprites.sprites.Size();
	return spawns;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static TMap<uint64_t, bool> checked;
static void markClassForPrecache(PClass* cls, unsigned pal)
{
	uint64_t key = cls->TypeName.GetIndex() + pal * (1ull << 32);
	if (checked.CheckKey(key)) return;
	checked.Insert(key, true);
	auto ainf = static_cast<PClassActor*>(cls)->ActorInfo();
	for (auto clss : ainf->precacheClasses)
	{
		markClassForPrecache(clss, pal);
	}
	for (auto tex : ainf->SpriteSet)
	{
		markTextureForPrecache(tex, pal);
	}

}
void cacheit(void)
{
	checked.Clear();
	for (auto tex : gameinfo.precacheTextures)
	{
		markTextureForPrecache(tex.GetChars());
	}
	for (auto tex : gameinfo.precacheClasses)
	{
		auto cls = PClass::FindActor(tex);
		if (cls) markClassForPrecache(cls, 0);
	}
	DukeSpriteIterator it;
	while (auto act = it.Next())
	{
		if (act->spr.scale.X != 0 && act->spr.scale.Y != 0 && (act->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
		{
			markClassForPrecache(act->GetClass(), act->spr.pal);
		}

		// todo: scan the actions and precache all textures referenced in there as well.
	}
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int LoadTheMap(MapRecord *mi, DDukePlayer*p, int gamemode)
{
	int16_t lbang;
	if (isShareware() && (mi->flags & MI_USERMAP))
	{
		I_Error("Cannot load user maps with shareware version!\n");
	}

	currentLevel = mi;
	sectortype* sect;
	SpawnSpriteDef sprites;
	DVector3 pos;
	loadMap(mi->fileName, isShareware(), &pos, &lbang, &sect, sprites);
	p->cursector = sect;

	SECRET_SetMapName(mi->DisplayName(), mi->name);
	STAT_NewLevel(mi->fileName);
	TITLE_InformName(mi->name);

	auto actorlist = spawnactors(sprites);

	if (isRR()) prelevel_r(gamemode, actorlist);
	else prelevel_d(gamemode, actorlist);

	for (auto& sect : sector)
	{
		if (tilesurface(sect.ceilingtexture) == TSURF_THUNDERSKY)
		{
			addthundersector(&sect);
			thunderon = 1;
		}
	}

	SpawnPortals();

	allignwarpelevators();
	resetpspritevars(gamemode, pos, mapangle(lbang));

	if (r_precache)
	{
		cacheit();

		precacheMap();
		precacheMarkedTiles();
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void clearfrags(void)
{
	for (int i = 0; i < ud.multimode; i++)
	{
		getPlayer(i)->frag = getPlayer(i)->fraggedself = 0;
		memset(getPlayer(i)->frags, 0, sizeof(getPlayer(i)->frags));
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void enterlevel(MapRecord *mi, int gamemode)
{
//    flushpackets();
//    waitforeverybody();

	ud.respawn_monsters = ud.m_respawn_monsters;
	ud.respawn_items = ud.m_respawn_items;
	ud.respawn_inventory = ud.m_respawn_inventory;
	ud.monsters_off = ud.m_monsters_off;
	ud.coop = ud.m_coop;
	ud.ffire = ud.m_ffire;
	lastlevel = 0;

	OnEvent(EVENT_ENTERLEVEL);

	// Stop all sounds
	FX_StopAllSounds();
	S_SetReverb(0);

	auto p = getPlayer(0);

	LoadTheMap(mi, p, gamemode);

	// Try this first so that it can disable the CD player if no tracks are found.
	if (isRR())
		S_PlayRRMusic();

	if (ud.recstat != 2)
	{
		S_PlayLevelMusic(mi);
	}

	for (int i = connecthead; i >= 0; i = connectpoint2[i])
	{
		bool clearweapon = !!(currentLevel->flags & LEVEL_CLEARWEAPONS);
		auto pn = getPlayer(i)->GetActor()->sector()->floortexture;
		if (tileflags(pn) & TFLAG_CLEARINVENTORY)
		{
			resetinventory(getPlayer(i));
			clearweapon = true;
		}
		if (clearweapon)
		{
			resetweapons(getPlayer(i));
			getPlayer(i)->gotweapon[PISTOL_WEAPON] = false;
			getPlayer(i)->ammo_amount[PISTOL_WEAPON] = 0;
			getPlayer(i)->curr_weapon = KNEE_WEAPON;
			getPlayer(i)->kickback_pic = 0;
			getPlayer(i)->okickback_pic = getPlayer(i)->kickback_pic = 0;
		}
		if (currentLevel->flags & LEVEL_CLEARINVENTORY) resetinventory(getPlayer(i));
	}
	resetmys();

	global_random = 0;

	ud.last_level = 1;
	getPlayer(myconnectindex)->over_shoulder_on = 0;
	clearfrags();
	resettimevars();  // Here we go
	setLevelStarted(mi);
	for (auto& wal : wall)
	{
		if (tileflags(wal.walltexture) & TFLAG_SEASICKWALL)
			StartInterpolation(&wal, Interp_Wall_PanX);
	}
}

//---------------------------------------------------------------------------
//
// Start a new game from the menu
//
//---------------------------------------------------------------------------

void GameInterface::NewGame(MapRecord* map, int skill, bool)
{
	for (int i = 0; i != -1; i = connectpoint2[i])
	{
		resetweapons(getPlayer(i));
		resetinventory(getPlayer(i));
	}

	getPlayer(0)->last_extra = gs.max_player_health;


	if (skill == -1) skill = ud.player_skill;
	else skill++;
	ud.player_skill = skill;
	ud.m_respawn_monsters = (skill == 4);
	ud.m_monsters_off = ud.monsters_off = 0;
	ud.m_respawn_items = 0;
	ud.m_respawn_inventory = 0;
	ud.multimode = 1;

	donewgame(map, skill);
	enterlevel(map, 0);
	if (isShareware() && ud.recstat != 2) FTA(QUOTE_F1HELP, getPlayer(myconnectindex));

	PlayerColorChanged();
}

//---------------------------------------------------------------------------
//
// Ideally this will become the only place where map progression gets set up.
//
//---------------------------------------------------------------------------

int setnextmap(bool checksecretexit)
{
	MapRecord* map = nullptr;
	MapRecord* from_bonus = nullptr;

	if (ud.eog && !(currentLevel->flags & LEVEL_FORCENOEOG))
	{
	}
	else if (checksecretexit && ud.from_bonus == 0)
	{
		if (ud.secretlevel > 0)
		{
			// allow overriding the secret exit destination to make episode compilation easier with maps containing secret exits.
			if (currentLevel->flags & LEVEL_SECRETEXITOVERRIDE) map = FindNextSecretMap(currentLevel);
			if (!map) map = FindMapByIndex(currentLevel->cluster, ud.secretlevel);

			if (map)
			{
				from_bonus = FindNextMap(currentLevel);
			}
		}
	}
	else if (ud.from_bonus && currentLevel->NextMap.IsEmpty())	// if the current level has an explicit link, use that instead of ud.from_bonus.
	{
		map = FindMapByLevelNum(ud.from_bonus);
	}
	else
	{
		map = FindNextMap(currentLevel);
	}

	// Make sure these two are cleared in case the map check errors out.
	ud.from_bonus = 0;
	ud.secretlevel = 0;
	if (map)
	{
		// If the map doesn't exist, abort with a meaningful message instead of crashing.
		if (fileSystem.FindFile(map->fileName) < 0)
		{
			I_Error("Trying to open non-existent %s", map->fileName.GetChars());
		}
		ud.from_bonus = from_bonus? from_bonus->levelNumber : 0;
	}
	CompleteLevel(map);
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void exitlevel(MapRecord* nextlevel)
{
	bool endofgame = nextlevel == nullptr;
	STAT_Update(endofgame);
	StopCommentary();

	SummaryInfo info{};

	Level.fillSummary(info);
	info.endofgame = endofgame;
	Mus_Stop();

	if (playerswhenstarted > 1 && ud.coop != 1)
		{
		// MP scoreboard
		ShowScoreboard(playerswhenstarted, [=](bool)
			{
			gameaction = ga_level;
			ud.eog = false;
			if (endofgame)
			{
					auto nextlevel = FindMapByLevelNum(0);
					if (!nextlevel)
					{
						gameaction = ga_startup;
						return;
					}
					else gameaction = ga_nextlevel;
				}
				else
				gameaction = ga_nextlevel;

		});
	}
	else if (ud.multimode <= 1)
	{
		// SP cutscene + summary
		ShowIntermission(currentLevel, nextlevel, &info, [=](bool)
		{
			ud.eog = false;
			gameaction = endofgame? ga_startup : ga_nextlevel;
		});
	}
}


END_DUKE_NS  
