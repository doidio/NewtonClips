// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NewtonGranularFluidActor.h"
#include "NewtonShapeMeshActor.h"
#include "NewtonSoftMeshActor.h"
#include "GameFramework/Actor.h"
#include "NewtonClipsDirectory.generated.h"

USTRUCT(BlueprintType)
struct FNewtonModel
{
	GENERATED_BODY()

	UPROPERTY()
	FString Sha1;

	UPROPERTY()
	float Scale;

	UPROPERTY()
	TArray<FNewtonShapeMesh> ShapeMesh;

	UPROPERTY()
	TArray<FNewtonSoftMesh> SoftMesh;

	UPROPERTY()
	TArray<FNewtonGranularFluid> GranularFluid;
};

USTRUCT(BlueprintType)
struct FNewtonState
{
	GENERATED_BODY()

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	FString BodyTransform;

	UPROPERTY()
	FString ParticlePosition;
};

UCLASS()
class NEWTONCLIPS_API ANewtonClipsDirectory : public AActor
{
	GENERATED_BODY()

	FTimerHandle Timer;
	FDateTime ModelFileTimeStamp;

	void OnTimer();

	UPROPERTY()
	TArray<ANewtonShapeMeshActor*> ShapeMeshActors;

	UPROPERTY()
	TArray<ANewtonSoftMeshActor*> SoftMeshActors;

	UPROPERTY()
	TArray<ANewtonGranularFluidActor*> GranularFluidActors;

	UPROPERTY()
	FVector3f DefaultVertexColor = {1, 0, 0};

	FDynamicMesh3 CreateDynamicMesh(const FString& Vertices, const FString& Indices) const;
	FDynamicMesh3 CreateDynamicMesh(const TArray<FVector3f>& Vertices, const TArray<FIntVector>& Triangles) const;

	UPROPERTY()
	FNewtonModel Model;

	void SpawnModel(const FNewtonModel& NewtonModel);
	void DestroyModel();

	UPROPERTY()
	TArray<FNewtonState> Frames;

	void PopulateFrameLast();
	void PopulateFrame(int32 FrameId);

	UPROPERTY()
	UMaterial* MUnlit = nullptr;

	UPROPERTY()
	UMaterial* MOpaque = nullptr;

	UPROPERTY()
	UMaterial* MTranslucent = nullptr;

	UPROPERTY()
	UNiagaraSystem* NSphere = nullptr;

public:
	// Sets default values for this actor's properties
	ANewtonClipsDirectory();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString Directory;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;
};
