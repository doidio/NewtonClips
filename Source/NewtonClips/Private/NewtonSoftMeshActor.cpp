// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonSoftMeshActor.h"

#include "Components/BaseDynamicMeshSceneProxy.h"
#include "DynamicMesh/MeshNormals.h"
#include "Selection/MeshTopologySelectionMechanic.h"


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

	if (LerpTime > 0)
	{
		const float Alpha = FMath::Clamp<float>(DeltaTime / LerpTime, 0.f, 1.f);

		auto& Mesh = GetDynamicMeshComponent()->GetDynamicMesh()->GetMeshRef();

		ParallelFor(Mesh.VertexCount(), [&](const int32 VertexID)
		{
			if (LerpParticlePositions.IsValidIndex(VertexID))
			{
				const auto L = (1 - Alpha) * Mesh.GetVertex(VertexID) + Alpha *
					FVector(LerpParticlePositions[VertexID]);
				Mesh.SetVertex(VertexID, L);
			}
		});

		if (!LerpVertexColors.IsEmpty())
		{
			FDynamicMeshColorOverlay* ColorOverlay = Mesh.Attributes()->PrimaryColors();
			for (int i = 0; i < ColorOverlay->ElementCount(); ++i)
			{
				auto Old = ColorOverlay->GetElement(i);
				auto New = LerpVertexColors.IsValidIndex(i) ? LerpVertexColors[i] : LerpVertexColors[0];
				New = (1 - Alpha) * Old + Alpha * New;
				ColorOverlay->SetElement(i, New);
			}
		}

		GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));
		UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
		UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);

		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}
