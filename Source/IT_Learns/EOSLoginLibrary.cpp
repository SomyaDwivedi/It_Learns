#include "EOSLoginLibrary.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

static TSharedPtr<FOnlineSessionSearch> GEOSSessionSearch;

static FDelegateHandle GEOSLoginDelegateHandle;
static bool bGEOSLoginInProgress = false;

static bool StartEOSAccountPortalLogin(const IOnlineIdentityPtr& Identity)
{
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("EOS AccountPortal Login Failed: Identity invalid"));
        return false;
    }

    FOnlineAccountCredentials Credentials;
    Credentials.Type = TEXT("accountportal");
    Credentials.Id = TEXT("");
    Credentials.Token = TEXT("");

    UE_LOG(LogTemp, Warning, TEXT("EOS cached login was unavailable. Opening Epic Account Portal once to create it..."));
    return Identity->Login(0, Credentials);
}

static void StartEOSAutoLogin(const IOnlineIdentityPtr& Identity)
{
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("EOS Auto Login Failed: Identity invalid"));
        return;
    }

    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Warning, TEXT("EOS Auto Login skipped: User 0 is already logged in"));
        return;
    }

    if (bGEOSLoginInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("EOS Auto Login skipped: A login is already in progress"));
        return;
    }

    if (GEOSLoginDelegateHandle.IsValid())
    {
        Identity->ClearOnLoginCompleteDelegate_Handle(0, GEOSLoginDelegateHandle);
        GEOSLoginDelegateHandle.Reset();
    }

    GEOSLoginDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(
        0,
        FOnLoginCompleteDelegate::CreateLambda(
            [Identity](int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
            {
                bGEOSLoginInProgress = false;

                if (Identity.IsValid() && GEOSLoginDelegateHandle.IsValid())
                {
                    Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, GEOSLoginDelegateHandle);
                    GEOSLoginDelegateHandle.Reset();
                }

                if (bWasSuccessful)
                {
                    UE_LOG(LogTemp, Warning, TEXT("EOS Auto Login Success. UserId: %s"), *UserId.ToString());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("EOS Auto Login Failed: %s"), *Error);
                }
            }
        )
    );

    UE_LOG(LogTemp, Warning, TEXT("Attempting EOS automatic login using command-line or cached credentials..."));
    bGEOSLoginInProgress = true;
    bool bLoginStarted = Identity->AutoLogin(0);

    if (!bLoginStarted)
    {
        bLoginStarted = StartEOSAccountPortalLogin(Identity);
    }

    if (!bLoginStarted)
    {
        bGEOSLoginInProgress = false;

        if (GEOSLoginDelegateHandle.IsValid())
        {
            Identity->ClearOnLoginCompleteDelegate_Handle(0, GEOSLoginDelegateHandle);
            GEOSLoginDelegateHandle.Reset();
        }

        UE_LOG(LogTemp, Error, TEXT("EOS Auto Login Failed: Neither automatic nor Account Portal login started"));
    }
}

void UEOSLoginLibrary::LoginEOS(UObject* WorldContextObject)
{
    (void)WorldContextObject;

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

    UE_LOG(LogTemp, Warning, TEXT("LOGIN CODE VERSION: PERSISTENT AUTOLOGIN 009"));
    StartEOSAutoLogin(Identity);
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
