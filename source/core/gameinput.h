#pragma once

#include "m_fixed.h"
#include "gamecvars.h"
#include "gamestruct.h"
#include "gamefuncs.h"
#include "packet.h"

struct PlayerHorizon
{
	DAngle ZzHORIZON, ZzOLDHORIZON, ZzHORIZOFF, ZzOHORIZOFF;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);

	// Prototypes for functions in gameinput.cpp.
	void applyPitch(float const horz, ESyncBits* actions, double const scaleAdjust = 1);
	void doViewPitch(const DVector2& pos, DAngle const ang, bool const aimmode, bool const canslopetilt, sectortype* const cursectnum, double const scaleAdjust = 1, bool const climbing = false);

	// Interpolation helpers.
	void backupPitch()
	{
		ZzOLDHORIZON = ZzHORIZON;
		ZzOHORIZOFF = ZzHORIZOFF;
	}
	void restorePitch()
	{
		ZzHORIZON = ZzOLDHORIZON;
		ZzHORIZOFF = ZzOHORIZOFF;
	}

	// Commonly used getters.
	DAngle horizOLDSUM() { return ZzOLDHORIZON + ZzOHORIZOFF; }
	DAngle horizSUM() { return ZzHORIZON + ZzHORIZOFF; }
	DAngle horizLERPSUM(double const interpfrac) { return interpolatedvalue(horizOLDSUM(), horizSUM(), interpfrac); }

	// Ticrate playsim adjustment helpers.
	void resetAdjustmentPitch() { legacyAdjustmentPitch = nullAngle; }
	bool targetedPitch() { return legacyTargetPitch.Sgn(); }

	// Input locking helpers.
	void lockPitch() { legacyDisabledPitch = true; }
	void unlockPitch() { legacyDisabledPitch = false; }
	bool lockedPitch() { return targetedPitch() || legacyDisabledPitch; }

	// Ticrate playsim adjustment setters and processor.
	void addPitch(DAngle const value)
	{
		if (!SyncInput())
		{
			legacyAdjustmentPitch += value;
		}
		else
		{
			ZzHORIZON += value;
		}
	}

	void setPitch(DAngle value, bool const backup = false)
	{
		// Clamp incoming variable because sometimes the caller can exceed bounds.
		value = ClampViewPitch(value);

		if (!SyncInput() && !backup)
		{
			legacyTargetPitch = value.Sgn() ? value : minAngle;
		}
		else
		{
			ZzHORIZON = value;
			if (backup) ZzOLDHORIZON = ZzHORIZON;
		}
	}

	void processLegacyHelperPitch(double const scaleAdjust)
	{
		if (targetedPitch())
		{
			auto delta = deltaangle(ZzHORIZON, legacyTargetPitch);

			if (abs(delta).Degrees() > 0.45)
			{
				ZzHORIZON += delta * scaleAdjust;
			}
			else
			{
				ZzHORIZON = legacyTargetPitch;
				legacyTargetPitch = nullAngle;
			}
		}
		else if (legacyAdjustmentPitch.Sgn())
		{
			ZzHORIZON += legacyAdjustmentPitch * scaleAdjust;
		}
	}

private:
	DAngle legacyTargetPitch, legacyAdjustmentPitch;
	bool legacyDisabledPitch;
};

struct PlayerAngle
{
	DAngle ZzANGLE, ZzOLDANGLE, ZzLOOKANG, ZzOLDLOOKANG, ZzROTSCRNANG, orotscrnang, spin;

