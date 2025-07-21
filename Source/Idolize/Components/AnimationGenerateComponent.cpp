// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AnimationGenerateComponent.h"
#include "Misc/Compression.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UAnimationGenerateComponent::UAnimationGenerateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAnimationGenerateComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	check(Character);

	SkeletalMeshComponent = Character->GetMesh();
	SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	CapsuleComponent = Character->GetCapsuleComponent();
	CharacterMovementComponent = Character->GetCharacterMovement();

	PlayGeneratedAnimationFromPrompt(TEXT("the person walked forward and jump high."));
}

void UAnimationGenerateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector LFoot = SkeletalMeshComponent->GetBoneLocation(FName("m_avg_L_Foot"), EBoneSpaces::WorldSpace);
	FVector RFoot = SkeletalMeshComponent->GetBoneLocation(FName("m_avg_R_Foot"), EBoneSpaces::WorldSpace);
	float MinFootZ = FMath::Min(LFoot.Z, RFoot.Z);

	float CapsuleBottomZ = CapsuleComponent->GetComponentLocation().Z - CapsuleComponent->GetScaledCapsuleHalfHeight();
	float OffsetZ = CapsuleBottomZ - MinFootZ - CapsuleComponent->GetScaledCapsuleHalfHeight();

	FVector RelLoc = SkeletalMeshComponent->GetRelativeLocation();
	float SmoothedZ = FMath::FInterpTo(RelLoc.Z, OffsetZ, DeltaTime, 10.0f);
	SkeletalMeshComponent->SetRelativeLocation(FVector(RelLoc.X, RelLoc.Y, SmoothedZ));
}

bool UAnimationGenerateComponent::DecompressZlibJson(const TArray<uint8>& CompressedData, int32 OriginalSize, FString& OutJsonString)
{
	TArray<uint8> UncompressedData;
	UncompressedData.SetNum(OriginalSize);

	bool bSuccess = FCompression::UncompressMemory(
		NAME_Zlib,
		UncompressedData.GetData(),
		OriginalSize,
		CompressedData.GetData(),
		CompressedData.Num()
	);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to decompress zlib data."));
		FFileHelper::SaveArrayToFile(UncompressedData, *(FPaths::ProjectLogDir() + "LastFailedJson.json"));
		return false;
	}

	FUTF8ToTCHAR Converted((const ANSICHAR*)UncompressedData.GetData(), UncompressedData.Num());
	OutJsonString = FString(Converted.Length(), Converted.Get());

	return true;
}

bool UAnimationGenerateComponent::SendPromptAndReceiveJson(const FString& Prompt, FString& OutJsonString)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

	bool bIsValid;
	Addr->SetIp(TEXT("52.8.11.142"), bIsValid);
	Addr->SetPort(14327);

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("PromptSocket"), false);
	if (!Socket->Connect(*Addr))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connect to server."));
		return false;
	}

	FString PromptToSend = Prompt + TEXT("\n");
	TArray<uint8> PromptBytes;
	FTCHARToUTF8 Convert(*PromptToSend);
	PromptBytes.Append((uint8*)Convert.Get(), Convert.Length());

	int32 BytesSent = 0;
	Socket->Send(PromptBytes.GetData(), PromptBytes.Num(), BytesSent);

	uint8 HeaderBuffer[128] = {};

	int32 BytesRead = 0;
	Socket->Recv(HeaderBuffer, sizeof(HeaderBuffer), BytesRead);
	FString HeaderStr = FString(UTF8_TO_TCHAR(HeaderBuffer)).TrimStartAndEnd();

	TArray<FString> Sizes;
	HeaderStr.ParseIntoArray(Sizes, TEXT(":"));
	if (Sizes.Num() != 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid size header: %s"), *HeaderStr);
		Socket->Close(); return false;
	}
	int32 CompressedSize = FCString::Atoi(*Sizes[0]);
	int32 OriginalSize = FCString::Atoi(*Sizes[1]);

	const char* ReadyMsg = "READY\n";
	Socket->Send((uint8*)ReadyMsg, strlen(ReadyMsg), BytesSent);

	TArray<uint8> CompressedData;
	CompressedData.SetNum(CompressedSize);
	int32 TotalReceived = 0;

	while (TotalReceived < CompressedSize)
	{
		int32 Remaining = CompressedSize - TotalReceived;
		int32 ChunkSize = 65536;
		int32 ThisRead = 0;
		Socket->Recv(CompressedData.GetData() + TotalReceived, FMath::Min(ChunkSize, Remaining), ThisRead);
		if (ThisRead <= 0) break;
		TotalReceived += ThisRead;
	}

	Socket->Close();

	if (TotalReceived != CompressedSize)
	{
		UE_LOG(LogTemp, Error, TEXT("Did not receive full data. Expected: %d, Got: %d"), CompressedSize, TotalReceived);
		return false;
	}

	return DecompressZlibJson(CompressedData, OriginalSize, OutJsonString);
}

