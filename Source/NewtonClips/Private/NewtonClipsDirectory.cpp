// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonClipsDirectory.h"

#include "JsonObjectConverter.h"
#include "NewtonClips.h"
#include "Components/BaseDynamicMeshSceneProxy.h"
#include "Curve/DynamicGraph.h"
#include "DynamicMesh/MeshNormals.h"
#include "Operations/RepairOrientation.h"
#include "Selection/MeshTopologySelectionMechanic.h"

using FM = ConstructorHelpers::FObjectFinder<UMaterial>;

// Sets default values
ANewtonClipsDirectory::ANewtonClipsDirectory()
{
	static FM MatUnlit(TEXT("Material'/NewtonClips/MatUnlit.MatUnlit'"));
	check(MatUnlit.Object);
	MUnlit = MatUnlit.Object;

	static FM MatOpaque(TEXT("Material'/NewtonClips/MatOpaque.MatOpaque'"));
	check(MatOpaque.Object);
	MOpaque = MatOpaque.Object;

	static FM MatTranslucent(TEXT("Material'/NewtonClips/MatTranslucent.MatTranslucent'"));
	check(MatTranslucent.Object);
	MTranslucent = MatTranslucent.Object;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);
}

// Called every frame
void ANewtonClipsDirectory::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	FString Filename = FString::Printf(TEXT("%d.json"), Frames.Num());
	if (const auto File = FPaths::Combine(Directory, TEXT("frames"), Filename);
		FPaths::FileExists(File))
	{
		if (FString JsonString; FFileHelper::LoadFileToString(JsonString, *File))
		{
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

			if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				if (FNewtonState NewtonState; FJsonObjectConverter::JsonObjectToUStruct(
					JsonObject.ToSharedRef(), FNewtonState::StaticStruct(), &NewtonState))
				{
					Frames.Add(NewtonState);
					PopulateFrameLast();
				}
				else UE_LOG(LogNewtonClips, Error, TEXT("Failed to convert JSON file: %s"), *File);
			}
			else UE_LOG(LogNewtonClips, Error, TEXT("Failed to parse JSON file: %s"), *File);
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Failed to load JSON file: %s"), *File);
	}
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
	DestroyModel();
}

void ANewtonClipsDirectory::OnTimer()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FDateTime TimeStamp;

	if (const auto File = FPaths::Combine(Directory, TEXT("model.json"));
		(TimeStamp = PlatformFile.GetTimeStamp(*File)) > ModelFileTimeStamp)
	{
		ModelFileTimeStamp = TimeStamp;

		if (FString JsonString; FFileHelper::LoadFileToString(JsonString, *File))
		{
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

			if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				if (FNewtonModel NewtonModel; FJsonObjectConverter::JsonObjectToUStruct(
					JsonObject.ToSharedRef(), FNewtonModel::StaticStruct(), &NewtonModel))
				{
					if (Model.Sha1 != NewtonModel.Sha1)
					{
						Model = NewtonModel;
						Frames.Empty();
						DestroyModel();
						SpawnModel(NewtonModel);
					}
				}
				else UE_LOG(LogNewtonClips, Error, TEXT("Failed to convert JSON file: %s"), *File);
			}
			else UE_LOG(LogNewtonClips, Error, TEXT("Failed to parse JSON file: %s"), *File);
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Failed to load JSON file: %s"), *File);
	}
}

