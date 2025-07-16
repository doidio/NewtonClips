// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonShapeMeshActor.h"


// Sets default values
ANewtonShapeMeshActor::ANewtonShapeMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANewtonShapeMeshActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANewtonShapeMeshActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LerpTime > DeltaTime && LerpTime > 0)
	{
		const float Alpha = FMath::Clamp<float>(DeltaTime / LerpTime, 0.f, 1.f);
		const auto Q = FQuat::Slerp(GetActorRotation().Quaternion(), TargetRotation, Alpha);
		const FVector L = (1 - Alpha) * GetActorLocation() + Alpha * TargetLocation;

		SetActorTransform(FTransform(Q, L));
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}

