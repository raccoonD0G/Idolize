// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "ActionComponent.generated.h"

USTRUCT()
struct FExecutableAction
{
    GENERATED_BODY()

    FString Description;
    float InteractRange;
    TFunction<void(class UActionComponent* Caller)> ActFunction;

    TFunction<int32(const class UActionComponent* Caller)> RewardFunction;

    TFunction<bool(const class UActionComponent* Caller)> IsAvailable;

    int32 GetExpectedReward(const class UActionComponent* Caller) const
    {
        return RewardFunction ? RewardFunction(Caller) : 0;
    }

    bool GetIsAvailable(const class UActionComponent* Caller) const
    {
        return IsAvailable ? IsAvailable(Caller) : true;
    }

    FExecutableAction()
    {

    }

    FExecutableAction(const FString& InDescription, float InInteractRange, TFunction<void(UActionComponent*)> InActFunction, TFunction<int32(const class UActionComponent* Caller)> InRewardFunction, TFunction<bool(const class UActionComponent* Caller)> InIsAvailable)
        : Description(InDescription), InteractRange(InInteractRange), ActFunction(InActFunction), RewardFunction(InRewardFunction), IsAvailable(InIsAvailable)
    {

    }
};

USTRUCT()
struct FActionInfo
{
    GENERATED_BODY()

    UPROPERTY()
    FString ActionName;

    UPROPERTY()
    FString Description;

    UPROPERTY()
    int32 ExpectedReward;
};

USTRUCT()
struct FActionInfosWrapper
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FActionInfo> ActionInfos;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class IDOLIZE_API UActionComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
    UActionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

// Interact Section
public:
    UPROPERTY()
    TMap<FString, FExecutableAction> ExecutableActionMap;

private:
    const TArray<UActionComponent*> GetScannedActionComponent(float Range) const;
    FString ScanAvailableActionsAsJson(float Range = 1000.0f) const;

private:
    UFUNCTION()
    void DoNothing(UActionComponent* Caller);
};
