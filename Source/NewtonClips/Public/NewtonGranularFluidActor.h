// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMeshActor.h"
#include "NiagaraComponent.h"
#include "NewtonGranularFluidActor.generated.h"

USTRUCT(BlueprintType)
struct FNewtonGranularFluid
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int32 Begin = 0;

	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	FString Particles;
};

UCLASS(Blueprintable)
class NEWTONCLIPS_API ANewtonGranularFluidActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANewtonGranularFluidActor();

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 Begin = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 Count = 0;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector3f> ParticlePositions;

	UPROPERTY(BlueprintReadWrite)
	float LerpTime;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
