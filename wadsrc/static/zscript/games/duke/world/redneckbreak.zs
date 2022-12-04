// Container for handler functions that handle walls with breakable textures.
// Note that this must be a class, not a struct, so that the internal lookup functions can find it.

class RedneckBreakWalls
{

	static void breakwall(TextureID newpn, DukeActor spr, walltype wal)
	{
		wal.SetTexture(walltype.main, newpn);
		spr.PlayActorSound("VENT_BUST");
		spr.PlayActorSound("GLASS_HEAVYBREAK");
		spr.lotsofglass(10, wal);
	}

	static void SinglePlayerBreak(walltype wal, TextureID newtex, Sound snd, DukeActor hitter)
	{
		if (ud.multimode < 2)
		{
			wal.SetTexture(walltype.main, newtex);
			hitter.PlayActorSound(snd);
		}
	}

}