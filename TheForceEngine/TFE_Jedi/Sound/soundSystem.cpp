#include <cstring>

#include "soundSystem.h"
#include <TFE_Asset/vocAsset.h>
#include <TFE_Audio/audioSystem.h>
#include <TFE_Jedi/IMuse/imuse.h>

using namespace TFE_Audio;

namespace TFE_Jedi
{
	enum SoundConstants
	{
		JSND_SLOT_MASK = 255,
		JSND_UID_SHIFT = 8,
		JSND_UID_MASK  = (1 << 24) - 1
	};
	#define BUILD_EFFECT_ID(slot) SoundEffectID(slot | (s_slotID[slot] << JSND_UID_SHIFT))

	static const SoundSource* s_slotMapping[MAX_SOUND_SOURCES];
	static s32 s_slotID[MAX_SOUND_SOURCES] = { 0 };

	//Fluffy (DukeVoice)
	static int dukeVoiceSlot = -1;
	SoundEffectID curScriptedSoundID = -1;

	void sound_open(MemoryRegion* memRegion)
	{
		ImInitialize(memRegion);
	}

	void sound_close()
	{
		ImTerminate();
	}

	void sound_stopAll()
	{
		stopAllSounds();
		memset(s_slotID, 0, sizeof(s32)*MAX_SOUND_SOURCES);
		memset(s_slotMapping, 0, sizeof(SoundSource*)*MAX_SOUND_SOURCES);

		//Fluffy (DukeVoice)
		dukeVoiceSlot = -1;
	}

	void sound_freeAll()
	{
		sound_stopAll();
		TFE_VocAsset::freeAll();
	}

	//Fluffy (DukeVoice)
	void StopDukeVoice()
	{
		//Check if Duke is currently talking
		if(dukeVoiceSlot != -1 && s_slotID[dukeVoiceSlot] != 0)
		{
			SoundSource *source = (SoundSource *) s_slotMapping[dukeVoiceSlot];
			if(source && isSourcePlaying(source))
			{
				freeSource(source);
				s_slotMapping[dukeVoiceSlot] = nullptr;
				s_slotID[dukeVoiceSlot] = 0;
				dukeVoiceSlot = -1;
			}
		}
	}

	SoundEffectID playSoundDukeVoiceClip(SoundSourceID sourceId, bool overrideCurrentVoice, bool delay)
	{
		//Check if a scripted voice line is playing, if so, that will always override any dynamic Duke line
		if(curScriptedSoundID != -1)
		{
			SoundSource *source = (SoundSource *) s_slotMapping[curScriptedSoundID & JSND_SLOT_MASK];
			if(source && isSourcePlaying(source))
				return NULL_SOUND;
		}

		//Check if Duke is currently talking
		if(dukeVoiceSlot != -1 && s_slotID[dukeVoiceSlot] != 0)
		{
			SoundSource *source = (SoundSource *) s_slotMapping[dukeVoiceSlot];
			if(source && isSourcePlaying(source))
			{
				if(overrideCurrentVoice)
				{
					freeSource(source);
					s_slotMapping[dukeVoiceSlot] = nullptr;
					s_slotID[dukeVoiceSlot] = 0;
					dukeVoiceSlot = -1;
				}
				else
					return NULL_SOUND;
			}
		}
		
		if (sourceId == NULL_SOUND) { return NULL_SOUND; }
		SoundBuffer* buffer = TFE_VocAsset::getFromIndex(sourceId - 1);
		if (!buffer) { return NULL_SOUND; }

		//Added playback delay
		fixed16_16 playbackDelay;
		if(delay)
			playbackDelay = HALF_16;
		else
			playbackDelay = 0;

		SoundSource* source = createSoundSource(SOUND_2D, 8.0f, 0.5f, buffer, nullptr, false, nullptr, nullptr, &playbackDelay);

		if (!source) { return NULL_SOUND; }

		s32 slot = getSourceSlot(source);
		if (slot < 0)
		{
			freeSource(source);
			return NULL_SOUND;
		}
		playSource(source);

		dukeVoiceSlot = slot;

		s_slotMapping[slot] = source;
		s_slotID[slot] = (s_slotID[slot] + 1) & JSND_UID_MASK;
		if (s_slotID[slot] == 0) { s_slotID[slot]++; }

		if((curScriptedSoundID & JSND_SLOT_MASK) == slot)
			curScriptedSoundID = -1;

		return BUILD_EFFECT_ID(slot);
	}
	
	SoundEffectID playSound2D(SoundSourceID sourceId)
	{
		if (sourceId == NULL_SOUND) { return NULL_SOUND; }
		SoundBuffer* buffer = TFE_VocAsset::getFromIndex(sourceId - 1);
		if (!buffer) { return NULL_SOUND; }

		SoundSource* source = createSoundSource(SOUND_2D, 1.0f, 0.5f, buffer);
		if (!source) { return NULL_SOUND; }
		
		s32 slot = getSourceSlot(source);
		if (slot < 0)
		{
			freeSource(source);
			return NULL_SOUND;
		}
		playSource(source);

		//Fluffy (DukeVoice)
		if(slot == dukeVoiceSlot)
			dukeVoiceSlot = -1;
		if((curScriptedSoundID & JSND_SLOT_MASK) == slot)
			curScriptedSoundID = -1;

		s_slotMapping[slot] = source;
		s_slotID[slot] = (s_slotID[slot] + 1) & JSND_UID_MASK;
		if (s_slotID[slot] == 0) { s_slotID[slot]++; }
		
		return BUILD_EFFECT_ID(slot);
	}

