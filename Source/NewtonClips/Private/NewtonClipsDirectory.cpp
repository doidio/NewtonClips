// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonClipsDirectory.h"

#include "JsonObjectConverter.h"
#include "NewtonClips.h"
#include "Components/BaseDynamicMeshSceneProxy.h"
#include "Curve/DynamicGraph.h"

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
	DestroyActors();
}

void ANewtonClipsDirectory::OnTimer()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FDateTime TimeStamp;

	if (const auto ModelFile = FPaths::Combine(Directory, TEXT("model.json"));
		(TimeStamp = PlatformFile.GetTimeStamp(*ModelFile)) > ModelFileTimeStamp)
	{
		ModelFileTimeStamp = TimeStamp;

		if (FString JsonString; FFileHelper::LoadFileToString(JsonString, *ModelFile))
		{
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

			if (TSharedPtr<FJsonObject> JsonObject; FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				if (FNewtonModel NewtonModel; FJsonObjectConverter::JsonObjectToUStruct(
					JsonObject.ToSharedRef(), FNewtonModel::StaticStruct(), &NewtonModel))
				{
					if (Uuid != NewtonModel.Uuid)
					{
						DestroyActors();
						SpawnActors(NewtonModel);
					}
				}
				else UE_LOG(LogNewtonClips, Error, TEXT("Failed to convert JSON file: %s"), *ModelFile);
			}
			else UE_LOG(LogNewtonClips, Error, TEXT("Failed to parse JSON file: %s"), *ModelFile);
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Failed to load JSON file: %s"), *ModelFile);
	}

	// TODO: check state
}

FDynamicMesh3 ANewtonClipsDirectory::CreateDynamicMesh(const FString& Vertices,
                                                       const FString& Indices,
                                                       const FString& VertexNormals,
                                                       const FString& VertexUVs) const
{
	TArray<FVector3f> OutVertices;
	TArray<FIntVector> OutTriangles;
	TArray<FVector3f> OutVertexNormals;
	TArray<FVector2f> OutVertexUVs;

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

	if (TArray<uint8> Bytes;
		!VertexNormals.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, VertexNormals)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		if (const int32 Num = Bytes.Num() / sizeof(FVector3f); Num == OutVertices.Num())
		{
			OutVertexNormals.SetNumUninitialized(Num);
			FMemory::Memcpy(OutVertexNormals.GetData(), Bytes.GetData(), Bytes.Num());
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexNormals num: %d != %d"), Num, OutVertices.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexNormals cache: %s"), *VertexNormals);

	if (!VertexUVs.IsEmpty())
	{
		if (TArray<uint8> Bytes;
			FPaths::FileExists(File = FPaths::Combine(CacheDir, VertexUVs)) &&
			FFileHelper::LoadFileToArray(Bytes, *File))
		{
			if (const int32 Num = Bytes.Num() / sizeof(FVector2f); Num == OutVertices.Num())
			{
				OutVertexUVs.SetNumUninitialized(Num);
				FMemory::Memcpy(OutVertexUVs.GetData(), Bytes.GetData(), Bytes.Num());
			}
			else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexUVs num: %d != %d"), Num, OutVertices.Num());
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexUVs cache: %s"), *VertexUVs);
	}

	return CreateDynamicMesh(OutVertices, OutTriangles, OutVertexNormals, OutVertexUVs);
}

FDynamicMesh3 ANewtonClipsDirectory::CreateDynamicMesh(const TArray<FVector3f>& Vertices,
                                                       const TArray<FIntVector>& Triangles,
                                                       const TArray<FVector3f>& VertexNormals,
                                                       const TArray<FVector2f>& VertexUVs) const
{
	FDynamicMesh3 DynamicMesh;

	DynamicMesh.EnableTriangleGroups();

	const int NumVerts = Vertices.Num();
	for (int i = 0; i < NumVerts; ++i)
	{
		DynamicMesh.AppendVertex(FVector3d(Vertices[i]));
	}

	for (const auto& Triangle : Triangles)
	{
		FIntVector Tri = {Triangle[2], Triangle[1], Triangle[0]};
		const int32 Tid = DynamicMesh.AppendTriangle(Tri, 0);

		if (Tid == FDynamicMesh3::InvalidID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Invalid Triangle: %s"), *Triangle.ToString());
			return {};
		}
		if (Tid == FDynamicMesh3::NonManifoldID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("NonManifold Triangle: %s"), *Triangle.ToString());
			return {};
		}
		if (Tid == FDynamicMesh3::DuplicateTriangleID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Duplicate Triangle: %s"), *Triangle.ToString());
			return {};
		}
	}

	if (VertexUVs.Num() > 0 || VertexNormals.Num() > 0)
	{
		DynamicMesh.EnableAttributes();

		const auto UVOverlay = DynamicMesh.Attributes()->PrimaryUV();
		const auto NormalOverlay = DynamicMesh.Attributes()->PrimaryNormals();

		for (int i = 0; i < VertexUVs.Num(); ++i)
		{
			UVOverlay->AppendElement(VertexUVs[i]);
		}

		for (int i = 0; i < VertexNormals.Num(); ++i)
		{
			NormalOverlay->AppendElement(VertexNormals[i]);
		}

		for (int i = 0; i < Triangles.Num(); ++i)
		{
			FIntVector Tri = {Triangles[i][2], Triangles[i][1], Triangles[i][0]};
			UVOverlay->SetTriangle(i, Tri);
			NormalOverlay->SetTriangle(i, Tri);
		}
	}

	DynamicMesh.EnableVertexColors(DefaultVertexColor);

	return DynamicMesh;
}

void ANewtonClipsDirectory::SpawnActors(const FNewtonModel& NewtonModel)
{
	int32 i = 0;
	for (const auto& Item : NewtonModel.ShapeMesh)
	{
		FDynamicMesh3 DynamicMesh = CreateDynamicMesh(Item.Vertices, Item.Indices, Item.VertexNormals, Item.VertexUVs);

		UE_LOG(LogNewtonClips, Log, TEXT("[ ShapeMesh: %d ]"), i);
		UE_LOG(LogNewtonClips, Log, TEXT("%s"), *DynamicMesh.MeshInfoString());

		auto Actor = GetWorld()->SpawnActor<ANewtonShapeMeshActor>();
		ShapeActors.Add(Actor);
		Actor->GetDynamicMeshComponent()->SetMesh(MoveTemp(DynamicMesh));

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonShapeMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		// convert from right-hand-system
		const FVector L(Item.Transform[0], Item.Transform[1], Item.Transform[2]);
		const FQuat Q(Item.Transform[3], -Item.Transform[4], -Item.Transform[5], Item.Transform[6]);
		const FVector S(Item.Scale[0], Item.Scale[1], Item.Scale[2]);
		Actor->SetActorTransform(FTransform(Q, L, S));

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		++i;
	}

	i = 0;

	for (const auto& Item : NewtonModel.SoftMesh)
	{
		FDynamicMesh3 DynamicMesh = CreateDynamicMesh(Item.Vertices, Item.Indices, Item.VertexNormals, Item.VertexUVs);

		UE_LOG(LogNewtonClips, Log, TEXT("[ SoftMesh: %d ]"), i);
		UE_LOG(LogNewtonClips, Log, TEXT("%s"), *DynamicMesh.MeshInfoString());

		auto Actor = GetWorld()->SpawnActor<ANewtonSoftMeshActor>();
		SoftActors.Add(Actor);
		Actor->GetDynamicMeshComponent()->SetMesh(MoveTemp(DynamicMesh));

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonSoftMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		++i;
	}
}

void ANewtonClipsDirectory::DestroyActors()
{
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
