// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMeshActor.h"
#include "NewtonShapeMeshActor.generated.h"

USTRUCT(BlueprintType)
struct FNewtonShapeMesh
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Body = 0;

	UPROPERTY()
	TArray<float> Transform;

	UPROPERTY()
	TArray<float> Scale;

	UPROPERTY()
	FString Vertices;

	UPROPERTY()
	FString Indices;

	UPROPERTY()
	FString VertexNormals;

	UPROPERTY()
	FString VertexUVs;
};

/**
 * 
 */
UCLASS(Blueprintable)
class NEWTONCLIPS_API ANewtonShapeMeshActor : public ADynamicMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANewtonShapeMeshActor();
	
	UPROPERTY(BlueprintReadOnly)
	FString Vertices;

	UPROPERTY(BlueprintReadOnly)
	FString Indices;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
