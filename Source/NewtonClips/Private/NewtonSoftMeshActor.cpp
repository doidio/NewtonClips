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
		Lerp(DeltaTime / LerpTime);
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}

void ANewtonSoftMeshActor::Lerp(float Alpha)
{
	Alpha = FMath::Clamp<float>(Alpha, 0.f, 1.f);

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

	if (!LerpVertexHues.IsEmpty())
	{
		FDynamicMeshColorOverlay* ColorOverlay = Mesh.Attributes()->PrimaryColors();
		for (int i = 0; i < ColorOverlay->ElementCount(); ++i)
		{
			const auto Hue = LerpVertexHues.IsValidIndex(i) ? LerpVertexHues[i] : LerpVertexHues[0];
			auto To = FLinearColor(Hue, 1, 1, 1).HSVToLinearRGB();
			auto From = ColorOverlay->GetElement(i);
			ColorOverlay->SetElement(i, FLinearColor::LerpUsingHSV(From, To, Alpha));
		}
	}

	GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));
	UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
	UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);
}
