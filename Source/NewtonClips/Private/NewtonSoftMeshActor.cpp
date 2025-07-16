// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonSoftMeshActor.h"


// Sets default values
ANewtonSoftMeshActor::ANewtonSoftMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANewtonSoftMeshActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANewtonSoftMeshActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (LerpTime > DeltaTime && LerpTime > 0)
	{
		const float Alpha = FMath::Clamp<float>(DeltaTime / LerpTime, 0.f, 1.f);

		for (auto& Mesh = GetDynamicMeshComponent()->GetDynamicMesh()->GetMeshRef();
		     const auto VertexID : Mesh.VertexIndicesItr())
		{
			if (TargetLocation.IsValidIndex(VertexID))
			{
				const FVector L = (1 - Alpha) * Mesh.GetVertex(VertexID) + Alpha * FVector3d(TargetLocation[VertexID]);
				Mesh.SetVertex(VertexID, L);
			}
		}

		GetDynamicMeshComponent()->NotifyMeshModified();
		GetDynamicMeshComponent()->NotifyMeshVertexAttributesModified();
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}
