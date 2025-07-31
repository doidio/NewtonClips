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
	int32 Body = -1;

	UPROPERTY()
	TArray<float> Transform = {0, 0, 0, 0, 0, 0, 1};

	UPROPERTY()
	FString Vertices;

	UPROPERTY()
	FString Indices;

	UPROPERTY()
	FString VertexHues;
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

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 Body = -1;

	UPROPERTY(BlueprintReadWrite)
	TArray<float> LerpTransform = {0, 0, 0, 0, 0, 0, 1};

	UPROPERTY(BlueprintReadWrite)
	TArray<float> LerpVertexHues;

	UPROPERTY(BlueprintReadWrite)
	float LerpTime;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> BodyComponent;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void Lerp(float Alpha = 1.0);
};