	friend FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);

	// Prototypes for functions in gameinput.cpp.
	void applyinput(float const avel, ESyncBits* actions, double const scaleAdjust = 1);

	// Interpolation helpers.
	void backup()
	{
		ZzOLDANGLE = ZzANGLE;
		ZzOLDLOOKANG = ZzLOOKANG;
		orotscrnang = ZzROTSCRNANG;
	}
	void restore()
	{
		ZzANGLE = ZzOLDANGLE;
		ZzLOOKANG = ZzOLDLOOKANG;
		ZzROTSCRNANG = orotscrnang;
	}

	// Commonly used getters.
	DAngle osum() { return ZzOLDANGLE + ZzOLDLOOKANG; }
	DAngle sum() { return ZzANGLE + ZzLOOKANG; }
	DAngle interpolatedsum(double const interpfrac) { return interpolatedvalue(osum(), sum(), interpfrac); }
	DAngle interpolatedang(double const interpfrac) { return interpolatedvalue(ZzOLDANGLE, ZzANGLE, interpfrac); }
	DAngle interpolatedlookang(double const interpfrac) { return interpolatedvalue(ZzOLDLOOKANG, ZzLOOKANG, interpfrac); }
	DAngle interpolatedrotscrn(double const interpfrac) { return interpolatedvalue(orotscrnang, ZzROTSCRNANG, interpfrac); }
	DAngle renderlookang(double const interpfrac) { return !SyncInput() ? ZzLOOKANG : interpolatedlookang(interpfrac); }
	DAngle renderrotscrn(double const interpfrac) { return !SyncInput() ? ZzROTSCRNANG : interpolatedrotscrn(interpfrac); }

	// Ticrate playsim adjustment helpers.
	void resetadjustment() { adjustment = nullAngle; }
	bool targetset() { return target.Sgn(); }

	// Input locking helpers.
	void lockinput() { inputdisabled = true; }
	void unlockinput() { inputdisabled = false; }
	bool movementlocked() { return targetset() || inputdisabled; }

	// Draw code helpers. The logic where these are used rely heavily on Build's angle period.
	double look_anghalf(double const interpfrac) { return renderlookang(interpfrac).Normalized180().Degrees() * (128. / 45.); }
	double looking_arc(double const interpfrac) { return fabs(renderlookang(interpfrac).Normalized180().Degrees() * (1024. / 1620.)); }

	// Crosshair x/y offsets based on look_ang's tangent.
	DVector2 crosshairoffsets(const double interpfrac)
	{
		return DVector2(159.72, 145.5 * renderrotscrn(interpfrac).Sin()) * -renderlookang(interpfrac).Tan() * (1. / tan(r_fov * pi::pi() / 360.));
	}

	// Weapon x/y offsets based on the above.
	DVector2 weaponoffsets(const double interpfrac)
	{
		auto offsets = crosshairoffsets(interpfrac); offsets.Y = abs(offsets.Y) * 4.;
		return offsets;
	}

	// Ticrate playsim adjustment setters and processor.
	void addadjustment(const DAngle value)
	{
		if (!SyncInput())
		{
			adjustment += value.Normalized180();
		}
		else
		{
			ZzANGLE += value;
		}
	}

	void settarget(const DAngle value, bool const backup = false)
	{
		if (!SyncInput() && !backup)
		{
			target = value.Sgn() ? value : minAngle;
		}
		else
		{
			ZzANGLE = value;
			if (backup) ZzOLDANGLE = ZzANGLE;
		}
	}

	void processhelpers(double const scaleAdjust)
	{
		if (targetset())
		{
			auto delta = deltaangle(ZzANGLE, target);

			if (abs(delta) > DAngleBuildToDeg)
			{
				ZzANGLE += delta * scaleAdjust;
			}
			else
			{
				ZzANGLE = target;
				target = nullAngle;
			}
		}
		else if (adjustment.Sgn())
		{
			ZzANGLE += adjustment * scaleAdjust;
		}
	}

private:
	DAngle target, adjustment;
	bool inputdisabled;
};

class FSerializer;
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerAngle& w, PlayerAngle* def);
FSerializer& Serialize(FSerializer& arc, const char* keyname, PlayerHorizon& w, PlayerHorizon* def);


void updateTurnHeldAmt(double const scaleAdjust);
bool isTurboTurnTime();
void resetTurnHeldAmt();
void processMovement(InputPacket* const currInput, InputPacket* const inputBuffer, ControlInfo* const hidInput, double const scaleAdjust, int const drink_amt = 0, bool const allowstrafe = true, double const turnscale = 1);
