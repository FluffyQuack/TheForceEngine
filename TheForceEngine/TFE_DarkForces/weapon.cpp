#include "weapon.h"
#include "player.h"
#include "pickup.h"
#include "weaponFireFunc.h"
#include <TFE_System/system.h>
#include <TFE_Jedi/Level/rtexture.h>
#include <TFE_Jedi/InfSystem/message.h>
#include <TFE_Jedi/Renderer/RClassic_Fixed/rlightingFixed.h>
#include <TFE_Jedi/Renderer/virtualFramebuffer.h>
#include <TFE_Jedi/Renderer/screenDraw.h>

namespace TFE_DarkForces
{
	///////////////////////////////////////////
	// Internal State
	///////////////////////////////////////////
	static JBool s_weaponTexturesLoaded = JFALSE;
	static JBool s_switchWeapons = JFALSE;
	static JBool s_queWeaponSwitch = JFALSE;
	
	static TextureData* s_rhand1 = nullptr;
	static TextureData* s_gasmaskTexture = nullptr;
	static PlayerWeapon s_playerWeaponList[WPN_COUNT];
			
	static Tick s_weaponDelayPrimary;
	static Tick s_weaponDelaySeconary;
	static s32* s_canFirePrimPtr;
	static s32* s_canFireSecPtr;

	WeaponAnimState s_weaponAnimState;

	s32 s_prevWeapon;
	s32 s_curWeapon;
	s32 s_nextWeapon;
	s32 s_lastWeapon;

	JBool s_weaponAutoMount2 = JFALSE;
	JBool s_secondaryFire = JFALSE;
	JBool s_weaponOffAnim = JFALSE;
	JBool s_isShooting = JFALSE;
	s32 s_canFireWeaponSec;
	s32 s_canFireWeaponPrim;
	u32 s_fireFrame = 0;
		
	SoundSourceID s_punchSwingSndSrc;
	SoundSourceID s_pistolSndSrc;
	SoundSourceID s_pistolEmptySndSrc;
	SoundSourceID s_rifleSndSrc;
	SoundSourceID s_rifleEmptySndSrc;
	SoundSourceID s_fusion1SndSrc;
	SoundSourceID s_fusion2SndSrc;
	SoundSourceID s_repeaterSndSrc;
	SoundSourceID s_repeater1SndSrc;
	SoundSourceID s_repeaterEmptySndSrc;
	SoundSourceID s_mortarFireSndSrc;
	SoundSourceID s_mortarFireSndSrc2;
	SoundSourceID s_mortarEmptySndSrc;
	SoundSourceID s_mineSndSrc;
	SoundSourceID s_concussion6SndSrc;
	SoundSourceID s_concussion5SndSrc;
	SoundSourceID s_concussion1SndSrc;
	SoundSourceID s_plasma4SndSrc;
	SoundSourceID s_plasmaEmptySndSrc;
	SoundSourceID s_missile1SndSrc;
	SoundSourceID s_weaponChangeSnd;

	SoundEffectID s_repeaterFireSndID = 0;

	///////////////////////////////////////////
	// Shared State
	///////////////////////////////////////////
	PlayerWeapon* s_curPlayerWeapon = nullptr;
	SoundSourceID s_superchargeCountdownSound;
	Task* s_playerWeaponTask = nullptr;

	///////////////////////////////////////////
	// Forward Declarations
	///////////////////////////////////////////
	TextureData* loadWeaponTexture(const char* texName);
	void weapon_loadTextures();
	void weapon_setFireRateInternal(WeaponFireMode mode, Tick delay, s32* canFire);
	void weapon_playerWeaponTaskFunc(MessageType msg);
	void weapon_handleOnAnimation(MessageType msg);
	void weapon_prepareToFire();

	static WeaponFireFunc s_weaponFireFunc[WPN_COUNT] =
	{
		weaponFire_fist,			// WPN_FIST
		weaponFire_pistol,			// WPN_PISTOL
		weaponFire_rifle,			// WPN_RIFLE
		weaponFire_thermalDetonator,// WPN_THERMAL_DET
		weaponFire_repeater,		// WPN_REPEATER
		weaponFire_fusion,			// WPN_FUSION
		weaponFire_mortar,			// WPN_MORTAR
		weaponFire_mine,			// WPN_MINE
		weaponFire_concussion,		// WPN_CONCUSSION
		weaponFire_cannon			// WPN_CANNON
	};

	///////////////////////////////////////////
	// API Implementation
	///////////////////////////////////////////
	void weapon_resetState()
	{
		s_weaponTexturesLoaded = JFALSE;
		s_switchWeapons = JFALSE;
		s_queWeaponSwitch = JFALSE;

		s_rhand1 = nullptr;
		s_gasmaskTexture = nullptr;

		s_weaponAutoMount2 = JFALSE;
		s_secondaryFire = JFALSE;
		s_weaponOffAnim = JFALSE;
		s_isShooting = JFALSE;

		s_fireFrame = 0;
		s_curPlayerWeapon = nullptr;
		s_playerWeaponTask = nullptr;
	}

