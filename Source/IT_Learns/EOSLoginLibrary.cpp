#include "EOSLoginLibrary.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

static TSharedPtr<FOnlineSessionSearch> GEOSSessionSearch;
static FDelegateHandle GEOSFindSessionsCompleteDelegateHandle;
static FDelegateHandle GEOSJoinSessionCompleteDelegateHandle;

void UEOSLoginLibrary::LoginEOS(UObject* WorldContextObject)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

    if (!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("EOS Login Failed: No Online Subsystem"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Online Subsystem Name: %s"), *Subsystem->GetSubsystemName().ToString());

    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();

    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("EOS Login Failed: No Identity Interface"));
        return;
    }

    TWeakObjectPtr<UObject> WeakWorldContext(WorldContextObject);

    FOnlineAccountCredentials Credentials;
    Credentials.Type = TEXT("accountportal");
    Credentials.Id = TEXT("");
    Credentials.Token = TEXT("");

    Identity->OnLoginCompleteDelegates->AddLambda(
        [WeakWorldContext, Identity](int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
        {
            if (bWasSuccessful)
            {
                UE_LOG(LogTemp, Warning, TEXT("EOS Login Success. UserId: %s"), *UserId.ToString());

                if (GEngine && WeakWorldContext.IsValid())
                {
                    UWorld* World = GEngine->GetWorldFromContextObject(
                        WeakWorldContext.Get(),
                        EGetWorldErrorMode::LogAndReturnNull
                    );

                    if (World && World->GetGameInstance())
                    {
                        ULocalPlayer* LocalPlayer = World->GetGameInstance()->GetFirstGamePlayer();

                        TSharedPtr<const FUniqueNetId> UniqueId = Identity->GetUniquePlayerId(LocalUserNum);

                        if (LocalPlayer && UniqueId.IsValid())
                        {
                            LocalPlayer->SetCachedUniqueNetId(UniqueId);
                            UE_LOG(LogTemp, Warning, TEXT("LocalPlayer UniqueNetId cached: %s"), *UniqueId->ToString());
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("Could not cache LocalPlayer UniqueNetId"));
                        }
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Login Failed: %s"), *Error);
            }
        }
    );

    UE_LOG(LogTemp, Warning, TEXT("LOGIN CODE VERSION: ACCOUNTPORTAL CACHE LOCALPLAYER 004"));

    Identity->Login(0, Credentials);
}

void UEOSLoginLibrary::CreateEOSSession(UObject* WorldContextObject)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

    if (!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateEOSSession Failed: No Online Subsystem"));
        return;
    }

    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
    IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();

    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateEOSSession Failed: No Identity Interface"));
        return;
    }

    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateEOSSession Failed: No Session Interface"));
        return;
    }

    TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);

    if (!UserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CreateEOSSession Failed: User is not logged in"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("CreateEOSSession Using UserId: %s"), *UserId->ToString());

    FOnlineSessionSettings SessionSettings;
    SessionSettings.NumPublicConnections = 4;
    SessionSettings.bIsLANMatch = false;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bUseLobbiesIfAvailable = false;

    SessionSettings.Set(
        SETTING_MAPNAME,
        FString("/Game/Variant_Horror/Lvl_Horror"),
        EOnlineDataAdvertisementType::ViaOnlineService
    );

    TWeakObjectPtr<UObject> WeakWorldContext(WorldContextObject);

    Sessions->OnCreateSessionCompleteDelegates.AddLambda(
        [WeakWorldContext](FName SessionName, bool bWasSuccessful)
        {
            if (bWasSuccessful)
            {
                UE_LOG(LogTemp, Warning, TEXT("EOS Create Session Success"));

                if (WeakWorldContext.IsValid())
                {
                    UGameplayStatics::OpenLevel(
                        WeakWorldContext.Get(),
                        FName("/Game/Variant_Horror/Lvl_Horror"),
                        true,
                        FString("listen")
                    );
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("EOS Create Session Success but WorldContext invalid"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Create Session Failed"));
            }
        }
    );

    Sessions->CreateSession(*UserId, NAME_GameSession, SessionSettings);
}

void UEOSLoginLibrary::FindAndJoinEOSSession(UObject* WorldContextObject)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

    if (!Subsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("FindAndJoinEOSSession Failed: No Online Subsystem"));
        return;
    }

    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
    IOnlineSessionPtr Sessions = Subsystem->GetSessionInterface();

    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FindAndJoinEOSSession Failed: No Identity Interface"));
        return;
    }

    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FindAndJoinEOSSession Failed: No Session Interface"));
        return;
    }

    TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);

    if (!UserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FindAndJoinEOSSession Failed: User is not logged in"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("FindAndJoinEOSSession Using UserId: %s"), *UserId->ToString());

    GEOSSessionSearch = MakeShareable(new FOnlineSessionSearch());
    GEOSSessionSearch->MaxSearchResults = 100;
    GEOSSessionSearch->bIsLanQuery = false;

    TWeakObjectPtr<UObject> WeakWorldContext(WorldContextObject);

    Sessions->OnFindSessionsCompleteDelegates.AddLambda(
        [Sessions, UserId, WeakWorldContext](bool bWasSuccessful)
        {
            if (!bWasSuccessful)
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Find Sessions Failed"));
                return;
            }

            if (!GEOSSessionSearch.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Find Sessions Failed: Search object invalid"));
                return;
            }

            UE_LOG(LogTemp, Warning, TEXT("EOS Find Sessions Success. Results: %d"), GEOSSessionSearch->SearchResults.Num());

            if (GEOSSessionSearch->SearchResults.Num() <= 0)
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Find Sessions Failed: No sessions found"));
                return;
            }

            FOnlineSessionSearchResult SearchResult = GEOSSessionSearch->SearchResults[0];

            Sessions->OnJoinSessionCompleteDelegates.AddLambda(
                [Sessions, WeakWorldContext](FName SessionName, EOnJoinSessionCompleteResult::Type Result)
                {
                    if (Result != EOnJoinSessionCompleteResult::Success)
                    {
                        UE_LOG(LogTemp, Error, TEXT("EOS Join Session Failed. Result: %d"), static_cast<int32>(Result));
                        return;
                    }

                    UE_LOG(LogTemp, Warning, TEXT("EOS Join Session Success"));

                    FString ConnectString;

                    if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
                    {
                        UE_LOG(LogTemp, Error, TEXT("EOS Join Session Failed: Could not get connect string"));
                        return;
                    }

                    UE_LOG(LogTemp, Warning, TEXT("EOS Join Connect String: %s"), *ConnectString);

                    if (!WeakWorldContext.IsValid())
                    {
                        UE_LOG(LogTemp, Error, TEXT("EOS Join Session Failed: World context invalid"));
                        return;
                    }

                    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WeakWorldContext.Get(), 0);

                    if (!PlayerController)
                    {
                        UE_LOG(LogTemp, Error, TEXT("EOS Join Session Failed: No Player Controller"));
                        return;
                    }

                    PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
                }
            );

            UE_LOG(LogTemp, Warning, TEXT("EOS Joining first found session"));

            bool bJoinStarted = Sessions->JoinSession(*UserId, NAME_GameSession, SearchResult);

            if (!bJoinStarted)
            {
                UE_LOG(LogTemp, Error, TEXT("EOS Join Session Failed: JoinSession did not start"));
            }
        }
    );

    bool bFindStarted = Sessions->FindSessions(*UserId, GEOSSessionSearch.ToSharedRef());

    if (!bFindStarted)
    {
        UE_LOG(LogTemp, Error, TEXT("EOS Find Sessions Failed: FindSessions did not start"));
    }
}
