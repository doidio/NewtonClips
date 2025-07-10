// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonClipsDirectory.h"

#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ANewtonClipsDirectory::ANewtonClipsDirectory()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;
}

// Called every frame
void ANewtonClipsDirectory::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called when the game starts or when spawned
void ANewtonClipsDirectory::BeginPlay()
{
	Super::BeginPlay();

	ModelFileTimeStamp = FDateTime::MinValue();

	GetWorld()->GetTimerManager().SetTimer(
		Timer, this, &ANewtonClipsDirectory::OnTimer, 1.f, true, 1.f);
}

void ANewtonClipsDirectory::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(Timer);
}

void ANewtonClipsDirectory::OnTimer()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FDateTime TimeStamp;

	if (const auto ModelFile = FPaths::Combine(Directory, TEXT("model.json"));
		(TimeStamp = PlatformFile.GetTimeStamp(*ModelFile)) > ModelFileTimeStamp)
	{
		ModelFileTimeStamp = TimeStamp;

		UKismetSystemLibrary::PrintString(
			GetWorld(), ModelFile, true, true, FColor::Red, 5);

		UKismetSystemLibrary::PrintString(
			GetWorld(), ModelFileTimeStamp.ToString(), true, true, FColor::Red, 5);

		// TODO: read json create DynamicMesh
	}

	// TODO: check state
}
