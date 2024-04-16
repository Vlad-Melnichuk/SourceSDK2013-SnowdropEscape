//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "grenade_ar2.h"
#include "ai_memory.h"
#include "soundent.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "convar.h"
#include <player.h>
#include <hl2_player.h>
//#include <GrenadeLaunchersLoaded.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar    sk_plr_dmg_smg1_grenade;	

class CWeaponSMG1 : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponSMG1, CHLSelectFireMachineGun );

	CWeaponSMG1();

	DECLARE_SERVERCLASS();
	
	void	Precache( void );
	void	AddViewKick( void );
	void	LoadGrenade(CHL2_Player *pHL2Player);
	void	PrimaryAttack(void);
	void	HoldIronsight(void);
	void	SecondaryAttack( void );
	void	ItemPostFrame( void );
	void	SetSkin(int skinNum);
	void	GrenadeShellEject(void);

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );
	bool	Deploy(void);

	float	GetFireRate( void ) { return 0.075f; }	// 13.3hz
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		if (m_bIsIronsighted)
		{
			static const Vector cone = VECTOR_CONE_3DEGREES;
			return cone;
		}
		else
		{
			static const Vector cone = VECTOR_CONE_6DEGREES;
			return cone;
		}
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void PickupAnim(void);
	public: int smg1_anim_status = 0;
	DECLARE_ACTTABLE();

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
	float m_flSecondaryReloadActivationTime; //new
	float m_flSecondaryReloadDeactivationTime; //new
	float	m_flSecondaryEjectTime; //new
	bool	m_bSecondaryEjectPending; //new
};

IMPLEMENT_SERVERCLASS_ST(CWeaponSMG1, DT_WeaponSMG1)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg1, CWeaponSMG1 );
PRECACHE_WEAPON_REGISTER(weapon_smg1);

BEGIN_DATADESC( CWeaponSMG1 )

DEFINE_FIELD( m_vecTossVelocity, FIELD_VECTOR ),
DEFINE_FIELD( m_flNextGrenadeCheck, FIELD_TIME ),
DEFINE_FIELD(m_flSecondaryReloadActivationTime, FIELD_TIME),
DEFINE_FIELD(m_flSecondaryReloadDeactivationTime, FIELD_TIME),
DEFINE_FIELD(m_flSecondaryEjectTime, FIELD_TIME), //new
DEFINE_FIELD(m_bSecondaryEjectPending, FIELD_BOOLEAN), //new

END_DATADESC()

acttable_t	CWeaponSMG1::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE(CWeaponSMG1);

