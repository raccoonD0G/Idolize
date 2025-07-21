// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RootMotionMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class IDOLIZE_API URootMotionMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	URootMotionMovementComponent();
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

protected:
	virtual void MaintainHorizontalGroundVelocity() override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

private:
	float PendingZCorrection;
	float LastPhysicsTime;
};
