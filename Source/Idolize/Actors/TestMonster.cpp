// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/TestMonster.h"
#include "Components/ActionComponent.h"
#include "GameFramework/Actor.h"

ATestMonster::ATestMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ActionComponent = CreateDefaultSubobject<UActionComponent>(TEXT("ActionComponent"));
	ActionComponent->SetupAttachment(RootComponent);

    ActionComponent->ExecutableActionMap.Add("MoveTowardMonster", FExecutableAction(
        TEXT("Move Toward to the monster."),
        1000.0f,
        [this](UActionComponent* Caller)
        {
            MoveTowardMonster(Caller);
        },
        [this](const UActionComponent* Caller) -> int32
        {
            return 3;
        },
        [this](const UActionComponent* Caller) -> bool
        {
            return true;
        }
    ));
}

void ATestMonster::BeginPlay()
{
	Super::BeginPlay();
}

void ATestMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATestMonster::MoveTowardMonster(UActionComponent* Caller)
{
    if (!Caller)
    {
        return;
    }

    AActor* CallerOwner = Caller->GetOwner();
    if (!CallerOwner)
    {
        return;
    }

    FVector CallerLocation = CallerOwner->GetActorLocation();
    FVector MonsterLocation = GetActorLocation();

    FVector Direction = (MonsterLocation - CallerLocation).GetSafeNormal();

    float MoveDistance = 100.0f;
    FVector NewLocation = CallerLocation + Direction * MoveDistance;

    CallerOwner->SetActorLocation(NewLocation);

    UE_LOG(LogTemp, Log, TEXT("%s is moving toward monster: %s"), *CallerOwner->GetName(), *GetName());
}