UAnimSequence* UAnimationGenerateComponent::ParseJsonAndGenerateAnimSequence(const FString& JsonStr)
{
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON."));
		return nullptr;
	}

	const FName RootBoneName = "m_avg_root";
	const FName PelvisBoneName = "m_avg_Pelvis";
	const TArray<FName> BodyBoneNames = {
		"m_avg_L_Hip",
		"m_avg_R_Hip",
		"m_avg_Spine1",
		"m_avg_L_Knee",
		"m_avg_R_Knee",
		"m_avg_Spine2",
		"m_avg_L_Ankle",
		"m_avg_R_Ankle",
		"m_avg_Spine3",
		"m_avg_L_Foot",
		"m_avg_R_Foot",
		"m_avg_Neck",
		"m_avg_L_Collar",
		"m_avg_R_Collar",
		"m_avg_Head",
		"m_avg_L_Shoulder",
		"m_avg_R_Shoulder",
		"m_avg_L_Elbow",
		"m_avg_R_Elbow",
		"m_avg_L_Wrist",
		"m_avg_R_Wrist",
		// "m_avg_L_Hand",
		// "m_avg_R_Hand"
	};


	const TArray<TSharedPtr<FJsonValue>>* GlobalOrientArr;
	const TArray<TSharedPtr<FJsonValue>>* BodyPoseArr;
	const TArray<TSharedPtr<FJsonValue>>* TranslArr;

	if (!Root->TryGetArrayField(TEXT("global_orient"), GlobalOrientArr) ||
		!Root->TryGetArrayField(TEXT("body_pose"), BodyPoseArr) ||
		!Root->TryGetArrayField(TEXT("transl"), TranslArr))
	{
		UE_LOG(LogTemp, Error, TEXT("Missing global_orient / body_pose / transl"));
		return nullptr;
	}

	const int32 NumFrames = GlobalOrientArr->Num();
	if (BodyPoseArr->Num() != NumFrames || TranslArr->Num() != NumFrames)
	{
		UE_LOG(LogTemp, Error, TEXT("Mismatched frame count."));
		return nullptr;
	}

	const float FrameRate = 20.0f;
	AnimSeq = NewObject<UAnimSequence>(GetTransientPackage(), NAME_None, RF_Public | RF_Standalone);
	AnimSeq->SetSkeleton(SkeletalMesh->Skeleton);
	AnimSeq->bEnableRootMotion = true;
	AnimSeq->RootMotionRootLock = ERootMotionRootLock::RefPose;
	AnimSeq->bForceRootLock = false;

	IAnimationDataController& Controller = AnimSeq->GetController();
	Controller.InitializeModel();
	Controller.SetFrameRate(FFrameRate(FrameRate, 1));
	Controller.SetNumberOfFrames(NumFrames);
	Controller.OpenBracket(FText::FromString(TEXT("Import SMPL Animation")));

	USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

	{
		int32 RefBoneIndex = RefSkeleton.FindBoneIndex(RootBoneName);
		if (RefBoneIndex != INDEX_NONE)
		{
			TArray<FVector3f> PosKeys;
			TArray<FQuat4f> RotKeys;
			TArray<FVector3f> ScaleKeys;

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				const TArray<TSharedPtr<FJsonValue>>& Transl = (*TranslArr)[Frame]->AsArray();

				float SMPL_X = Transl[0]->AsNumber();
				float SMPL_Y = Transl[1]->AsNumber();
				float SMPL_Z = Transl[2]->AsNumber();

				FVector3f Pos = FVector3f(SMPL_X * 100.f, SMPL_Z * 100.f, SMPL_Y * 100.f);

				PosKeys.Add(Pos);
				ScaleKeys.Add(FVector3f::OneVector);

				FQuat RefRot = RefSkeleton.GetRefBonePose()[RefBoneIndex].GetRotation();
				RotKeys.Add(FQuat4f(RefRot));
			}

			if (Controller.AddBoneCurve(RootBoneName))
			{
				Controller.SetBoneTrackKeys(RootBoneName, PosKeys, RotKeys, ScaleKeys);
			}
		}
	}

	{
		int32 RefBoneIndex = RefSkeleton.FindBoneIndex(PelvisBoneName);
		if (RefBoneIndex != INDEX_NONE)
		{
			TArray<FVector3f> PosKeys;
			TArray<FQuat4f> RotKeys;
			TArray<FVector3f> ScaleKeys;

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				const TArray<TSharedPtr<FJsonValue>>& Global = (*GlobalOrientArr)[Frame]->AsArray();
				FVector RotVec(Global[0]->AsNumber(), Global[1]->AsNumber(), Global[2]->AsNumber());

				float AngleRad = RotVec.Size();
				FVector Axis = (AngleRad < KINDA_SMALL_NUMBER) ? FVector::ForwardVector : RotVec / AngleRad;
				FVector AxisUE = FVector(Axis.X, -Axis.Y, Axis.Z);
				FQuat Quat = FQuat(AxisUE, AngleRad).GetNormalized();

				FQuat RefPoseRot = RefSkeleton.GetRefBonePose()[RefBoneIndex].GetRotation();
				FQuat FinalQuat = RefPoseRot.Inverse() * Quat;

				RotKeys.Add(FQuat4f(FinalQuat));
				PosKeys.Add((FVector3f)RefSkeleton.GetRefBonePose()[RefBoneIndex].GetLocation());
				ScaleKeys.Add(FVector3f::OneVector);
			}

			if (Controller.AddBoneCurve(PelvisBoneName))
			{
				Controller.SetBoneTrackKeys(PelvisBoneName, PosKeys, RotKeys, ScaleKeys);
			}
		}
	}

	for (int32 i = 0; i < BodyBoneNames.Num(); ++i)
	{
		const FName& BoneName = BodyBoneNames[i];
		int32 RefBoneIndex = RefSkeleton.FindBoneIndex(BoneName);
		if (RefBoneIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Bone not found: %s"), *BoneName.ToString());
			continue;
		}

		TArray<FVector3f> PosKeys;
		TArray<FQuat4f> RotKeys;
		TArray<FVector3f> ScaleKeys;

		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			const TArray<TSharedPtr<FJsonValue>>& Body = (*BodyPoseArr)[Frame]->AsArray();

			int32 Offset = i * 3;
			if (Offset + 2 >= Body.Num())
			{
				UE_LOG(LogTemp, Error, TEXT("Invalid offset for %s: %d"), *BoneName.ToString(), Offset);
				continue;
			}

			FVector RotVec = FVector(
				Body[Offset + 0]->AsNumber(),
				Body[Offset + 1]->AsNumber(),
				Body[Offset + 2]->AsNumber());

			float AngleRad = RotVec.Size();
			FVector Axis = (AngleRad < KINDA_SMALL_NUMBER) ? FVector::ForwardVector : RotVec / AngleRad;
			FVector AxisUE = FVector(Axis.X, -Axis.Y, Axis.Z);
			FQuat Quat = FQuat(AxisUE, AngleRad);
			Quat.Normalize();

			FQuat RefPoseRot = RefSkeleton.GetRefBonePose()[RefBoneIndex].GetRotation();
			FQuat FinalQuat = RefPoseRot.Inverse() * Quat;
			RotKeys.Add(FQuat4f(FinalQuat));

			FVector3f Pos = (FVector3f)RefSkeleton.GetRefBonePose()[RefBoneIndex].GetLocation();
			PosKeys.Add(Pos);
			ScaleKeys.Add(FVector3f::OneVector);
		}

		if (Controller.AddBoneCurve(BoneName))
		{
			Controller.SetBoneTrackKeys(BoneName, PosKeys, RotKeys, ScaleKeys);
		}
	}


	Controller.NotifyPopulated();
	Controller.CloseBracket();

	return AnimSeq;
}

