#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EOSLoginLibrary.generated.h"

UCLASS()
class IT_LEARNS_API UEOSLoginLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EOS", meta = (WorldContext = "WorldContextObject"))
    static void LoginEOS(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "EOS", meta = (WorldContext = "WorldContextObject"))
    static void CreateEOSSession(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "EOS", meta = (WorldContext = "WorldContextObject"))
    static void FindAndJoinEOSSession(UObject* WorldContextObject);
};