//=========================================================
CWeaponSMG1::CWeaponSMG1( )
{
	m_fMinRange1		= 0;// No minimum range. 
	m_fMaxRange1		= 1400;

	m_bAltFiresUnderwater = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::Precache( void )
{
	UTIL_PrecacheOther("grenade_ar2");
	PrecacheModel("models/items/ar3_grenade_shell.mdl");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponSMG1::Equip( CBaseCombatCharacter *pOwner )
{
	if( pOwner->Classify() == CLASS_PLAYER_ALLY )
	{
		m_fMaxRange1 = 3000;
	}
	else
	{
		m_fMaxRange1 = 1400;
	}

	BaseClass::Equip( pOwner );
}

bool CWeaponSMG1::Deploy(void)
{
	m_nShotsFired = 0;
	Msg("SDE_SMG!_deploy\n");
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
		pPlayer->ShowCrosshair(true);
	DisplaySDEHudHint();

	bool return_value = BaseClass::Deploy();

	m_bForbidIronsight = true; // to suppress ironsight during deploy as the weapon is bolted. Behavior of ironsightable weapons that DO bolt on deploy

	return return_value;
}

//added
ConVar sde_pickup_smg1("sde_pickup_smg1", "0");
void CWeaponSMG1::PickupAnim(void)
{
	SendWeaponAnim(ACT_VM_DRYFIRE);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}
void CWeaponSMG1::SetSkin(int skinNum)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if (pViewModel == NULL)
		return;

	pViewModel->m_nSkin = skinNum;
}
void CWeaponSMG1::ItemPostFrame(void)
{

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	if (m_bForbidIronsight && gpGlobals->curtime >= m_flNextPrimaryAttack)
	{
		m_bForbidIronsight = false;
		if (!m_iClip1 && pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			Reload();
	}

	if (gpGlobals->curtime >= m_flSecondaryReloadActivationTime)
	{
		m_bInSecondaryReload = true;
	}

	if (gpGlobals->curtime >= m_flSecondaryReloadDeactivationTime)
	{
		m_bInSecondaryReload = false;
	}

	// Ironsight if not reloading or deploying
	if (!(m_bInReload || m_bInSecondaryReload || m_bForbidIronsight))
		HoldIronsight();

	// Debounce the recoiling counter
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;
	}

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;
	if (pPlayer)
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		// �������������� ������� � ������������
		trace_t tr;
		Vector	vecStart, vecStop, vecDir;

		// �������� ����
		AngleVectors(pPlayer->EyeAngles(), &vecDir);

		// �������� �������
		vecStart = pPlayer->EyePosition();
		vecStop = vecStart + vecDir * MAX_TRACE_LENGTH;

		UTIL_TraceLine(vecStart, vecStop, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			float flDist = (tr.startpos - tr.endpos).Length();
			//DevMsg("DIST: %.0f \n", flDist);

			//skins
			if (flDist < 700)
			{
				SetSkin(0);
			}
			else if ((flDist > 700) & (flDist < 850))
			{
				SetSkin(1);
			}
			else if ((flDist > 850) & (flDist < 1000))
			{
				SetSkin(2);
			}
			else if ((flDist > 850) & (flDist < 1000))
			{
				SetSkin(3);
			}
			else if ((flDist > 1000) & (flDist < 1200))
			{
				SetSkin(4);
			}
			else if ((flDist > 1200) & (flDist < 1400))
			{
				SetSkin(5);
			}
			else if ((flDist > 1400) & (flDist < 1550))
			{
				SetSkin(6);
			}
			else if ((flDist > 1550) & (flDist < 1700))
			{
				SetSkin(7);
			}
			else if ((flDist > 1700) & (flDist < 1900))
			{
				SetSkin(8);
			}
			else if (flDist > 1900)
			{
				SetSkin(9);
			}
		}

		

	}

	if (m_bSecondaryEjectPending && gpGlobals->curtime > m_flSecondaryEjectTime) //new
	{
		GrenadeShellEject();
	}

	BaseClass::ItemPostFrame();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SMG1:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = pOperator->Weapon_ShootPosition();
			}

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
		break;

		/*//FIXME: Re-enable
		case EVENT_WEAPON_AR2_GRENADE:
		{
		CAI_BaseNPC *npc = pOperator->MyNPCPointer();

		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetShootEnemyDir( vecShootOrigin );

		Vector vecThrow = m_vecTossVelocity;

		CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecShootOrigin, vec3_angle, npc );
		pGrenade->SetAbsVelocity( vecThrow );
		pGrenade->SetLocalAngularVelocity( QAngle( 0, 400, 0 ) );
		pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY ); 
		pGrenade->m_hOwner			= npc;
		pGrenade->m_pMyWeaponAR2	= this;
		pGrenade->SetDamage(sk_npc_dmg_ar2_grenade.GetFloat());

		// FIXME: arrgg ,this is hard coded into the weapon???
		m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.

		m_iClip2--;
		}
		break;
		*/

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponSMG1::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponSMG1::Reload(void)
{
	if (m_bInSecondaryReload)
		return false; //prevent interruption of secondary reload with primary reload

	{
		float fCacheTime = m_flNextSecondaryAttack;


		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
		if (pPlayer)
		{
			pPlayer->ShowCrosshair(true); // show crosshair to fix crosshair for reloading weapons in toggle ironsight
			if (m_iClip1 < 1)
			{
				Msg("SDE_R+ \n");
				bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
				if (fRet)
				{
					WeaponSound(RELOAD);
					m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;
				}
				return fRet;
			}
			else
			{
				Msg("SDE_R- \n");
				bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_NOBOLD);
				if (fRet)
				{
					WeaponSound(RELOAD);
					m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;
				}
				return fRet;
			}
		}
		else
		{
			return false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

void CWeaponSMG1::LoadGrenade(CHL2_Player *pHL2Player)
{
	if (!pHL2Player)
		return;

	DisableIronsights();
	SendWeaponAnim(ACT_VM_SECONDARY_RELOAD);
	//m_flNextPrimaryAttack = gpGlobals->curtime + 2.2f;
	//m_flNextSecondaryAttack = gpGlobals->curtime + 2.2f;
	m_flSecondaryReloadActivationTime = gpGlobals->curtime; // signal the secondary reload to ItemPostFrame() immediately to forbid ironsight
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flSecondaryReloadDeactivationTime = gpGlobals->curtime + SequenceDuration();
	pHL2Player->SMG1_GL_Load();
	pHL2Player->ShowCrosshair(true); //for the case of reloading grenade launcher when in toggle ironsight
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::SecondaryAttack( void )
{
	if (m_bInReload)
		return; //prevent interruption of primary reload with secondary attack

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if (pPlayer == NULL)
		return;

	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player*>(pPlayer);

	if (pHL2Player->Get_SMG1_GLL()) // Grenade launcher loading mechanic when the player wants to - HEVcrab
	{

	//Must have ammo
	if ( ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 ) || ( pPlayer->GetWaterLevel() == 3 ) )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	if( m_bInReload )
		m_bInReload = false;

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );

	pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAGS_NONE );

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 2000.0f, vecThrow );
	
	//Create the grenade
	QAngle angles;
	VectorAngles( vecThrow, angles );
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, angles, pPlayer );
	pGrenade->SetModel("models/items/ar3_grenade_noshell.mdl");
	pGrenade->SetAbsVelocity( vecThrow );

	//pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetLocalAngularVelocity(RandomAngle(-5, 5));
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( sk_plr_dmg_smg1_grenade.GetFloat() );

	
	//WeaponSound(RELOAD);
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON );

	// player "shoot" animation
	//pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	//HERE ENABLE ONLY ONE PART OF THE CYCLE WHICH CORRESPONDS TO FALSE BRANCH - HEVcrab
	
	/*if (pPlayer->GetAmmoCount(m_iSecondaryAmmoType) > 1)
	{
		DisableIronsights();
		SendWeaponAnim(ACT_VM_SECONDARYATTACK_RELOAD);
		m_flNextPrimaryAttack = gpGlobals->curtime + 2.2f;
		m_flNextSecondaryAttack = gpGlobals->curtime + 2.2f;
	}*/
	//else
	//{
		SendWeaponAnim(ACT_VM_SECONDARYATTACK);
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
	//}

	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Grenade launcher gets unloaded
	pHL2Player->SMG1_GL_Unload();
	pHL2Player->SMG1_GL_action_failed_reset();
	//engine->ClientCommand(edict(), "testhudanim %s", "AmmoSecondaryDecreasedUnloaded");
	// Can shoot again immediately
	

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );

	}
	
	else if (pPlayer->GetAmmoCount(m_iSecondaryAmmoType) > 0) // If the grenade launcher is not loaded, but player has ammo for it, load it - HEVcrab
	{
		LoadGrenade(pHL2Player);
		m_bSecondaryEjectPending = true; //new
		m_flSecondaryEjectTime = gpGlobals->curtime + 0.5f; //new
		/*DisableIronsights();
		SendWeaponAnim(ACT_VM_SECONDARY_RELOAD);
		//m_flNextPrimaryAttack = gpGlobals->curtime + 2.2f;
		//m_flNextSecondaryAttack = gpGlobals->curtime + 2.2f;
		m_flSecondaryReloadActivationTime = gpGlobals->curtime; // signal the secondary reload to ItemPostFrame() immediately to forbid ironsight
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flSecondaryReloadDeactivationTime = gpGlobals->curtime + SequenceDuration();
		pHL2Player->SMG1_GL_Load();
		pHL2Player->ShowCrosshair(true); //for the case of reloading grenade launcher when in toggle ironsight
		//engine->ClientCommand(edict(), "testhudanim %s", "AmmoSecondaryIncreased");

		//secondary_ammo_recolor_crutch = true; */
	}
	else //attempt to fire with no ammo
	{
		pHL2Player->SMG1_GL_action_failed_set();
		//engine->ClientCommand(edict(), "testhudanim %s", "AmmoSecondaryEmpty");
		//secondary_ammo_recolor_crutch = true;

	}
	
	/*if (secondary_ammo_recolor_crutch)
	{
		pPlayer->SetAmmoCount(pPlayer->GetAmmoCount(m_iSecondaryAmmoType) + 1, m_iSecondaryAmmoType);
		pPlayer->RemoveAmmo(1, m_iSecondaryAmmoType);
		secondary_ammo_recolor_crutch = false;
	}*/
}

