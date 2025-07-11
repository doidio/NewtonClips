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

	UPROPERTY()
	FVector3f DefaultVertexColor = {1, 0, 0};

	FDynamicMesh3 CreateDynamicMesh(const FString& Vertices,
	                                const FString& Indices,
	                                const FString& VertexNormals,
	                                const FString& VertexUVs) const;
	FDynamicMesh3 CreateDynamicMesh(const TArray<FVector3f>& Vertices,
	                                const TArray<FIntVector>& Triangles,
	                                const TArray<FVector3f>& VertexNormals,
	                                const TArray<FVector2f>& VertexUVs) const;
	void SpawnActors(const FNewtonModel& NewtonModel);
	void DestroyActors();

	UPROPERTY()
	UMaterial* MUnlit = nullptr;

	UPROPERTY()
	UMaterial* MOpaque = nullptr;

	UPROPERTY()
	UMaterial* MTranslucent = nullptr;

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
