// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonClipsDirectory.h"

#include "JsonObjectConverter.h"
#include "NewtonClips.h"

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
	DestroyActors();
}

FNewtonMeshGenerator* FNewtonMeshGenerator::Copy(const FString& CacheDir,
                                                 const FString& InVertices, const FString& InIndices,
                                                 const FString& InVertexNormals, const FString& InVertexUVs)
{
	TArray<FVector3f> OutVertices;
	TArray<FIntVector> OutTriangles;
	TArray<FVector3f> OutVertexNormals;
	TArray<FVector2f> OutVertexUVs;

	FString File;

	if (TArray<uint8> Bytes;
		!InVertices.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, InVertices)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		OutVertices.SetNumUninitialized(Bytes.Num() / sizeof(FVector3f));
		FMemory::Memcpy(OutVertices.GetData(), Bytes.GetData(), Bytes.Num());
		// UE_LOG(LogNewtonClips, Warning, TEXT("Vertices: %d"), OutVertices.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid Vertices cache: %s"), *InVertices);

	if (TArray<uint8> Bytes;
		!InIndices.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, InIndices)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		OutTriangles.SetNumUninitialized(Bytes.Num() / sizeof(FIntVector));
		FMemory::Memcpy(OutTriangles.GetData(), Bytes.GetData(), Bytes.Num());
		// UE_LOG(LogNewtonClips, Warning, TEXT("Triangles: %d"), OutTriangles.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid Indices cache: %s"), *InIndices);

	if (TArray<uint8> Bytes;
		!InVertexNormals.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, InVertexNormals)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		if (const int32 Num = Bytes.Num() / sizeof(FVector3f); Num == OutVertices.Num())
		{
			OutVertexNormals.SetNumUninitialized(Num);
			FMemory::Memcpy(OutVertexNormals.GetData(), Bytes.GetData(), Bytes.Num());
			// UE_LOG(LogNewtonClips, Warning, TEXT("VertexNormals: %d"), OutVertexNormals.Num());
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexNormals num: %d != %d"), Num, OutVertices.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexNormals cache: %s"), *InVertexNormals);

	if (!InVertexUVs.IsEmpty())
	{
		if (TArray<uint8> Bytes;
			FPaths::FileExists(File = FPaths::Combine(CacheDir, InVertexUVs)) &&
			FFileHelper::LoadFileToArray(Bytes, *File))
		{
			if (const int32 Num = Bytes.Num() / sizeof(FVector2f); Num == OutVertices.Num())
			{
				OutVertexUVs.SetNumUninitialized(Num);
				FMemory::Memcpy(OutVertexUVs.GetData(), Bytes.GetData(), Bytes.Num());
				// UE_LOG(LogNewtonClips, Warning, TEXT("VertexUVs: %d"), OutVertexUVs.Num());
			}
			else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexUVs num: %d != %d"), Num, OutVertices.Num());
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid VertexUVs cache: %s"), *InVertexUVs);
	}

	return Copy(OutVertices, OutTriangles, OutVertexNormals, OutVertexUVs);
}

FNewtonMeshGenerator* FNewtonMeshGenerator::Copy(const TArray<FVector3f>& InVertices,
                                                 const TArray<FIntVector>& InTriangles,
                                                 const TArray<FVector3f>& InVertexNormals,
                                                 const TArray<FVector2f>& InVertexUVs)
{
	SetBufferSizes(InVertices.Num(), InTriangles.Num(),
	               InVertices.Num(), InVertices.Num());

	bReverseOrientation = true;

	for (int32 i = 0; i < InVertices.Num(); ++i)
	{
		if (i < InVertexNormals.Num())
		{
			Normals[i] = FVector3f(InVertexNormals[i]);
		}
		NormalParentVertex[i] = i;
		if (i < InVertexUVs.Num())
		{
			UVs[i] = InVertexUVs[i];
		}
		else
		{
			UVs[i] = FVector2f::Zero();
		}
		UVParentVertex[i] = i;
		Vertices[i] = FVector3d(InVertices[i]);
	}

	for (int32 i = 0; i < InTriangles.Num(); ++i)
	{
		SetTriangle(i, InTriangles[i]);
		SetTrianglePolygon(i, 0);
		SetTriangleUVs(i, InTriangles[i]);
		SetTriangleNormals(i, InTriangles[i]);
	}

	return this;
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

void ANewtonClipsDirectory::SpawnActors(const FNewtonModel& NewtonModel)
{
	int32 i = 0;
	for (const auto& Item : NewtonModel.ShapeMesh)
	{
		FDynamicMesh3 DynamicMesh;
		if (FNewtonMeshGenerator Generator; DynamicMesh.Copy(Generator.Copy(
			FPaths::Combine(Directory, TEXT(".cache")),
			Item.Vertices, Item.Indices, Item.VertexNormals, Item.VertexUVs)))
		{
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
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid ShapeMesh: %d"), i);

		++i;
	}

	i = 0;

	for (const auto& Item : NewtonModel.SoftMesh)
	{
		FDynamicMesh3 DynamicMesh;
		if (FNewtonMeshGenerator Generator; DynamicMesh.Copy(Generator.Copy(
			FPaths::Combine(Directory, TEXT(".cache")),
			Item.Vertices, Item.Indices, Item.VertexNormals, Item.VertexUVs)))
		{
			auto Actor = GetWorld()->SpawnActor<ANewtonSoftMeshActor>();
			SoftActors.Add(Actor);
			Actor->GetDynamicMeshComponent()->SetMesh(MoveTemp(DynamicMesh));

#if WITH_EDITOR
			Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonSoftMesh") : Item.Name);
#endif
			Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid SoftMesh: %d"), i);

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