#define	COMBINE_MIN_GRENADE_CLEAR_DIST 256

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponSMG1::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	CAI_BaseNPC *npcOwner = GetOwner()->MyNPCPointer();

	return COND_NONE;

/*
	// --------------------------------------------------------
	// Assume things haven't changed too much since last time
	// --------------------------------------------------------
	if (gpGlobals->curtime < m_flNextGrenadeCheck )
		return m_lastGrenadeCondition;
*/

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( npcOwner->IsMoving())
		return COND_NONE;

	CBaseEntity *pEnemy = npcOwner->GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector vecEnemyLKP = npcOwner->GetEnemyLKP();
	if ( !( pEnemy->GetFlags() & FL_ONGROUND ) && pEnemy->GetWaterLevel() == 0 && vecEnemyLKP.z > (GetAbsOrigin().z + WorldAlignMaxs().z) )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}
	
	// --------------------------------------
	//  Get target vector
	// --------------------------------------
	Vector vecTarget;
	if (random->RandomInt(0,1))
	{
		// magically know where they are
		vecTarget = pEnemy->WorldSpaceCenter();
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = vecEnemyLKP;
	}
	// vecTarget = m_vecEnemyLKP + (pEnemy->BodyTarget( GetLocalOrigin() ) - pEnemy->GetLocalOrigin());
	// estimate position
	// vecTarget = vecTarget + pEnemy->m_vecVelocity * 2;


	if ( ( vecTarget - npcOwner->GetLocalOrigin() ).Length2D() <= COMBINE_MIN_GRENADE_CLEAR_DIST )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return (COND_NONE);
	}

	// ---------------------------------------------------------------------
	// Are any friendlies near the intended grenade impact area?
	// ---------------------------------------------------------------------
	CBaseEntity *pTarget = NULL;

	while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST ) ) != NULL )
	{
		//Check to see if the default relationship is hatred, and if so intensify that
		if ( npcOwner->IRelationType( pTarget ) == D_LI )
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
			return (COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: speed is based on difficulty...

	Vector vecToss = VecCheckThrow( this, npcOwner->GetLocalOrigin() + Vector(0,0,60), vecTarget, 600.0, 0.5 );
	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		// JAY: HL1 keeps checking - test?
		//m_flNextGrenadeCheck = gpGlobals->curtime;
		m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.
		return COND_CAN_RANGE_ATTACK2;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_WEAPON_SIGHT_OCCLUDED;
	}
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponSMG1::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}


