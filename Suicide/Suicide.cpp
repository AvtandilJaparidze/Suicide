#include <API/ARK/Ark.h>
#include "Suicide.h"
#include <ArkPermissions.h>
#pragma comment(lib, "Permissions.lib")
#pragma comment(lib, "ArkApi.lib")

FString GetText(const std::string& str)
{
	return FString(ArkApi::Tools::Utf8Decode(config["Suicide"].value(str, "No message")).c_str());
}

FString GetBuffBlueprint(APrimalBuff* buff)
{
	if (buff) {
		FString path_name;
		NativeCall<FString*, FString*, UObject*>(reinterpret_cast<UObjectBaseUtility*>(buff), "UObjectBaseUtility.GetFullName", &path_name, nullptr);

		if (int find_index = 0; path_name.FindChar(' ', find_index))
		{
			path_name = "Blueprint'" + path_name.Mid(find_index + 1,
				path_name.Len() - (find_index + (path_name.EndsWith(
					"_C", ESearchCase::
					CaseSensitive)
					? 3
					: 1))) + "'";
			return path_name.Replace(L"Default__", L"", ESearchCase::CaseSensitive);
		}
	}
	return FString("");
}

void SuicideCMD(AShooterPlayerController* AttackerShooterController, FString* message, int mode)
{
	if (AttackerShooterController && AttackerShooterController->PlayerStateField() && AttackerShooterController->GetPlayerCharacter())
	{
		const bool RequiresPermission = config["Suicide"]["SuicideRequiresPermission"];
		const bool KOSuicide = config["Suicide"]["CanSuicideWhileKnockedOut"];
		const bool HandCuffSuicide = config["Suicide"]["CanSuicideWithHandCuffs"];
		const bool PickedUPSuicide = config["Suicide"]["CanSuicideWhilePickedUP"];
		const bool NoglinControlled = config["Suicide"]["CanSuicideNoglinControlled"];
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
			if (!HandCuffSuicide && ArkApi::GetApiUtils().GetItemBlueprint(AttackerShooterController->GetPlayerCharacter()->CurrentWeaponField()->AssociatedPrimalItemField()).Contains("Handcuffs"))
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

		if (!NoglinControlled)
		{
			for (auto buff : AttackerShooterController->GetPlayerCharacter()->BuffsField())
			{
				Log::GetLog()->info(GetBuffBlueprint(buff).ToString());
				if (GetBuffBlueprint(buff).Contains("Buff_BrainSlug_HumanControl"))
				{
					ArkApi::GetApiUtils().SendChatMessage(AttackerShooterController, *GetText("Sender"), *GetText("MsgNoglinControlled"));
					return;
				}

			}
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
	catch (nlohmann::json::exception& error)
	{
		Log::GetLog()->error(error.what());
		throw;
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