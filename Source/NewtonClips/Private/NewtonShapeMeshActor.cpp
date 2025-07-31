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
		Lerp(DeltaTime / LerpTime);
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
}

void ANewtonShapeMeshActor::Lerp(float Alpha)
{
	Alpha = FMath::Clamp<float>(Alpha, 0.f, 1.f);
	
	FVector P(LerpTransform[0], LerpTransform[1], LerpTransform[2]);
	FQuat Q(LerpTransform[3], LerpTransform[4], LerpTransform[5], LerpTransform[6]);
		
	Q = FQuat::Slerp(RootComponent->GetRelativeRotation().Quaternion(), Q, Alpha);
	P = (1 - Alpha) * RootComponent->GetRelativeLocation() + Alpha * P;

	RootComponent->SetRelativeTransform(FTransform(Q, P, RootComponent->GetRelativeScale3D()));

	auto Mesh = GetDynamicMeshComponent()->GetDynamicMesh()->GetMeshRef();

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

		GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));
		UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
		UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);
	}
}