void CWeaponSMG1::PrimaryAttack(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;


	//dist 
	/*
	if (pPlayer)
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

		// �������������� ������� � ������������
		trace_t tr;
		Vector	vecStart, vecStop, vecDir;

		// �������� ����
		AngleVectors(pPlayer->EyeAngles(), &vecDir);

		// �������� �������
		vecStart = pPlayer->EyePosition();
		vecStop = vecStart + vecDir * MAX_TRACE_LENGTH;

		UTIL_TraceLine(vecStart, vecStop, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			float flDist = (tr.startpos - tr.endpos).Length();
			Msg("DIST: %.0f \n", flDist);
		}
	}
	*/
	//end dist

	if (m_bFireOnEmpty)
	{
		return;
	}
	switch (m_iFireMode)
	{
	case FIREMODE_FULLAUTO:
		BaseClass::PrimaryAttack();
		// Msg("%.3f\n", m_flNextPrimaryAttack.Get() );
		SetWeaponIdleTime(gpGlobals->curtime + 3.0f);
		break;

	case FIREMODE_3RNDBURST:
		m_iBurstSize = GetBurstSize();

		// Call the think function directly so that the first round gets fired immediately.
		BurstThink();
		SetThink(&CHLSelectFireMachineGun::BurstThink);
		m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstCycleRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate();

		// Pick up the rest of the burst through the think function.
		SetNextThink(gpGlobals->curtime + GetFireRate());
		break;
	}

	QAngle	viewPunch;
	if (m_bIsIronsighted)
	{
		SendWeaponAnim(ACT_VM_IRONSHOOT);

		viewPunch.x = random->RandomFloat(0.05f, 0.01f);
		viewPunch.y = random->RandomFloat(-0.01f, 0.01f);
		viewPunch.z = 0.0f;
	}
	else
	{
		SendWeaponAnim(ACT_VM_PRIMARYATTACK);

		viewPunch.x = random->RandomFloat(0.1f, 0.1f);
		viewPunch.y = random->RandomFloat(0.1f, 0.1f);
		viewPunch.z = 0.0f;
	}

	pPlayer->ViewPunch(viewPunch);
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired(pOwner, true, GetClassname());
	}
}

