// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestSword.generated.h"

UCLASS()
class IDOLIZE_API ATestSword : public AActor
{
    GENERATED_BODY()

public:
    ATestSword();

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

private:
    uint8 bIsGrabbed : 1;

    UPROPERTY(Replicated)
    TObjectPtr<class UActionComponent> SwordHolder;

// Action Section
public:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<class UActionComponent> ActionComponent;

private:
    UFUNCTION()
    void Grab(class UActionComponent* Caller);

    UFUNCTION()
    void Drop(UActionComponent* Caller);

    UFUNCTION()
    void Swing(class UActionComponent* Caller);


};