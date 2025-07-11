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
void ANewtonShapeMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