FDynamicMesh3 ANewtonClipsDirectory::CreateDynamicMesh(const FString& Vertices, const FString& Indices) const
{
	TArray<FVector3f> OutVertices;
	TArray<FIntVector> OutTriangles;

	FString File;
	FString CacheDir = FPaths::Combine(Directory, TEXT(".cache"));

	if (TArray<uint8> Bytes;
		!Vertices.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, Vertices)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		OutVertices.SetNumUninitialized(Bytes.Num() / sizeof(FVector3f));
		FMemory::Memcpy(OutVertices.GetData(), Bytes.GetData(), Bytes.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid Vertices cache: %s"), *Vertices);

	if (TArray<uint8> Bytes;
		!Indices.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, Indices)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		OutTriangles.SetNumUninitialized(Bytes.Num() / sizeof(FIntVector));
		FMemory::Memcpy(OutTriangles.GetData(), Bytes.GetData(), Bytes.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid Indices cache: %s"), *Indices);

	return CreateDynamicMesh(OutVertices, OutTriangles);
}

FDynamicMesh3 ANewtonClipsDirectory::CreateDynamicMesh(const TArray<FVector3f>& Vertices,
                                                       const TArray<FIntVector>& Triangles) const
{
	FDynamicMesh3 Mesh;

	Mesh.EnableTriangleGroups();

	const int N = Vertices.Num();
	for (int i = 0; i < N; ++i)
	{
		Mesh.AppendVertex(FVector3d(Vertices[i]));
	}

	for (const auto& Triangle : Triangles)
	{
		FIntVector Tri = {Triangle[2], Triangle[1], Triangle[0]};
		const int32 Tid = Mesh.AppendTriangle(Tri, 0);

		if (Tid == FDynamicMesh3::InvalidID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Invalid Triangle: %s (%d)"), *Triangle.ToString(), N);
			return {};
		}
		if (Tid == FDynamicMesh3::NonManifoldID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("NonManifold Triangle: %s (%d)"), *Triangle.ToString(), N);
			return {};
		}
		if (Tid == FDynamicMesh3::DuplicateTriangleID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Duplicate Triangle: %s (%d)"), *Triangle.ToString(), N);
			return {};
		}
	}

	Mesh.EnableAttributes();
	// DynamicMesh.EnableVertexUVs({}); TODO: ComputeUVs_PatchBuilder(DynamicMesh, nullptr);
	Mesh.EnableVertexColors(DefaultVertexColor);

	UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
	UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);

	return Mesh;
}

void ANewtonClipsDirectory::SpawnModel(const FNewtonModel& NewtonModel)
{
	SetActorRelativeScale3D(FVector(NewtonModel.Scale * 100)); // m to cm

	int32 i = 0;

	// ReSharper disable once CppUseStructuredBinding
	for (const auto& Item : NewtonModel.ShapeMesh)
	{
		FDynamicMesh3 Mesh = CreateDynamicMesh(Item.Vertices, Item.Indices);

		UE_LOG(LogNewtonClips, Log, TEXT("[ ShapeMesh: %d ]"), i);
		UE_LOG(LogNewtonClips, Log, TEXT("%s"), *Mesh.MeshInfoString());

		auto Actor = GetWorld()->SpawnActor<ANewtonShapeMeshActor>();
		Actor->Name = Item.Name;
		Actor->Body = Item.Body;
		Actor->GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));

		Actor->GetDynamicMeshComponent()->GetDynamicMesh()->EditMesh([&](FDynamicMesh3& EditMesh)
		{
			UE::Geometry::FMeshRepairOrientation Repair(&EditMesh);
			Repair.OrientComponents();
			FDynamicMeshAABBTree3 Tree(&EditMesh, true);
			Repair.SolveGlobalOrientation(&Tree);
		}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonShapeMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		// convert from right-hand-system
		const FVector L(Item.Transform[0], Item.Transform[1], Item.Transform[2]);
		const FQuat Q(Item.Transform[3], Item.Transform[4], Item.Transform[5], Item.Transform[6]);
		const FVector S(Item.Scale[0], Item.Scale[1], Item.Scale[2]);
		Actor->GetDynamicMeshComponent()->SetRelativeTransform(FTransform(Q, L, S));

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		ShapeActors.Add(Actor);
		++i;
	}

	i = 0;

	// ReSharper disable once CppUseStructuredBinding
	for (const auto& Item : NewtonModel.SoftMesh)
	{
		FDynamicMesh3 Mesh = CreateDynamicMesh(Item.Vertices, Item.Indices);

		UE_LOG(LogNewtonClips, Log, TEXT("[ SoftMesh: %d ]"), i);
		UE_LOG(LogNewtonClips, Log, TEXT("%s"), *Mesh.MeshInfoString());

		auto Actor = GetWorld()->SpawnActor<ANewtonSoftMeshActor>();
		Actor->Name = Item.Name;
		Actor->Begin = Item.Begin;
		Actor->Count = Item.Count;
		Actor->GetDynamicMeshComponent()->SetMesh(MoveTemp(Mesh));

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonSoftMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		SoftActors.Add(Actor);
		++i;
	}
}

