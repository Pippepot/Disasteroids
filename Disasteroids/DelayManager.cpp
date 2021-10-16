#include "DelayManager.h"

void DelayManager::Update(float fElapsedTime)
{
	for (int i = 0; i < delays.size(); i++)
	{
		delays[i] -= fElapsedTime;
	}
}

void DelayManager::PutOnCooldown(delayTypes type)
{
	int i = static_cast<int>(type);
	delays[i] = durations[i];
}

bool DelayManager::OnCooldown(delayTypes type)
{
	if (delays[static_cast<int>(type)] > 0)
		return true;
	return false;
}
