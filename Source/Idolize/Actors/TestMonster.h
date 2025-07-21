// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestMonster.generated.h"

UCLASS()
class IDOLIZE_API ATestMonster : public AActor
{
	GENERATED_BODY()
	
public:
	ATestMonster();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

// Action Section
public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UActionComponent> ActionComponent;

private:
	UFUNCTION()
	void MoveTowardMonster(UActionComponent* Caller);

// Stat Section
private:
	UPROPERTY(EditAnywhere, Category = "Stat")
	int32 HP = 100;

	UPROPERTY(EditAnywhere, Category = "Stat")
	int32 GoldReward = 50;
};
