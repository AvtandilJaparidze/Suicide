#include <API/ARK/Ark.h>
#include "Suicide.h"
#include <Permissions.h>
#pragma comment(lib, "Permissions.lib")
#pragma comment(lib, "ArkApi.lib")

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Suicide"].value(str, "No message")).c_str());
}

void SuicideCMD(AShooterPlayerController* AttackerShooterController, FString* message, int mode)
{
	if (AttackerShooterController && AttackerShooterController->PlayerStateField() && AttackerShooterController->GetPlayerCharacter())
	{
		const bool RequiresPermission = config["Suicide"]["SuicideRequiresPermission"];
		const bool KOSuicide = config["Suicide"]["CanSuicideWhileKnockedOut"];
		const bool HandCuffSuicide = config["Suicide"]["CanSuicideWithHandCuffs"];
		const bool PickedUPSuicide = config["Suicide"]["CanSuicideWhilePickedUP"];
		uint64 steamId = ArkApi::GetApiUtils().GetSteamIdFromController(AttackerShooterController);
		if (RequiresPermission && !Permissions::IsPlayerHasPermission(steamId, "Suicide"))
		{
			ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgNoPermission"));
			return;
		}
		if (!KOSuicide && !AttackerShooterController->GetPlayerCharacter()->IsConscious())
		{
			ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgUnconscious"));
			return;
		}
		if (AttackerShooterController->GetPlayerCharacter()->IsSitting(false))
		{
			ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgSitting"));
			return;
		}
		if (AttackerShooterController->GetPlayerCharacter()->CurrentWeaponField() && AttackerShooterController->GetPlayerCharacter()->CurrentWeaponField()->AssociatedPrimalItemField())
		{
			FString WeaponName;
			AttackerShooterController->GetPlayerCharacter()->CurrentWeaponField()->AssociatedPrimalItemField()->GetItemName(&WeaponName, false, true, nullptr);
			if (!HandCuffSuicide && WeaponName.Contains(L"Handcuffs"))
			{
				ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgHandcuffs"));
				return;
			}
		}
		if (!PickedUPSuicide && AttackerShooterController->GetPlayerCharacter()->bIsCarried()())
		{
			ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgPickedUP"));
			return;
		}
	    if (!ArkApi::GetApiUtils().IsRidingDino(AttackerShooterController))
		{
			AttackerShooterController->GetPlayerCharacter()->Suicide();
		}
		else
			ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgOnDino"));
		return;
	}
	else
		ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgAlreadyDead"));
	    return;

}


void Load()
{
	Log::Get().Init("Suicide");
	try
	{
		ReadConfig();
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
	try
	{
		ArkApi::GetCommands().AddChatCommand(*GetText("SuicideCMD"), &SuicideCMD);
		ArkApi::GetCommands().AddConsoleCommand("Suicide.Reload", &ReloadConfig);
		ArkApi::GetCommands().AddRconCommand("Suicide.Reload", &ReloadConfigRcon);
	}
	catch (const std::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
	}
}

void Unload()
{
	ArkApi::GetCommands().RemoveConsoleCommand("Suicide.Reload");
	ArkApi::GetCommands().RemoveRconCommand("Suicide.Reload");
	ArkApi::GetCommands().RemoveChatCommand(*GetText("SuicideCMD"));
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	}
	return TRUE;
}