void CWeaponSMG1::GrenadeShellEject(void)
{
	m_bSecondaryEjectPending = false;
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
	{
		Vector SpawnHeight(0, 0, 36); // ������ ������ ������
		QAngle ForwardAngles = pPlayer->EyeAngles(); // + pPlayer->GetPunchAngle() ������������� ����������� ��� ������ ����������, �� � �����?
		Vector vecForward, vecRight, vecUp;
		AngleVectors(ForwardAngles, &vecForward, &vecRight, &vecUp);
		Vector vecEject = SpawnHeight + 5 * vecRight; //- 10 * vecUp;

		CBaseEntity *pEjectProp = (CBaseEntity *)CreateEntityByName("prop_physics_override");

		if (pEjectProp)
		{
			// Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 32 + Vector(0, -8, 16);
			// Vector vecOrigin = pPlayer->GetAbsOrigin() + Vector(-16, -8, 16);
			Vector vecOrigin = pPlayer->GetAbsOrigin() + vecEject;
			QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 0.5, 0);
			pEjectProp->SetAbsOrigin(vecOrigin);
			pEjectProp->SetAbsAngles(vecAngles);
			pEjectProp->KeyValue("model", "models/items/ar3_grenade_shell.mdl");
			pEjectProp->KeyValue("solid", "1");
			pEjectProp->KeyValue("targetname", "EjectProp");
			pEjectProp->KeyValue("spawnflags", "260");
			pEjectProp->SetAbsVelocity(vecForward);
			DispatchSpawn(pEjectProp);
			pEjectProp->Activate();
			pEjectProp->Teleport(&vecOrigin, &vecAngles, NULL);
			pEjectProp->SUB_StartFadeOut(30, false);
		}
	}
}

void CWeaponSMG1::HoldIronsight(void)
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