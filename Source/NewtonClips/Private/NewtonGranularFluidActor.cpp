// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonGranularFluidActor.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"

using UNiaLib = UNiagaraDataInterfaceArrayFunctionLibrary;

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
		Lerp(DeltaTime / LerpTime);
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}

void ANewtonGranularFluidActor::Lerp(float Alpha)
{
	Alpha = FMath::Clamp<float>(Alpha, 0.f, 1.f);

	TArray<FVector> Positions = UNiaLib::GetNiagaraArrayPosition(NiagaraComponent, FName(TEXT("Positions")));
	FBox Box;

	for (int32 VecID = 0; VecID < Count; ++VecID)
	{
		if (!LerpParticlePositions.IsValidIndex(VecID)) continue;

		if (!Positions.IsValidIndex(VecID))
		{
			Positions.AddZeroed();
		}
		Box += Positions[VecID] = (1 - Alpha) * Positions[VecID] + Alpha * FVector(LerpParticlePositions[VecID]);
	}

	UNiaLib::SetNiagaraArrayPosition(NiagaraComponent, FName(TEXT("Positions")), Positions);
	NiagaraComponent->SetVariableInt(FName(TEXT("Count")), Count);
	NiagaraComponent->SetSystemFixedBounds(Box);

	TArray<FLinearColor> Colors = UNiaLib::GetNiagaraArrayColor(NiagaraComponent, FName(TEXT("Colors")));

	for (int32 VecID = 0; VecID < Count; ++VecID)
	{
		if (!LerpParticleHues.IsValidIndex(VecID)) continue;

		const auto Hue = LerpParticleHues[VecID];
		auto To = FLinearColor(Hue, 1, 1, 1).HSVToLinearRGB();

		if (!Colors.IsValidIndex(VecID))
		{
			Colors.Add(FLinearColor::White);
		}
		Colors[VecID] = FLinearColor::LerpUsingHSV(Colors[VecID], To, Alpha);
	}

	UNiaLib::SetNiagaraArrayColor(NiagaraComponent, FName(TEXT("Colors")), Colors);
}
