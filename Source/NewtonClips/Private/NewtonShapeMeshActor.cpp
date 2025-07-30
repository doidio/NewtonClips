// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonShapeMeshActor.h"

#include "Components/BaseDynamicMeshSceneProxy.h"
#include "DynamicMesh/MeshNormals.h"


// Sets default values
ANewtonShapeMeshActor::ANewtonShapeMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BodyComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BodyComponent"));
	BodyComponent->SetMobility(EComponentMobility::Movable);
	DynamicMeshComponent->AttachToComponent(BodyComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SetRootComponent(BodyComponent);
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

	if (LerpTime > 0)
	{
		const float Alpha = FMath::Clamp<float>(DeltaTime / LerpTime, 0.f, 1.f);
		const auto Q = FQuat::Slerp(RootComponent->GetRelativeRotation().Quaternion(), LerpRotation, Alpha);
		const auto L = (1 - Alpha) * RootComponent->GetRelativeLocation() + Alpha * LerpLocation;

		RootComponent->SetRelativeTransform(FTransform(Q, L, RootComponent->GetRelativeScale3D()));

		auto Mesh = GetDynamicMeshComponent()->GetDynamicMesh()->GetMeshRef();

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

			GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));
			UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
			UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);
		}

		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}
