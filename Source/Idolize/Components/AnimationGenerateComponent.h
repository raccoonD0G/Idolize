// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimationGenerateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class IDOLIZE_API UAnimationGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimationGenerateComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool DecompressZlibJson(const TArray<uint8>& CompressedData, int32 OriginalSize, FString& OutJsonString);
	bool SendPromptAndReceiveJson(const FString& Prompt, FString& OutJsonString);
	UAnimSequence* ParseJsonAndGenerateAnimSequence(const FString& JsonStr);

	void PlayGeneratedAnimationFromPrompt(const FString& Prompt);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAnimSequence> AnimSeq;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UCharacterMovementComponent> CharacterMovementComponent;
};