	void weapon_startup()
	{
		// TODO: Move this into data instead of hard coding it like vanilla Dark Forces.
		s_playerWeaponList[WPN_FIST] =
		{
			4,						// frameCount
			{nullptr},				// frames[]
			0,						// frame

			//Fluffy (DukeVoice): Adjusted coordinates for Duke's mighty foot
			{172, 10, 90, 10},		// xPos[]
			{141, 142, 74, 142},	// yPos[]

			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			nullptr,				// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			0,						// wakeupRange
			0,						// variation
		};
		s_playerWeaponList[WPN_PISTOL] =
		{
			3,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0xa5, 0xa9, 0xa9},		// xPos[]
			{0x8e, 0x88, 0x88},	    // yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoEnergy,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(45),				// wakeupRange
			22,						// variation
		};
		s_playerWeaponList[WPN_RIFLE] =
		{
			2,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x71, 0x70},			// xPos[]
			{0x7f, 0x72},			// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoEnergy,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(50),				// wakeupRange
			0x56,					// variation
		};
		s_playerWeaponList[WPN_THERMAL_DET] =
		{
			4,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0xa1, 0xb3, 0xb4, 0x84},// xPos[]
			{0x83, 0xa5, 0x66, 0x96},// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoDetonator,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			0,						// wakeupRange
			0x2d002d,				// variation
		};
		s_playerWeaponList[WPN_REPEATER] =
		{
			3,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x9c, 0xa3, 0xa3},		// xPos[]
			{0x8a, 0x8c, 0x8c},		// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoPower,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(60),				// wakeupRange
			0x2d002d,				// variation
		};
		s_playerWeaponList[WPN_FUSION] =
		{
			6,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x13, 0x17, 0x17, 0x17, 0x17, 0x17},// xPos[]
			{0x98, 0x9b, 0x9b, 0x9b, 0x9b, 0x9b},// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoPower,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(55),				// wakeupRange
			0x2d,					// variation
		};
		s_playerWeaponList[WPN_MORTAR] =
		{
			4,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x7b, 0x7e, 0x7f, 0x7b},// xPos[]
			{0x77, 0x75, 0x77, 0x74},// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoShell,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(60),				// wakeupRange
			0x2d,					// variation
		};
		s_playerWeaponList[WPN_MINE] =
		{
			3,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x69, 0x69, 0x84},		// xPos[]
			{0x99, 0x99, 0x96},		// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoMine, // ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			0,						// wakeupRange
			0,						// variation
		};
		s_playerWeaponList[WPN_CONCUSSION] =
		{
			3,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{0x82, 0xbf, 0xbc},		// xPos[]
			{0x83, 0x81, 0x84},		// yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoPower,// ammo
			nullptr,				// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(70),				// wakeupRange
			0,						// variation
		};
		s_playerWeaponList[WPN_CANNON] =
		{
			4,						// frameCount
			{nullptr},				// frames[]
			0,						// frame
			{206, 208, 224, 230},	// xPos[]
			{-74, -60, 81, 86},     // yPos[]
			7,						// flags
			0,						// rollOffset
			0,						// pchOffset
			0,						// xWaveOffset
			0,						// yWaveOffset
			0,						// xOffset
			0,						// yOffset
			&s_playerInfo.ammoPlasma,// ammo
			&s_playerInfo.ammoMissile,// secondaryAmmo
			0,						// u8c
			0,						// u90
			FIXED(55),				// wakeupRange
			0x2d002d,				// variation
		};

		// Load Textures.
		weapon_loadTextures();

		// Load Sounds.
		s_punchSwingSndSrc    = sound_Load("swing.voc");
		s_pistolSndSrc        = sound_Load("pistol-1.voc");
		s_pistolEmptySndSrc   = sound_Load("pistout1.voc");
		s_rifleSndSrc         = sound_Load("rifle-1.voc");
		s_rifleEmptySndSrc    = sound_Load("riflout.voc");
		s_fusion1SndSrc       = sound_Load("fusion1.voc");
		s_fusion2SndSrc       = sound_Load("fusion2.voc");
		s_repeaterSndSrc      = sound_Load("repeater.voc");
		s_repeater1SndSrc     = sound_Load("repeat-1.voc");
		s_repeaterEmptySndSrc = sound_Load("rep-emp.voc");
		s_mortarFireSndSrc    = sound_Load("mortar4.voc");
		s_mortarFireSndSrc2   = sound_Load("mortar2.voc");
		s_mortarEmptySndSrc   = sound_Load("mortar9.voc");
		s_mineSndSrc          = sound_Load("claymor1.voc");
		s_concussion6SndSrc   = sound_Load("concuss6.voc");
		s_concussion5SndSrc   = sound_Load("concuss5.voc");
		s_concussion1SndSrc   = sound_Load("concuss1.voc");
		s_plasma4SndSrc       = sound_Load("plasma4.voc");
		s_plasmaEmptySndSrc   = sound_Load("plas-emp.voc");
		s_missile1SndSrc      = sound_Load("missile1.voc");
		s_weaponChangeSnd     = sound_Load("weapon1.voc");
		s_superchargeCountdownSound = sound_Load("quarter.voc");

		s_isShooting       = JFALSE;
		s_secondaryFire    = JFALSE;
		s_superCharge      = JFALSE;
		s_superChargeHud   = JFALSE;
		s_superchargeTask  = nullptr;
		s_weaponAutoMount2 = JFALSE;
		s_prevWeapon = WPN_PISTOL;
		s_curWeapon  = WPN_PISTOL;
		s_lastWeapon = WPN_PISTOL;
		s_curPlayerWeapon = &s_playerWeaponList[WPN_PISTOL];
	}

	void weapon_enableAutomount(JBool enable)
	{
		s_weaponAutoMount2 = enable;
	}

	void player_cycleWeapons(s32 change)
	{
		s32 dir = (change < 0) ? -1 : 1;
		s32 nextWeapon = s_curWeapon + change;
		JBool hasWeapon = JFALSE;
		while (!hasWeapon)
		{
			if (nextWeapon < 0)
			{
				nextWeapon = WPN_COUNT - 1;
			}
			else if (nextWeapon == WPN_COUNT)
			{
				nextWeapon = WPN_FIST;
			}

			switch (nextWeapon)
			{
				case WPN_FIST:
				{
					hasWeapon = JTRUE;
				} break;
				case WPN_PISTOL:
				{
					if (!s_playerInfo.itemPistol) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_RIFLE:
				{
					if (!s_playerInfo.itemRifle) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_THERMAL_DET:
				{
					// TODO(Core Game Loop Release): Check this.
					if (!s_playerInfo.ammoDetonator) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_REPEATER:
				{
					if (!s_playerInfo.itemAutogun) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_FUSION:
				{
					if (!s_playerInfo.itemFusion) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_MORTAR:
				{
					if (!s_playerInfo.itemMortar) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_MINE:
				{
					// TODO(Core Game Loop Release): Check this.
					if (!s_playerInfo.ammoMine) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_CONCUSSION:
				{
					if (!s_playerInfo.itemConcussion) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
				case WPN_CANNON:
				{
					if (!s_playerInfo.itemCannon) { nextWeapon += dir; }
					else { hasWeapon = JTRUE; }
				} break;
			}
		}

		if (nextWeapon != s_playerInfo.curWeapon && s_playerWeaponTask)
		{
			s_msgArg1 = nextWeapon;
			task_runAndReturn(s_playerWeaponTask, MSG_SWITCH_WPN);

			if (s_playerInfo.curWeapon > s_playerInfo.maxWeapon)
			{
				s_playerInfo.maxWeapon = s_playerInfo.curWeapon;
			}
		}
		else
		{
			s_playerInfo.index2 = nextWeapon;
		}
	}

	void weapon_setNext(s32 wpnIndex)
	{
		PlayerWeapon* prevWeapon = s_curPlayerWeapon;
		s_curWeapon = wpnIndex;

		s_prevWeapon = wpnIndex;
		PlayerWeapon* nextWeapon = &s_playerWeaponList[wpnIndex];
		s_playerInfo.curWeapon = wpnIndex;

		if (wpnIndex == WPN_THERMAL_DET && !(*nextWeapon->ammo))
		{
			nextWeapon->frame = 3;
		}
		else if (wpnIndex == WPN_MINE && !(*nextWeapon->ammo))
		{
			nextWeapon->frame = 2;
		}
		else
		{
			nextWeapon->frame = 0;
		}

		s_curPlayerWeapon = nextWeapon;
		weapon_setFireRate();

		PlayerWeapon* weapon = s_curPlayerWeapon;
		weapon->rollOffset  = prevWeapon->rollOffset;
		weapon->pchOffset   = prevWeapon->pchOffset;
		weapon->xWaveOffset = prevWeapon->xWaveOffset;
		weapon->yWaveOffset = prevWeapon->yWaveOffset;
		weapon->xOffset     = prevWeapon->xOffset;
		weapon->yOffset     = prevWeapon->yOffset;
	}

	void weapon_setFireRate()
	{
		s_canFireWeaponPrim = 1;
		s_canFireWeaponSec = 1;

		// Clear both fire modes.
		weapon_setFireRateInternal(WFIRE_PRIMARY,   0, nullptr);
		weapon_setFireRateInternal(WFIRE_SECONDARY, 0, nullptr);

		switch (s_prevWeapon)
		{
			case WPN_PISTOL:
			{
				Tick fireDelay = s_superCharge ? 35 : 71;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);
			} break;
			case WPN_RIFLE:
			{
				Tick fireDelay = s_superCharge ? 10 : 21;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);
			} break;
			case WPN_THERMAL_DET:
			case WPN_MINE:
			{
				weapon_setFireRateInternal(WFIRE_PRIMARY, 1, &s_canFireWeaponPrim);
			} break;
			case WPN_REPEATER:
			{
				Tick fireDelay = s_superCharge ? 14 : 30;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);

				fireDelay = s_superCharge ? 18 : 37;
				weapon_setFireRateInternal(WFIRE_SECONDARY, fireDelay, &s_canFireWeaponSec);
			} break;
			case WPN_FUSION:
			{
				Tick fireDelay = s_superCharge ? 17 : 35;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);

				fireDelay = s_superCharge ? 33 : 68;
				weapon_setFireRateInternal(WFIRE_SECONDARY, fireDelay, &s_canFireWeaponSec);
			} break;
			case WPN_MORTAR:
			{
				Tick fireDelay = s_superCharge ? 50 : 100;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);
			} break;
			case WPN_CONCUSSION:
			{
				Tick fireDelay = s_superCharge ? 57 : 115;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);
			} break;
			case WPN_CANNON:
			{
				Tick fireDelay = s_superCharge ? 17 : 35;
				weapon_setFireRateInternal(WFIRE_PRIMARY, fireDelay, &s_canFireWeaponPrim);

				fireDelay = s_superCharge ? 86 : 87;
				weapon_setFireRateInternal(WFIRE_SECONDARY, fireDelay, &s_canFireWeaponSec);
			} break;
		}
	}

	void weapon_clearFireRate()
	{
		s_weaponDelayPrimary = 0;
		s_weaponDelaySeconary = 0;
	}
	
	void weapon_createPlayerWeaponTask()
	{
		//s_playerWeaponTask = createSubTask("player weapon", weapon_playerWeaponTaskFunc);
		// TODO: In the original code, createTask() was called - but then the firing is a frame behind.
		// Making it a "createTask" instead makes it happen before the main task, which is what we want.
		s_playerWeaponTask = createTask("player weapon", weapon_playerWeaponTaskFunc);

		s_superCharge = JFALSE;
		s_superChargeHud = JFALSE;
		s_queWeaponSwitch = JFALSE;
		s_superchargeTask = nullptr;
	}

	void weapon_fixupAnim()
	{
		PlayerWeapon* weapon = s_curPlayerWeapon;
		s32 ammo = weapon->ammo ? *weapon->ammo : 0;

		if (s_prevWeapon == WPN_THERMAL_DET && !ammo)
		{
			weapon->frame = 3;
		}
		else if (s_prevWeapon == WPN_MINE && !ammo)
		{
			weapon->frame = 2;
		}
	}

	void weapon_computeMatrix(fixed16_16* mtx, angle14_32 pitch, angle14_32 yaw)
	{
		fixed16_16 cosPch, sinPch;
		sinCosFixed(pitch, &sinPch, &cosPch);

		fixed16_16 cosYaw, sinYaw;
		sinCosFixed(yaw, &sinYaw, &cosYaw);

		mtx[0] = cosYaw;
		mtx[1] = 0;
		mtx[2] = sinYaw;
		mtx[3] = mul16(sinYaw, sinPch);
		mtx[4] = cosPch;
		mtx[5] = -mul16(cosYaw, sinPch);
		mtx[6] = -mul16(sinYaw, cosPch);
		mtx[7] = sinPch;
		mtx[8] = mul16(cosPch, cosYaw);
	}
		
	///////////////////////////////////////////
	// Internal Implementation
	///////////////////////////////////////////
	void weapon_setFireRateInternal(WeaponFireMode mode, Tick delay, s32* canFire)
	{
		if (mode == WFIRE_PRIMARY)
		{
			s_weaponDelayPrimary = delay;
			s_canFirePrimPtr = canFire;
		}
		else
		{
			s_weaponDelaySeconary = delay;
			s_canFireSecPtr = canFire;
		}
	}

	void weapon_loadTextures()
	{
		if (!s_weaponTexturesLoaded)
		{
			s_rhand1 = loadWeaponTexture("rhand1.bm");
			s_gasmaskTexture = loadWeaponTexture("gmask.bm");

			s_playerWeaponList[WPN_FIST].frames[0] = s_rhand1;
			s_playerWeaponList[WPN_FIST].frames[1] = loadWeaponTexture("punch1.bm");
			s_playerWeaponList[WPN_FIST].frames[2] = loadWeaponTexture("punch2.bm");
			s_playerWeaponList[WPN_FIST].frames[3] = loadWeaponTexture("punch3.bm");

			s_playerWeaponList[WPN_PISTOL].frames[0] = loadWeaponTexture("pistol1.bm");
			s_playerWeaponList[WPN_PISTOL].frames[1] = loadWeaponTexture("pistol2.bm");
			s_playerWeaponList[WPN_PISTOL].frames[2] = loadWeaponTexture("pistol3.bm");

			s_playerWeaponList[WPN_RIFLE].frames[0] = loadWeaponTexture("rifle1.bm");
			s_playerWeaponList[WPN_RIFLE].frames[1] = loadWeaponTexture("rifle2.bm");

			s_playerWeaponList[WPN_THERMAL_DET].frames[0] = loadWeaponTexture("therm1.bm");
			s_playerWeaponList[WPN_THERMAL_DET].frames[1] = loadWeaponTexture("therm2.bm");
			s_playerWeaponList[WPN_THERMAL_DET].frames[2] = loadWeaponTexture("therm3.bm");
			s_playerWeaponList[WPN_THERMAL_DET].frames[3] = s_rhand1;

			s_playerWeaponList[WPN_REPEATER].frames[0] = loadWeaponTexture("autogun1.bm");
			s_playerWeaponList[WPN_REPEATER].frames[1] = loadWeaponTexture("autogun2.bm");
			s_playerWeaponList[WPN_REPEATER].frames[2] = loadWeaponTexture("autogun3.bm");

			s_playerWeaponList[WPN_FUSION].frames[0] = loadWeaponTexture("fusion1.bm");
			s_playerWeaponList[WPN_FUSION].frames[1] = loadWeaponTexture("fusion2.bm");
			s_playerWeaponList[WPN_FUSION].frames[2] = loadWeaponTexture("fusion3.bm");
			s_playerWeaponList[WPN_FUSION].frames[3] = loadWeaponTexture("fusion4.bm");
			s_playerWeaponList[WPN_FUSION].frames[4] = loadWeaponTexture("fusion5.bm");
			s_playerWeaponList[WPN_FUSION].frames[5] = loadWeaponTexture("fusion6.bm");

			s_playerWeaponList[WPN_MORTAR].frames[0] = loadWeaponTexture("mortar1.bm");
			s_playerWeaponList[WPN_MORTAR].frames[1] = loadWeaponTexture("mortar2.bm");
			s_playerWeaponList[WPN_MORTAR].frames[2] = loadWeaponTexture("mortar3.bm");
			s_playerWeaponList[WPN_MORTAR].frames[3] = loadWeaponTexture("mortar4.bm");

			s_playerWeaponList[WPN_MINE].frames[0] = loadWeaponTexture("clay1.bm");
			s_playerWeaponList[WPN_MINE].frames[1] = loadWeaponTexture("clay2.bm");
			s_playerWeaponList[WPN_MINE].frames[2] = s_rhand1;

			s_playerWeaponList[WPN_CONCUSSION].frames[0] = loadWeaponTexture("concuss1.bm");
			s_playerWeaponList[WPN_CONCUSSION].frames[1] = loadWeaponTexture("concuss2.bm");
			s_playerWeaponList[WPN_CONCUSSION].frames[2] = loadWeaponTexture("concuss3.bm");

			s_playerWeaponList[WPN_CANNON].frames[0] = loadWeaponTexture("assault1.bm");
			s_playerWeaponList[WPN_CANNON].frames[1] = loadWeaponTexture("assault2.bm");
			s_playerWeaponList[WPN_CANNON].frames[2] = loadWeaponTexture("assault3.bm");
			s_playerWeaponList[WPN_CANNON].frames[3] = loadWeaponTexture("assault4.bm");

			s_weaponTexturesLoaded = JTRUE;
		}
	}

	TextureData* loadWeaponTexture(const char* texName)
	{
		FilePath filePath;
		if (TFE_Paths::getFilePath(texName, &filePath))
		{
			return bitmap_load(&filePath, 0);
		}
		else
		{
			TFE_System::logWrite(LOG_ERROR, "Weapon", "Weapon_Startup: %s NOT FOUND.", texName);
		}
		return nullptr;
	}

	void weapon_setShooting(u32 secondaryFire)
	{
		s_isShooting = JTRUE;
		s_secondaryFire = secondaryFire ? JTRUE : JFALSE;

		if (s_weaponOffAnim)
		{
			// TODO(Core Game Loop Release)
		}

		PlayerWeapon* weapon = s_curPlayerWeapon;
		weapon->flags &= ~2;
		s32 wpnIndex = s_prevWeapon;

		if (wpnIndex == 4)
		{
			// TODO(Core Game Loop Release)
		}
		else if (wpnIndex == 9)
		{
			// TODO(Core Game Loop Release)
		}

		weapon_setFireRate();
	}

	void weapon_queueWeaponSwitch(s32 wpnId)
	{
		s_queWeaponSwitch = JTRUE;
		s_msgArg1 = wpnId;
		task_makeActive(s_playerWeaponTask);
	}

	void weapon_handleState(MessageType msg)
	{
		task_begin;
		if (msg == MSG_STOP_FIRING)
		{
			s_isShooting = JFALSE;
			weapon_prepareToFire();
			s_curPlayerWeapon->flags |= 2;
		}
		else if (msg == MSG_START_FIRING)
		{
			s_secondaryFire = s_msgArg1 ? JTRUE : JFALSE;
			s_isShooting = JTRUE;
			if (s_weaponOffAnim)
			{
				task_callTaskFunc(weapon_handleOnAnimation);
			}
			s_curPlayerWeapon->flags &= ~2;
		}
		else if (msg == MSG_SWITCH_WPN)
		{
			s_switchWeapons = JTRUE;
			s_nextWeapon = s_msgArg1;
		}
		task_end;
	}

	void weapon_stopFiring()
	{
		s_isShooting = JFALSE;
		if (s_prevWeapon == WPN_REPEATER)
		{
			if (s_repeaterFireSndID)
			{
				stopSound(s_repeaterFireSndID);
				s_repeaterFireSndID = 0;
			}
			s_curPlayerWeapon->frame = 0;
		}
		else if (s_prevWeapon == WPN_CANNON)
		{
			if (s_secondaryFire)
			{
				s_curPlayerWeapon->frame = 0;
			}
		}
		PlayerWeapon* weapon = s_curPlayerWeapon;
		weapon->flags |= 2;
	}

	void weapon_handleState2(MessageType msg)
	{
		task_begin;
		task_makeActive(s_playerWeaponTask);
		task_yield(TASK_NO_DELAY);

		if (msg == MSG_STOP_FIRING)
		{
			weapon_stopFiring();
		}
		else if (msg == MSG_START_FIRING)
		{
			weapon_setShooting(s_msgArg1);
		}
		else if (msg == MSG_SWITCH_WPN)
		{
			s_switchWeapons = JTRUE;
			s_nextWeapon = s_msgArg1;
		}
		task_end;
	}
		
	// This task function handles animating a weapon on or off screen, based on the inputs.
	void weapon_animateOnOrOffscreen(MessageType msg)
	{
		struct LocalContext
		{
			Tick startTick;
		};
		task_begin_ctx;

		s_curPlayerWeapon->xOffset = s_weaponAnimState.startOffsetX;
		s_curPlayerWeapon->yOffset = s_weaponAnimState.startOffsetY;
		task_makeActive(s_playerWeaponTask);

		task_yield(TASK_NO_DELAY);
		task_callTaskFunc(weapon_handleState);

		s_curPlayerWeapon->frame = s_weaponAnimState.frame;
		while (s_weaponAnimState.frameCount)
		{
			taskCtx->startTick = s_curTick;
			// We may get calls to the weapon task while animating, so handle them as we get them.
			do
			{
				task_yield((msg != MSG_RUN_TASK) ? TASK_NO_DELAY : s_weaponAnimState.ticksPerFrame);

				if (msg == MSG_STOP_FIRING)
				{
					weapon_stopFiring();
				}
				else if (msg == MSG_START_FIRING)
				{
					weapon_setShooting(s_msgArg1/*secondaryFire*/);
				}
				else if (msg == MSG_SWITCH_WPN)
				{
					s_switchWeapons = JTRUE;
					s_nextWeapon = s_msgArg1;
				}
			} while (msg != MSG_RUN_TASK);

			// Calculate the number of elapsed frames.
			Tick elapsed = s_curTick - taskCtx->startTick;
			if (elapsed <= s_weaponAnimState.ticksPerFrame + 1 || s_weaponAnimState.frameCount < 2)
			{
				s_curPlayerWeapon->xOffset += s_weaponAnimState.xSpeed;
				s_curPlayerWeapon->yOffset += s_weaponAnimState.ySpeed;
				s_weaponAnimState.frameCount--;
			}
			else
			{
				s32 elapsedFrames = min(s_weaponAnimState.frameCount, elapsed / s_weaponAnimState.ticksPerFrame);
				s_curPlayerWeapon->xOffset += s_weaponAnimState.xSpeed * elapsedFrames;
				s_curPlayerWeapon->yOffset += s_weaponAnimState.ySpeed * elapsedFrames;
				s_weaponAnimState.frameCount -= elapsedFrames;
			}
		}

		task_end;
	}

	void weapon_handleOffAnimation(MessageType msg)
	{
		task_begin;
		// s_prevWeapon is the weapon we are switching away from.
		if (s_prevWeapon == WPN_REPEATER)
		{
			if (s_repeaterFireSndID)
			{
				stopSound(s_repeaterFireSndID);
				s_repeaterFireSndID = 0;
			}
			s_curPlayerWeapon->frame = 0;
		}
		else if (s_prevWeapon == WPN_CANNON && !s_secondaryFire)
		{
			s_curPlayerWeapon->frame = 0;
		}

		s_weaponAnimState =
		{
			s_curPlayerWeapon->frame,	// frame
			s_curPlayerWeapon->xOffset, s_curPlayerWeapon->yOffset,
			3, 4,						// xSpeed, ySpeed
			10, s_superCharge ? 1u : 2u	// frameCount, ticksPerFrame
		};
		task_callTaskFunc(weapon_animateOnOrOffscreen);
		s_weaponOffAnim = JTRUE;

		task_end;
	}

	void weapon_handleOnAnimation(MessageType msg)
	{
		task_begin;
		s_weaponOffAnim = JFALSE;
		if (s_curWeapon != WPN_FIST && s_curWeapon != WPN_THERMAL_DET && s_curWeapon != WPN_MINE)
		{
			playSound2D(s_weaponChangeSnd);
		}
				
		if (s_prevWeapon == WPN_THERMAL_DET)
		{
			// TODO(Core Game Loop Release)
		}
		else if (s_prevWeapon == WPN_MINE)
		{
			// TODO(Core Game Loop Release)
		}

		// s_prevWeapon is the weapon we are switching away from.
		s_weaponAnimState =
		{
			0,							// frame
			30, 40,						// startOffsetX, startOffsetY: was 14, 40
			-3, -4,						// xSpeed, ySpeed
			10, s_superCharge ? 1u : 2u	// frameCount, ticksPerFrame
		};
		task_callTaskFunc(weapon_animateOnOrOffscreen);
		task_end;
	}

	void weapon_holster()
	{
		if (s_playerWeaponTask)
		{
			task_runAndReturn(s_playerWeaponTask, MSG_HOLSTER);
		}
	}

	void weapon_prepareToFire()
	{
		PlayerWeapon* weapon = s_curPlayerWeapon;
		if (s_prevWeapon == WPN_REPEATER)
		{
			if (s_repeaterFireSndID)
			{
				stopSound(s_repeaterFireSndID);
				s_repeaterFireSndID = 0;
			}
			weapon->frame = 0;
		}
		else if (s_prevWeapon == WPN_CANNON)
		{
			if (!s_secondaryFire)
			{
				weapon->frame = 0;
			}
		}
	}

	void weapon_playerWeaponTaskFunc(MessageType msg)
	{
		struct LocalContext
		{
			JBool secondaryFire;
		};
		task_begin_ctx;
		while (1)
		{
			// If the weapon task is called with a non-zero id, handle it here.
			if (msg == MSG_FREE_TASK)
			{
				task_free(s_playerWeaponTask);
				s_playerWeaponTask = nullptr;
				return;
			}
			else if (msg == MSG_SWITCH_WPN || s_queWeaponSwitch)
			{
				s_queWeaponSwitch = JFALSE;

				task_makeActive(s_playerWeaponTask);
				s_nextWeapon = s_msgArg1;
				task_makeActive(s_playerWeaponTask);
				task_yield(TASK_NO_DELAY);
				task_callTaskFunc(weapon_handleState);

				if (s_nextWeapon == -1)
				{
					swap(s_curWeapon, s_lastWeapon);
				}
				else
				{
					s32 lastWeapon = s_curWeapon;
					s_curWeapon = s_nextWeapon;
					s_lastWeapon = lastWeapon;
				}
				s_playerInfo.curWeapon = s_prevWeapon;

				if (!s_weaponOffAnim)
				{
					task_callTaskFunc(weapon_handleOffAnimation);
				}
				weapon_setNext(s_curWeapon);
				task_callTaskFunc(weapon_handleOnAnimation);
			}
			else if (msg == MSG_START_FIRING)
			{
				taskCtx->secondaryFire = s_msgArg1;
				task_makeActive(s_playerWeaponTask);
				task_makeActive(s_playerWeaponTask);
				task_yield(TASK_NO_DELAY);

				task_callTaskFunc(weapon_handleState);
				s_isShooting = JTRUE;
				s_secondaryFire = taskCtx->secondaryFire ? JTRUE : JFALSE;
				if (s_weaponOffAnim)
				{
					task_callTaskFunc(weapon_handleOnAnimation);
				}
				s_curPlayerWeapon->flags &= ~2;

				weapon_prepareToFire();
				weapon_setFireRate();
			}
			else if (msg == MSG_STOP_FIRING)
			{
				s_isShooting = JFALSE;
				weapon_prepareToFire();
				s_curPlayerWeapon->flags |= 2;
			}
			else if (msg == MSG_HOLSTER)
			{
				task_makeActive(s_playerWeaponTask);
				task_yield(TASK_NO_DELAY);
				task_callTaskFunc(weapon_handleState);

				if (!s_weaponOffAnim)
				{
					weapon_prepareToFire();
					
					s_weaponAnimState =
					{
						s_curPlayerWeapon->frame,   // frame
						 0,  0,                     // startOffsetX, startOffsetY
						 3,  4,                     // xSpeed, ySpeed
						10, s_superCharge ? 1u : 2u // frameCount, ticksPerFrame
					};
					task_callTaskFunc(weapon_animateOnOrOffscreen);
					s_weaponOffAnim = JTRUE;
				}
				else
				{
					s_weaponOffAnim = JFALSE;
					if (s_curWeapon == WPN_PISTOL || s_curWeapon == WPN_RIFLE || s_curWeapon == WPN_REPEATER || s_curWeapon == WPN_FUSION || s_curWeapon == WPN_MORTAR ||
						s_curWeapon == WPN_CONCUSSION || s_curWeapon == WPN_CANNON)
					{
						playSound2D(s_weaponChangeSnd);
					}

					weapon_fixupAnim();

					s_weaponAnimState =
					{
						s_curPlayerWeapon->frame,   // frame
						30, 40,                     // startOffsetX, startOffsetY
						-3, -4,                     // xSpeed, ySpeed
						10, s_superCharge ? 1u : 2u // frameCount, ticksPerFrame
					};
					task_callTaskFunc(weapon_animateOnOrOffscreen);
				}
			}

			// Handle shooting.
			while (s_isShooting)
			{
				s_switchWeapons = JFALSE;
				s_queWeaponSwitch = JFALSE;
				s_fireFrame++;

				while (msg != MSG_RUN_TASK)
				{
					task_makeActive(s_playerWeaponTask);
					task_yield(TASK_NO_DELAY);
					task_callTaskFunc(weapon_handleState);
				}

				if ((s_secondaryFire && s_canFireWeaponSec) || s_canFireWeaponPrim)
				{
					// Fire the weapon.
					task_callTaskFunc(s_weaponFireFunc[s_prevWeapon]);
				}
				else
				{
					task_makeActive(s_playerWeaponTask);
					task_yield(TASK_NO_DELAY);
					task_callTaskFunc(weapon_handleState);
				}

				if (s_switchWeapons)
				{
					if (s_nextWeapon == -1)
					{
						swap(s_curWeapon, s_lastWeapon);
					}
					else
					{
						s_lastWeapon = s_curWeapon;
						s_curWeapon = s_nextWeapon;
					}
					s_playerInfo.curWeapon = s_prevWeapon;

					// Move the next weapon off screen, if it is not already off.
					// Then move the next one on screen.
					if (!s_weaponOffAnim)
					{
						task_callTaskFunc(weapon_handleOffAnimation);
					}
					weapon_setNext(s_curWeapon);
					task_callTaskFunc(weapon_handleOnAnimation);
				}
			}

			// Go to sleep until needed.
			task_yield(TASK_SLEEP);
		}
		task_end;
	}

	// In DOS, this was part of drawWorld() - 
	// for TFE I split it out to limit the amount of game code in the renderer.
	void weapon_draw(u8* display, DrawRect* rect)
	{
		const fixed16_16 weaponLightingZDist  = FIXED(6);
		const fixed16_16 gasmaskLightingZDist = FIXED(2);

		PlayerWeapon* weapon = s_curPlayerWeapon;
		if (weapon && !s_weaponOffAnim)
		{
			s32 x = weapon->xPos[weapon->frame];
			s32 y = weapon->yPos[weapon->frame];
			if (weapon->flags & 1)
			{
				x += weapon->rollOffset;
				y += weapon->pchOffset;
			}
			if (weapon->flags & 2)
			{
				x += weapon->xWaveOffset;
				y += weapon->yWaveOffset;
			}
			if (weapon->flags & 4)
			{
				x += weapon->xOffset;
				y += weapon->yOffset;
			}

		#if 0  // Low Detail
			if (s_screenWidthFract < ONE_16)
			{
				y += 8;
			}
		#endif
						
			const u8* atten = RClassic_Fixed::computeLighting(weaponLightingZDist, 0);
			TextureData* tex = weapon->frames[weapon->frame];
			if (weapon->ammo && *weapon->ammo == 0 && (weapon->ammo == &s_playerInfo.ammoDetonator || weapon->ammo == &s_playerInfo.ammoMine))
			{
				tex = s_playerWeaponList[WPN_FIST].frames[0];
			}
			
			u32 dispWidth, dispHeight;
			vfb_getResolution(&dispWidth, &dispHeight);

			if (dispWidth == 320 && dispHeight == 200)
			{
				if (atten && !s_weaponLight)
				{
					blitTextureToScreenLit(tex, rect, x, y, atten, display, JTRUE);
				}
				else
				{
					blitTextureToScreen(tex, rect, x, y, display, JTRUE);
				}
			}
			else
			{
				// HUD scaling.
				fixed16_16 xScale = vfb_getXScale();
				fixed16_16 yScale = vfb_getYScale();
				x = floor16(mul16(xScale, intToFixed16(x))) + vfb_getWidescreenOffset();
				y = floor16(mul16(yScale, intToFixed16(y)));

				// Hack to handle the Dark Trooper rifle.
				if (weapon->frames[0]->width == 169)
				{
					x += vfb_getWidescreenOffset();
				}

				if (atten && !s_weaponLight)
				{
					blitTextureToScreenLitScaled(tex, rect, x, y, xScale, yScale, atten, display, JTRUE);
				}
				else
				{
					blitTextureToScreenScaled(tex, rect, x, y, xScale, yScale, display, JTRUE);
				}
			}
		}

		if (s_wearingGasmask)
		{
			s32 x = 105;
			s32 y = 141;
			if (weapon)
			{
				x -= (weapon->xWaveOffset >> 3);
				y += (weapon->yWaveOffset >> 2);
			}
			const u8* atten = RClassic_Fixed::computeLighting(gasmaskLightingZDist, 0);
			TextureData* tex = s_gasmaskTexture;

			u32 dispWidth, dispHeight;
			vfb_getResolution(&dispWidth, &dispHeight);

			if (dispWidth == 320 && dispHeight == 200)
			{
				if (atten)
				{
					blitTextureToScreenLit(tex, rect, x, y, atten, display);
				}
				else
				{
					blitTextureToScreen(tex, rect, x, y, display);
				}
			}
			else
			{
				// HUD scaling.
				fixed16_16 xScale = vfb_getXScale();
				fixed16_16 yScale = vfb_getYScale();
				x = floor16(mul16(xScale, intToFixed16(x))) + vfb_getWidescreenOffset();
				y = floor16(yScale + mul16(yScale, intToFixed16(y)));

				if (atten && !s_weaponLight)
				{
					blitTextureToScreenLitScaled(tex, rect, x, y, xScale, yScale, atten, display);
				}
				else
				{
					blitTextureToScreenScaled(tex, rect, x, y, xScale, yScale, display);
				}
			}
		}
	}
}  // namespace TFE_DarkForces