void ANewtonClipsDirectory::DestroyModel()
{
	SetActorRelativeScale3D(FVector::OneVector * 100);

	for (const auto Actor : ShapeActors)
	{
		Actor->Destroy();
	}
	ShapeActors.Empty();

	for (const auto Actor : SoftActors)
	{
		Actor->Destroy();
	}
	SoftActors.Empty();
}

void ANewtonClipsDirectory::PopulateFrameLast()
{
	PopulateFrame(Frames.Num() - 1);
}

void ANewtonClipsDirectory::PopulateFrame(const int32 FrameId)
{
	if (!Frames.IsValidIndex(FrameId))
	{
		UE_LOG(LogNewtonClips, Error, TEXT("Invalid frame %d [0, %d)"), FrameId, Frames.Num());
		return;
	}

	// ReSharper disable once CppUseStructuredBinding
	const auto& Item = Frames[FrameId];

	FString File;
	FString CacheDir = FPaths::Combine(Directory, TEXT(".cache"));

	TArray<TArray<float>> BodyTransform;
	TArray<FVector3f> ParticlePosition;

	if (TArray<uint8> Bytes;
		!Item.BodyTransform.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, Item.BodyTransform)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		constexpr int32 Count = sizeof(float) * 7;
		for (int32 i = 0; i < Bytes.Num() / Count; ++i)
		{
			TArray<float> Transform;
			Transform.SetNumUninitialized(7);
			FMemory::Memcpy(Transform.GetData(), Bytes.GetData() + i * Count, Count);
			BodyTransform.Add(Transform);
		}
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid BodyTransform cache: %s"), *Item.BodyTransform);

	if (TArray<uint8> Bytes;
		!Item.ParticlePosition.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, Item.ParticlePosition)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		ParticlePosition.SetNumUninitialized(Bytes.Num() / sizeof(float) / 3);
		FMemory::Memcpy(ParticlePosition.GetData(), Bytes.GetData(), Bytes.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid ParticlePosition cache: %s"), *Item.ParticlePosition);

	for (const auto& Actor : ShapeActors)
	{
		if (BodyTransform.IsValidIndex(Actor->Body))
		{
			const auto& Transform = BodyTransform[Actor->Body];

			// convert from right-hand-system
			const FVector L(Transform[0], Transform[1], Transform[2]);
			const FQuat Q(Transform[3], Transform[4], Transform[5], Transform[6]);
			Actor->TargetLocation = L;
			Actor->TargetRotation = Q;
			Actor->LerpTime = Item.DeltaTime;
		}
	}

	for (const auto& Actor : SoftActors)
	{
		if (Actor->Count > 0 && Actor->Begin >= 0 && Actor->Begin + Actor->Count <= ParticlePosition.Num())
		{
			auto View = TArrayView<FVector3f>(ParticlePosition).Slice(Actor->Begin, Actor->Count);
			Actor->TargetLocation = TArray<FVector3f>(View);
			Actor->LerpTime = Item.DeltaTime;
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid particles %s begin %d count %d num %d"),
			*Actor->Name, Actor->Begin, Actor->Count, ParticlePosition.Num());
	}
}