	SoundEffectID playSound2D_looping(SoundSourceID sourceId)
	{
		if (sourceId == NULL_SOUND) { return NULL_SOUND; }
		SoundBuffer* buffer = TFE_VocAsset::getFromIndex(sourceId - 1);
		if (!buffer) { return NULL_SOUND; }

		SoundSource* source = createSoundSource(SOUND_2D, 1.0f, 0.5f, buffer);
		if (!source) { return NULL_SOUND; }

		s32 slot = getSourceSlot(source);
		if (slot < 0)
		{
			freeSource(source);
			return NULL_SOUND;
		}
		playSource(source, true);

		//Fluffy (DukeVoice)
		if(slot == dukeVoiceSlot)
			dukeVoiceSlot = -1;
		if((curScriptedSoundID & JSND_SLOT_MASK) == slot)
			curScriptedSoundID = -1;

		s_slotMapping[slot] = source;
		s_slotID[slot] = (s_slotID[slot] + 1) & JSND_UID_MASK;
		if (s_slotID[slot] == 0) { s_slotID[slot]++; }

		return BUILD_EFFECT_ID(slot);
	}

	SoundEffectID playSound3D_oneshot(SoundSourceID sourceId, vec3_fixed pos)
	{
		if (sourceId == NULL_SOUND) { return NULL_SOUND; }

		SoundBuffer* buffer = TFE_VocAsset::getFromIndex(sourceId - 1);
		if (!buffer) { return NULL_SOUND; }

		Vec3f posFloat = { fixed16ToFloat(pos.x), fixed16ToFloat(pos.y), fixed16ToFloat(pos.z) };
		SoundSource* source = createSoundSource(SOUND_3D, 1.0f, 0.5f, buffer, &posFloat, true);
		if (!source) { return NULL_SOUND; }
		
		s32 slot = getSourceSlot(source);
		if (slot < 0)
		{
			freeSource(source);
			return NULL_SOUND;
		}
		playSource(source);

		//Fluffy (DukeVoice)
		if(slot == dukeVoiceSlot)
			dukeVoiceSlot = -1;
		if((curScriptedSoundID & JSND_SLOT_MASK) == slot)
			curScriptedSoundID = -1;

		s_slotMapping[slot] = source;
		s_slotID[slot] = (s_slotID[slot] + 1) & JSND_UID_MASK;
		if (s_slotID[slot] == 0) { s_slotID[slot]++; }

		return BUILD_EFFECT_ID(slot);
	}

	SoundEffectID playSound3D_looping(SoundSourceID sourceId, SoundEffectID soundId, vec3_fixed pos)
	{
		if (sourceId == NULL_SOUND) { return NULL_SOUND; }

		Vec3f posFloat = { fixed16ToFloat(pos.x), fixed16ToFloat(pos.y), fixed16ToFloat(pos.z) };
		u32 slot = soundId & JSND_SLOT_MASK;
		u32 uid  = soundId >> JSND_UID_SHIFT;

		SoundSource* source = getSourceFromSlot(slot);
		if (soundId && source && source == s_slotMapping[slot] && uid == s_slotID[slot])
		{
			setSourcePosition(source, &posFloat);
			return soundId;
		}

		SoundBuffer* buffer = TFE_VocAsset::getFromIndex(sourceId - 1);
		if (!buffer) { return NULL_SOUND; }

		source = createSoundSource(SOUND_3D, 1.0f, 0.5f, buffer, &posFloat, true);
		slot = getSourceSlot(source);
		if (slot >= MAX_SOUND_SOURCES)
		{
			freeSource(source);
			return NULL_SOUND;
		}
		playSource(source, true);

		//Fluffy (DukeVoice)
		if(slot == dukeVoiceSlot)
			dukeVoiceSlot = -1;
		if((curScriptedSoundID & JSND_SLOT_MASK) == slot)
			curScriptedSoundID = -1;

		s_slotMapping[slot] = source;
		s_slotID[slot] = (s_slotID[slot] + 1) & JSND_UID_MASK;
		if (s_slotID[slot] == 0) { s_slotID[slot]++; }

		return BUILD_EFFECT_ID(slot);
	}

	void stopSound(SoundEffectID sourceId)
	{
		if (sourceId == NULL_SOUND) { return; }

		u32 slot = sourceId & JSND_SLOT_MASK;
		u32 uid = sourceId >> JSND_UID_SHIFT;
		SoundSource* curSoundSource = getSourceFromSlot(slot);
		
		if (curSoundSource == s_slotMapping[slot] && uid == s_slotID[slot])
		{
			freeSource(curSoundSource);
			s_slotMapping[slot] = nullptr;
			s_slotID[slot] = 0;

			//Fluffy (DukeVoice)
			if(slot == dukeVoiceSlot)
				dukeVoiceSlot = -1;
		}
	}

	SoundSourceID sound_Load(const char* sound)
	{
		return SoundSourceID(TFE_VocAsset::getIndex(sound) + 1);
	}

	void sound_pitchShift(SoundEffectID soundId, s32 shift)
	{
		// STUB
	}

	void setSoundSourceVolume(SoundSourceID soundId, s32 volume)
	{
		// STUB
	}
}  // TFE_JediSound