// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonGranularFluidActor.h"

#include "NiagaraDataInterfaceArrayFunctionLibrary.h"


// Sets default values
ANewtonGranularFluidActor::ANewtonGranularFluidActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetMobility(EComponentMobility::Movable);
	SetRootComponent(NiagaraComponent);
}

// Called when the game starts or when spawned
void ANewtonGranularFluidActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANewtonGranularFluidActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LerpTime > 0)
	{
		const float Alpha = FMath::Clamp<float>(DeltaTime / LerpTime, 0.f, 1.f);

		TArray<FVector> Positions = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayPosition(
			NiagaraComponent, FName(TEXT("Positions")));
		FBox Box;

		for (int32 VecID = 0; VecID < Positions.Num(); ++VecID)
		{
			if (ParticlePositions.IsValidIndex(VecID))
			{
				Box += Positions[VecID] = (1 - Alpha) * Positions[VecID] + Alpha * FVector(ParticlePositions[VecID]);
			}
		}

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(
			NiagaraComponent, FName(TEXT("Positions")), Positions);
		NiagaraComponent->SetSystemFixedBounds(Box);

		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}
