// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ActionComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "Collision/Collision.h"
#include "JsonObjectConverter.h"

UActionComponent::UActionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    SetCollisionProfileName(COLLISION_PROFILE_INTERACTABLE);

    ExecutableActionMap.Add("DoNothing", FExecutableAction(
        TEXT("Does nothing."),
        UE_BIG_NUMBER,
        [this](UActionComponent* Caller)
        {
            DoNothing(Caller);
        },
        [this](const UActionComponent* Caller) -> int32
        {
            return -1;
        },
        [this](const UActionComponent* Caller) -> bool
        {
            return true;
        }
    ));
}

void UActionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


const TArray<UActionComponent*> UActionComponent::GetScannedActionComponent(float Range) const
{
    TArray<UActionComponent*> FoundComponents;

    UWorld* World = GetWorld();
    if (!World)
    {
        return FoundComponents;
    }

    TArray<FOverlapResult> OverlapResults;

    FCollisionQueryParams QueryParams;

    QueryParams.AddIgnoredActor(GetOwner());

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(COLLISION_INTERACTABLE);

    FVector ScanOrigin = GetComponentLocation();

    bool bHit = World->OverlapMultiByObjectType(
        OverlapResults,
        ScanOrigin,
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(Range),
        QueryParams
    );

    if (bHit)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            if (AActor* OverlappedActor = Result.GetActor())
            {
                TArray<UActionComponent*> Components;
                OverlappedActor->GetComponents<UActionComponent>(Components);

                for (UActionComponent* Comp : Components)
                {
                    if (Comp && Comp != this)
                    {
                        FoundComponents.Add(Comp);
                    }
                }
            }
        }
    }

    return FoundComponents;
}

FString UActionComponent::ScanAvailableActionsAsJson(float Range) const
{
    FActionInfosWrapper ActionInfosWrapper;

    TArray<UActionComponent*> NearbyComponents = GetScannedActionComponent(Range);

    for (UActionComponent* Comp : NearbyComponents)
    {
        if (!Comp) continue;

        for (const auto& Pair : Comp->ExecutableActionMap)
        {
            const FString& ActionName = Pair.Key;
            const FExecutableAction& Action = Pair.Value;

            if (Action.GetIsAvailable(this))
            {
                FActionInfo Info;
                Info.ActionName = ActionName;
                Info.Description = Action.Description;
                Info.ExpectedReward = Action.GetExpectedReward(this);
                ActionInfosWrapper.ActionInfos.Add(Info);
            }
        }
    }

    FString OutputString;
    FJsonObjectConverter::UStructToJsonObjectString(ActionInfosWrapper, OutputString);
    return OutputString;
}

void UActionComponent::DoNothing(UActionComponent* Caller)
{
}