void UAnimationGenerateComponent::PlayGeneratedAnimationFromPrompt(const FString& Prompt)
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Prompt]()
		{
			FString JsonStr;
			if (!SendPromptAndReceiveJson(Prompt, JsonStr))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get JSON from prompt."));
				return;
			}

			AsyncTask(ENamedThreads::GameThread, [this, JsonStr]()
				{
					UAnimSequence* Anim = ParseJsonAndGenerateAnimSequence(JsonStr);
					if (!Anim)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to generate animation sequence"));
						return;
					}

					CharacterMovementComponent->SetMovementMode(MOVE_Custom, 0);

					UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
					if (!AnimInstance)
					{
						UE_LOG(LogTemp, Warning, TEXT("No AnimInstance found on SkeletalMeshComponent"));
						return;
					}

					AnimInstance->OnMontageEnded.Clear();
					AnimInstance->OnMontageEnded.AddDynamic(this, &UAnimationGenerateComponent::OnMontageEnded);

					UAnimMontage* Montage = AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, FName("DefaultSlot"), 0.2f, 0.2f, 1.0f, 10);

					if (!Montage)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to create dynamic montage"));
					}
				});
		});
}

void UAnimationGenerateComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	CharacterMovementComponent->SetMovementMode(MOVE_Walking);
}
