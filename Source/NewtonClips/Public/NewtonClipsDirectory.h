// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NewtonClipsDirectory.generated.h"

UCLASS()
class NEWTONCLIPS_API ANewtonClipsDirectory : public AActor
{
	GENERATED_BODY()
	
	FTimerHandle Timer;
	FDateTime ModelFileTimeStamp;
	
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
	
	FDelegateHandle WatcherDelegate;
	void OnTimer();
};
