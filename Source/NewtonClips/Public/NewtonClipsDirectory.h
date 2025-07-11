// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NewtonShapeMeshActor.h"
#include "NewtonSoftMeshActor.h"
#include "GameFramework/Actor.h"
#include "Generators/MeshShapeGenerator.h"
#include "NewtonClipsDirectory.generated.h"

USTRUCT(BlueprintType)
struct FNewtonModel
{
	GENERATED_BODY()

	UPROPERTY()
	FString Uuid;

	UPROPERTY()
	TArray<FNewtonShapeMesh> ShapeMesh;

	UPROPERTY()
	TArray<FNewtonSoftMesh> SoftMesh;
};

class FNewtonMeshGenerator final : public UE::Geometry::FMeshShapeGenerator
{
public:
	virtual FMeshShapeGenerator& Generate() override { return *this; }

	FNewtonMeshGenerator* Copy(const FString& CacheDir, const FString& InVertices, const FString& InIndices,
							   const FString& InVertexNormals, const FString& InVertexUVs);

	FNewtonMeshGenerator* Copy(const TArray<FVector3f>& InVertices, const TArray<FIntVector>& InTriangles,
	                           const TArray<FVector3f>& InVertexNormals, const TArray<FVector2f>& InVertexUVs);
};

UCLASS()
class NEWTONCLIPS_API ANewtonClipsDirectory : public AActor
{
	GENERATED_BODY()

	FTimerHandle Timer;
	FDateTime ModelFileTimeStamp;

	void OnTimer();

	FString Uuid;

	UPROPERTY()
	TArray<ANewtonShapeMeshActor*> ShapeActors;

	UPROPERTY()
	TArray<ANewtonSoftMeshActor*> SoftActors;

	void SpawnActors(const FNewtonModel& NewtonModel);
	void DestroyActors();

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
