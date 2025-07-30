// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMeshActor.h"
#include "NewtonSoftMeshActor.generated.h"

USTRUCT(BlueprintType)
struct FNewtonSoftMesh
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Begin = 0;

	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	FString Vertices;

	UPROPERTY()
	FString Indices;

	UPROPERTY()
	FString VertexColors;
};

UCLASS(Blueprintable)
class NEWTONCLIPS_API ANewtonSoftMeshActor : public ADynamicMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANewtonSoftMeshActor();

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 Begin = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Count = 0;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector3f> LerpParticlePositions;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector4f> LerpVertexColors;

	UPROPERTY(BlueprintReadWrite)
	float LerpTime;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
