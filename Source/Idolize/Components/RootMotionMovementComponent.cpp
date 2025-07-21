// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/RootMotionMovementComponent.h"
#include "GameFramework/Character.h"

URootMotionMovementComponent::URootMotionMovementComponent()
{
	PendingZCorrection = 0.0f;
	LastPhysicsTime = 0.0f;
}

FVector URootMotionMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const
{
    return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
}

void URootMotionMovementComponent::MaintainHorizontalGroundVelocity()
{
	Super::MaintainHorizontalGroundVelocity();
}

void URootMotionMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode != 0 || !CharacterOwner || !UpdatedComponent)
	{
		Super::PhysCustom(DeltaTime, Iterations);
		return;
	}

	if (DeltaTime < MIN_TICK_TIME)
		return;

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	float RawDeltaTime = CurrentTime - LastPhysicsTime;
	float TrueDeltaTime = FMath::Clamp(RawDeltaTime, 0.f, 1.f / 30.f);
	LastPhysicsTime = CurrentTime;

	RestorePreAdditiveRootMotionVelocity();
	ApplyRootMotionToVelocity(DeltaTime);

	const float GravityZDelta = GetGravityZ() * TrueDeltaTime;
	const float PreGravityZDelta = Velocity.Z;

	if (PreGravityZDelta <= 0.f)
	{
		Velocity.Z += GravityZDelta;
	}

	if (PreGravityZDelta > 0.f && PendingZCorrection > 0.f)
	{
		const float MaxMoveZ = Velocity.Z * TrueDeltaTime;
		const float ConsumeZ = FMath::Min(PendingZCorrection, MaxMoveZ);
		const float RemainingZ = MaxMoveZ - ConsumeZ;

		Velocity.Z = RemainingZ / TrueDeltaTime;
		PendingZCorrection -= ConsumeZ;
	}

	const FVector DeltaMove = Velocity * TrueDeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(DeltaMove, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		const float DotUp = FVector::DotProduct(Hit.Normal, FVector::UpVector);
		const bool bWasFallingByGravityOnly = FMath::IsNearlyEqual(PreGravityZDelta, 0.f, 0.1f);

		if (DotUp > 0.7f && PreGravityZDelta < 0.f)
		{
			if (!FMath::IsNearlyZero(PreGravityZDelta, 0.1f))
			{
				const float Correction = -PreGravityZDelta * TrueDeltaTime;
				PendingZCorrection += Correction;
			}

			Velocity.Z = 0.f;

			const FVector XYMove = FVector(DeltaMove.X, DeltaMove.Y, 0.f);
			FHitResult XYHit;
			SafeMoveUpdatedComponent(XYMove, UpdatedComponent->GetComponentRotation(), true, XYHit);
		}
	}
}
