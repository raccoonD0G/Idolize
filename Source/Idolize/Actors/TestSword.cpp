// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/TestSword.h"
#include "Components/ActionComponent.h"
#include "Actors/TestMonster.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ATestSword::ATestSword()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    ActionComponent = CreateDefaultSubobject<UActionComponent>(TEXT("ActionComponent"));
    ActionComponent->SetupAttachment(RootComponent);

    bIsGrabbed = false;
}

void ATestSword::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATestSword, SwordHolder);
}

void ATestSword::BeginPlay()
{
    Super::BeginPlay();

    ActionComponent->ExecutableActionMap.Add("Grab", FExecutableAction(
        TEXT("Grab the sword."),
        200.0f,
        [this](UActionComponent* Caller)
        {
            Grab(Caller);
        },
        [this](const UActionComponent* Caller) -> int32
        {
            return 5;
        },
        [this](const UActionComponent* Caller) -> bool
        {
            return !bIsGrabbed;
        }
    ));

    ActionComponent->ExecutableActionMap.Add("Swing", FExecutableAction(
        TEXT("Swing the sword."),
        100.0f,
        [this](UActionComponent* Caller)
        {
            Swing(Caller);
        },
        [this](const UActionComponent* Caller) -> int32
        {
            FVector MyLocation = GetActorLocation();
            float SearchRadius = 100.0f;

            TArray<AActor*> FoundActors;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestMonster::StaticClass(), FoundActors);

            for (AActor* Actor : FoundActors)
            {
                if (Actor && FVector::Dist(Actor->GetActorLocation(), MyLocation) <= SearchRadius)
                {
                    return 10;
                }
            }
            return 0;
        },
        [this](const UActionComponent* Caller) -> bool
        {
            if (bIsGrabbed && Caller == ActionComponent)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    ));
}


void ATestSword::Grab(UActionComponent* Caller)
{
    bIsGrabbed = true;
    SwordHolder = Caller;
    UE_LOG(LogTemp, Log, TEXT("Sword grabbed by: %s"), *Caller->GetOwner()->GetName());
}

void ATestSword::Drop(UActionComponent* Caller)
{
    if (!bIsGrabbed)
    {
        UE_LOG(LogTemp, Warning, TEXT("You can't drop a sword you're not holding."));
        return;
    }

    bIsGrabbed = false;

    UE_LOG(LogTemp, Log, TEXT("Sword dropped by: %s"), *Caller->GetOwner()->GetName());
}

void ATestSword::Swing(UActionComponent* Caller)
{
    UE_LOG(LogTemp, Log, TEXT("Sword swung by: %s"), *Caller->GetOwner()->GetName());
}
