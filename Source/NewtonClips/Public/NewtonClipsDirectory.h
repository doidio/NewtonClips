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
	FString Uuid;

	UPROPERTY()
	float Scale = 1.0;

	UPROPERTY()
	int32 UpAxis = 2;

	UPROPERTY()
	TArray<FNewtonShapeMesh> ShapeMesh;

	UPROPERTY()
	TArray<FNewtonSoftMesh> SoftMesh;

	UPROPERTY()
	TArray<FNewtonGranularFluid> GranularFluid;

	UPROPERTY()
	FDateTime LastModified = FDateTime::MinValue();
};

USTRUCT(BlueprintType)
struct FNewtonState
{
	GENERATED_BODY()

	UPROPERTY()
	float DeltaTime = 0.0;

	UPROPERTY()
	FString BodyTransforms;

	UPROPERTY()
	FString ParticlePositions;

	UPROPERTY()
	TMap<int32, FString> ShapeVertexColors;

	UPROPERTY()
	TMap<int32, FString> ParticleColors;
};

UCLASS(Blueprintable)
class NEWTONCLIPS_API ANewtonClipsDirectory : public AActor
{
	GENERATED_BODY()

	const TCHAR* ConfigSection = TEXT("/Script/NewtonClips.ANewtonClipsDirectory");

	UPROPERTY()
	TArray<ANewtonShapeMeshActor*> ShapeMeshActors;

	UPROPERTY()
	TArray<ANewtonSoftMeshActor*> SoftMeshActors;

	UPROPERTY()
	TArray<ANewtonGranularFluidActor*> GranularFluidActors;

	FDynamicMesh3 CreateDynamicMesh(const FString& Vertices, const FString& Indices, const FString& VertexColors) const;
	FDynamicMesh3 CreateDynamicMesh(const TArray<FVector3f>& Vertices, const TArray<FIntVector>& Triangles,
	                                const TArray<FVector4f>& VertexColors) const;

	UPROPERTY()
	FNewtonModel Model;

	UPROPERTY()
	TArray<FNewtonState> Frames;

	void SpawnModel();
	void DestroyModel();

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

	UPROPERTY(BlueprintReadWrite)
	FString Directory;

	UFUNCTION(BlueprintCallable)
	void SaveAsStatic();

	UFUNCTION(BlueprintCallable)
	void RestoreFromStatic();

	UPROPERTY(BlueprintReadWrite)
	bool AutoPlay = false;

	UPROPERTY(BlueprintReadWrite)
	bool AutoLoop = false;

	UPROPERTY(BlueprintReadOnly)
	int32 FrameId = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 FrameNum = 0;

	UFUNCTION(BlueprintCallable)
	void SetFrameNext();

	UFUNCTION(BlueprintCallable)
	void SetFrame(int32 InFrameId);
	void SetFrame(const float DeltaTime, const TArray<TArray<float>>& BodyTransforms,
	              const TArray<FVector3f>& ParticlePositions,
	              const TMap<int32, TArray<FVector4f>>& ShapeVertexColors,
	              const TMap<int32, TArray<FVector4f>>& ParticleColors);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;
};
