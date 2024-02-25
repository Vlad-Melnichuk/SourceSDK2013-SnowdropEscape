//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Semi-automatic sniper rifle
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponAnnabelle
//-----------------------------------------------------------------------------

class CWeaponAnnabelle : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponAnnabelle, CBaseHLCombatWeapon);
public:

	CWeaponAnnabelle(void);

	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	virtual bool	Reload(void);
	void	HoldIronsight(void);
	float	WeaponAutoAimScale()	{ return 0.6f; }
	virtual void	ItemPostFrame(void);
	bool Deploy(void);

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(weapon_annabelle, CWeaponAnnabelle);

PRECACHE_WEAPON_REGISTER(weapon_annabelle);

IMPLEMENT_SERVERCLASS_ST(CWeaponAnnabelle, DT_WeaponAnnabelle)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponAnnabelle)
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAnnabelle::CWeaponAnnabelle(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

bool CWeaponAnnabelle::Deploy(void)
{
	Msg("SDE_SMG!_deploy\n");
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
		pPlayer->ShowCrosshair(true);
	DisplaySDEHudHint();
	return BaseClass::Deploy();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponAnnabelle::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (m_iClip1 <= 0)
	{
		if (!m_bFireOnEmpty)
		{
			Reload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
		}

		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.4f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	pPlayer->FireBullets(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);

	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt(-1, 1);
	angles.y += random->RandomInt(-1, 1);
	angles.z = 0;

	pPlayer->SnapEyeAngles(angles);

	pPlayer->ViewPunch(QAngle(-8, random->RandomFloat(-1, 1), 0));
	if (m_iClip1 >= 1)
	{
		//CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());
		WeaponSound(SINGLE);
	}
	else
	{
		WeaponSound(EMPTY_SHOT);
	}
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}

void CWeaponAnnabelle::HoldIronsight(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer->m_afButtonPressed & IN_IRONSIGHT)
	{
		EnableIronsights();
		pPlayer->ShowCrosshair(false);
	}
	if (pPlayer->m_afButtonReleased & IN_IRONSIGHT)
	{
		DisableIronsights();
		pPlayer->ShowCrosshair(true);
	}
}

bool CWeaponAnnabelle::Reload(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	pPlayer->ShowCrosshair(true); // show crosshair to fix crosshair for reloading weapons in toggle ironsight
	if (pPlayer)
	{
		if (m_iClip1 < 1)
		{
			bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
			if (fRet)
			{
				WeaponSound(RELOAD);
			}
			return fRet;
		}
		else
		{
			bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_NOBOLD);
			if (fRet)
			{
				WeaponSound(RELOAD);
			}
			return fRet;
		}

	}
	else
	{
		return false;
	}
}

void CWeaponAnnabelle::SecondaryAttack(void)
{
	//// Only the player fires this way so we can cast
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}

	ToggleIronsights();
	pOwner->ToggleCrosshair();
}

void CWeaponAnnabelle::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}

	if (!m_bInReload && m_iClip1 > 0)
	{
		// Allow  Ironsight
		HoldIronsight();
		if ((pOwner->m_afButtonPressed & IN_ATTACK) && gpGlobals->curtime >= m_flNextPrimaryAttack)
		{
			PrimaryAttack();
		}

		if ((pOwner->m_afButtonPressed & IN_ATTACK2) && gpGlobals->curtime >= m_flNextSecondaryAttack)
			// toggle zoom on sniper rifle like vanilla HL2 crossbow
		{
			SecondaryAttack();
		}

		if ((pOwner->m_afButtonPressed & IN_RELOAD) && gpGlobals->curtime >= m_flNextPrimaryAttack)
		{
			Reload();
		}

	}
	else
		BaseClass::ItemPostFrame(); //reload
}