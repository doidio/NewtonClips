// Fill out your copyright notice in the Description page of Project Settings.


#include "NewtonClipsDirectory.h"

#include "JsonObjectConverter.h"
#include "NewtonClips.h"
#include "Components/BaseDynamicMeshSceneProxy.h"
#include "Curve/DynamicGraph.h"
#include "DynamicMesh/MeshNormals.h"
#include "Operations/RepairOrientation.h"
#include "Selection/MeshTopologySelectionMechanic.h"
#include "NiagaraSystem.h"

using FM = ConstructorHelpers::FObjectFinder<UMaterial>;
using FN = ConstructorHelpers::FObjectFinder<UNiagaraSystem>;

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

	static FN NiaSphere(TEXT("NiagaraSystem'/NewtonClips/NiaSphere.NiaSphere'"));
	check(NiaSphere.Object);
	NSphere = NiaSphere.Object;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);
}

// Called every frame
void ANewtonClipsDirectory::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// check new model
	for (int _ = 0; _ < 1; ++_)
	{
		FString File = FPaths::Combine(Directory, TEXT("model.json"));
		if (!FPaths::FileExists(File))
		{
			Model = FNewtonModel();
			DestroyModel();
			Frames.Empty();
			FrameNum = Frames.Num();
			FrameId = -1;
			break;
		}

		const FDateTime TimeStamp = PlatformFile.GetTimeStamp(*File);
		if (TimeStamp <= Model.LastModified) break;

		FString JsonString;
		if (!FFileHelper::LoadFileToString(JsonString, *File))
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Failed to load JSON file: %s"), *File);
			break;
		}

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		TSharedPtr<FJsonObject> JsonObject;
		if (!FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Failed to parse JSON file: %s"), *File);
			break;
		}

		FNewtonModel NewtonModel;
		if (!FJsonObjectConverter::JsonObjectToUStruct(
			JsonObject.ToSharedRef(), FNewtonModel::StaticStruct(), &NewtonModel))
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Failed to convert JSON file: %s"), *File);
			break;
		}

		Model = NewtonModel;
		Model.LastModified = TimeStamp;
		DestroyModel();
		SpawnModel();
		Frames.Empty();
		FrameNum = Frames.Num();
		FrameId = -1;
	}

	// check the next frame
	if (!Model.Uuid.IsEmpty())
	{
		for (int _ = 0; _ < 100; ++_)
		{
			FString Filename = FString::Printf(TEXT("%d.json"), Frames.Num());
			FString File = FPaths::Combine(Directory, TEXT("frames"), Filename);
			if (!FPaths::FileExists(File)) continue;

			FString JsonString;
			if (!FFileHelper::LoadFileToString(JsonString, *File))
			{
				UE_LOG(LogNewtonClips, Error, TEXT("Failed to load JSON file: %s"), *File);
				break;
			}

			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
			TSharedPtr<FJsonObject> JsonObject;
			if (!FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				UE_LOG(LogNewtonClips, Error, TEXT("Failed to parse JSON file: %s"), *File);
				break;
			}

			FNewtonState NewtonState;
			if (!FJsonObjectConverter::JsonObjectToUStruct(
				JsonObject.ToSharedRef(), FNewtonState::StaticStruct(), &NewtonState))
			{
				UE_LOG(LogNewtonClips, Error, TEXT("Failed to convert JSON file: %s"), *File);
				break;
			}

			Frames.Add(NewtonState);
			FrameNum = Frames.Num();
		}
	}

	if (AutoPlay)
	{
		if (LerpTime == 0)
		{
			SetFrameNext();
		}
		LerpTime = FMath::Max(LerpTime - DeltaTime, 0);
	}
	else LerpTime = 0;
}

// ReSharper disable once CppUE4BlueprintCallableFunctionMayBeConst
void ANewtonClipsDirectory::SaveAsStatic()
{
	GConfig->SetString(ConfigSection, TEXT("Directory"), *Directory, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void ANewtonClipsDirectory::RestoreFromStatic()
{
	GConfig->GetString(ConfigSection, TEXT("Directory"), Directory, GGameUserSettingsIni);
}

// Called when the game starts or when spawned
void ANewtonClipsDirectory::BeginPlay()
{
	Super::BeginPlay();
}

void ANewtonClipsDirectory::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestroyModel();
}

template <typename T>
TArray<T> ANewtonClipsDirectory::Cache(FString Sha1)
{
	TArray<T> Array;
	if (Sha1.IsEmpty()) return Array;

	FString File;
	FString CacheDir = FPaths::Combine(Directory, TEXT(".cache"));

	if (TArray<uint8> Bytes;
		!Sha1.IsEmpty() &&
		FPaths::FileExists(File = FPaths::Combine(CacheDir, Sha1)) &&
		FFileHelper::LoadFileToArray(Bytes, *File))
	{
		Array.SetNumUninitialized(Bytes.Num() / sizeof(T));
		FMemory::Memcpy(Array.GetData(), Bytes.GetData(), Bytes.Num());
	}
	else UE_LOG(LogNewtonClips, Error, TEXT("Invalid cache: %s"), *Sha1);

	return Array;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
FDynamicMesh3 ANewtonClipsDirectory::CreateDynamicMesh(const TArray<FVector3f>& Vertices,
                                                       const TArray<FIntVector>& Triangles) const
{
	FDynamicMesh3 Mesh;
	Mesh.EnableTriangleGroups();
	Mesh.EnableAttributes();
	Mesh.Attributes()->EnablePrimaryColors();
	FDynamicMeshColorOverlay* ColorOverlay = Mesh.Attributes()->PrimaryColors();

	const int N = Vertices.Num();
	for (int i = 0; i < N; ++i)
	{
		Mesh.AppendVertex(FVector(Vertices[i]));
		ColorOverlay->AppendElement(FVector4f::One());
	}

	for (const auto& Triangle : Triangles)
	{
		FIntVector Tri = {Triangle[2], Triangle[1], Triangle[0]};
		const int32 Tid = Mesh.AppendTriangle(Tri, 0);

		if (Tid == FDynamicMesh3::InvalidID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Invalid Triangle: %s (%d)"), *Triangle.ToString(), N);
			continue;
		}
		if (Tid == FDynamicMesh3::NonManifoldID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("NonManifold Triangle: %s (%d)"), *Triangle.ToString(), N);
			continue;
		}
		if (Tid == FDynamicMesh3::DuplicateTriangleID)
		{
			UE_LOG(LogNewtonClips, Error, TEXT("Duplicate Triangle: %s (%d)"), *Triangle.ToString(), N);
			continue;
		}

		ColorOverlay->SetTriangle(Tid, Tri);
	}

	UE::Geometry::FMeshNormals::QuickComputeVertexNormals(Mesh);
	UE::Geometry::FMeshNormals::QuickRecomputeOverlayNormals(Mesh);

	return Mesh;
}

void ANewtonClipsDirectory::SpawnModel()
{
	SetActorRelativeScale3D(FVector(Model.Scale * 100)); // m to cm
	if (Model.UpAxis == 0)
	{
		SetActorRelativeRotation(FQuat::FindBetweenNormals({1, 0, 0}, {0, 0, 1}));
	}
	else if (Model.UpAxis == 1)
	{
		SetActorRelativeRotation(FQuat::FindBetweenNormals({0, 1, 0}, {0, 0, 1}));
	}

	// ReSharper disable once CppUseStructuredBinding
	for (const auto& Item : Model.ShapeMesh)
	{
		UE_LOG(LogNewtonClips, Log, TEXT("[ ShapeMesh: %s ]"), *Item.Name);

		auto Actor = GetWorld()->SpawnActor<ANewtonShapeMeshActor>();

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonShapeMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		const TArray<FVector3f> Vertices = Cache<FVector3f>(Item.Vertices);
		const TArray<FIntVector> Triangles = Cache<FIntVector>(Item.Indices);
		const TArray<float> VertexHues = Cache<float>(Item.VertexHues);

		Actor->GetDynamicMeshComponent()->SetMesh(CreateDynamicMesh(Vertices, Triangles));
		Actor->GetDynamicMeshComponent()->GetDynamicMesh()->EditMesh([&](FDynamicMesh3& EditMesh)
		{
			UE::Geometry::FMeshRepairOrientation Repair(&EditMesh);
			Repair.OrientComponents();
			FDynamicMeshAABBTree3 Tree(&EditMesh, true);
			Repair.SolveGlobalOrientation(&Tree);
		}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		const FVector P(Item.Transform[0], Item.Transform[1], Item.Transform[2]);
		const FQuat Q(Item.Transform[3], Item.Transform[4], Item.Transform[5], Item.Transform[6]);
		Actor->GetDynamicMeshComponent()->SetRelativeTransform(FTransform(Q, P));

		Actor->Name = Item.Name;
		Actor->Body = Item.Body;
		Actor->LerpVertexHues = VertexHues;
		Actor->Lerp();

		ShapeMeshActors.Add(Actor);
	}

	// ReSharper disable once CppUseStructuredBinding
	for (const auto& Item : Model.SoftMesh)
	{
		UE_LOG(LogNewtonClips, Log, TEXT("[ SoftMesh: %s ]"), *Item.Name);

		auto Actor = GetWorld()->SpawnActor<ANewtonSoftMeshActor>();

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonSoftMesh") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		const TArray<FVector3f> Vertices = Cache<FVector3f>(Item.Vertices);
		const TArray<FIntVector> Triangles = Cache<FIntVector>(Item.Indices);
		const TArray<float> VertexHues = Cache<float>(Item.VertexHues);

		Actor->GetDynamicMeshComponent()->SetMesh(CreateDynamicMesh(Vertices, Triangles));
		Actor->GetDynamicMeshComponent()->GetDynamicMesh()->EditMesh([&](FDynamicMesh3& EditMesh)
		{
			UE::Geometry::FMeshRepairOrientation Repair(&EditMesh);
			Repair.OrientComponents();
			FDynamicMeshAABBTree3 Tree(&EditMesh, true);
			Repair.SolveGlobalOrientation(&Tree);
		}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);

		Actor->GetDynamicMeshComponent()->SetMaterial(0, UMaterialInstanceDynamic::Create(MOpaque, this));
		Actor->GetDynamicMeshComponent()->MarkRenderStateDirty();

		Actor->Name = Item.Name;
		Actor->Begin = Item.Begin;
		Actor->Count = Item.Count;
		Actor->LerpVertexHues = VertexHues;
		Actor->Lerp();

		SoftMeshActors.Add(Actor);
	}

	// ReSharper disable once CppUseStructuredBinding
	for (const auto& Item : Model.GranularFluid)
	{
		UE_LOG(LogNewtonClips, Log, TEXT("[ GranularFluid: %s ]"), *Item.Name);

		auto Actor = GetWorld()->SpawnActor<ANewtonGranularFluidActor>();

#if WITH_EDITOR
		Actor->SetActorLabel(Item.Name.IsEmpty() ? TEXT("NewtonGranularFluid") : Item.Name);
#endif
		Actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

		UNiagaraComponent* Nia = Cast<UNiagaraComponent>(Actor->GetRootComponent());

		const TArray<FVector3f> ParticlePositions = Cache<FVector3f>(Item.ParticlePositions);
		const TArray<float> ParticleHues = Cache<float>(Item.ParticleHues);

		Nia->SetAsset(NSphere);
		Nia->SetVariableMaterial(FName(TEXT("Mat")), UMaterialInstanceDynamic::Create(MOpaque, this));

		Actor->Name = Item.Name;
		Actor->Begin = Item.Begin;
		Actor->Count = Item.Count;
		Actor->LerpParticlePositions = ParticlePositions;
		Actor->LerpParticleHues = ParticleHues;
		Actor->Lerp();

		GranularFluidActors.Add(Actor);
	}
}

void ANewtonClipsDirectory::DestroyModel()
{
	SetActorRelativeScale3D(FVector::OneVector * 100);
	SetActorRelativeRotation(FQuat::Identity);

	for (const auto Actor : ShapeMeshActors)
	{
		Actor->Destroy();
	}
	ShapeMeshActors.Empty();

	for (const auto Actor : SoftMeshActors)
	{
		Actor->Destroy();
	}
	SoftMeshActors.Empty();

	for (const auto Actor : GranularFluidActors)
	{
		Actor->Destroy();
	}
	GranularFluidActors.Empty();
}

void ANewtonClipsDirectory::SetFrameNext()
{
	int32 Id = FrameId + 1;
	if (AutoLoop && Frames.Num() > 0)
	{
		Id %= Frames.Num();
	}
	if (Frames.IsValidIndex(Id))
	{
		SetFrame(Id);
	}
}

void ANewtonClipsDirectory::SetFrame(const int32 InFrameId)
{
	if (InFrameId < 0)
	{
		DestroyModel();
		SpawnModel();
		FrameId = -1;
		return;
	}

	if (!Frames.IsValidIndex(InFrameId))
	{
		UE_LOG(LogNewtonClips, Error, TEXT("Invalid frame %d [1, %d]"), InFrameId, Frames.Num());
		return;
	}

	// ReSharper disable once CppUseStructuredBinding
	const auto& State = Frames[InFrameId];

	FString File;
	FString CacheDir = FPaths::Combine(Directory, TEXT(".cache"));

	TArray<float> BodyTransformsRaw = Cache<float>(State.BodyTransforms);
	TArray<TArray<float>> BodyTransforms;
	for (int i = 0; i < BodyTransformsRaw.Num(); i += 7)
	{
		BodyTransforms.Add({BodyTransformsRaw.GetData() + i, 7});
	}

	TArray<FVector3f> ParticlePositions = Cache<FVector3f>(State.ParticlePositions);

	TMap<int32, TArray<float>> ShapeVertexHues;
	for (const auto& Pair : State.ShapeVertexHues)
	{
		ShapeVertexHues.Add(Pair.Key, Cache<float>(Pair.Value));
	}

	TMap<int32, TArray<float>> ParticleHues;
	for (const auto& Pair : State.ParticleHues)
	{
		ParticleHues.Add(Pair.Key, Cache<float>(Pair.Value));
	}

	FrameId = InFrameId;

	for (int i = 0; i < ShapeMeshActors.Num(); ++i)
	{
		ANewtonShapeMeshActor* Actor = ShapeMeshActors[i];
		if (BodyTransforms.IsValidIndex(Actor->Body))
		{
			const auto& Transform = BodyTransforms[Actor->Body];
			Actor->LerpTransform = Transform;
			Actor->LerpTime = State.DeltaTime;
		}

		if (ShapeVertexHues.Contains(i))
		{
			Actor->LerpVertexHues = ShapeVertexHues[i];
		}
	}

	for (int i = 0; i < SoftMeshActors.Num(); ++i)
	{
		ANewtonSoftMeshActor* Actor = SoftMeshActors[i];
		if (Actor->Begin >= 0 && Actor->Count > 0 && Actor->Begin + Actor->Count <= ParticlePositions.Num())
		{
			auto View = TArrayView<const FVector3f>(ParticlePositions).Slice(Actor->Begin, Actor->Count);
			Actor->LerpParticlePositions = TArray<FVector3f>(View);
			Actor->LerpTime = State.DeltaTime;
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid %s begin %d count %d num %d frame %d"),
		            *Actor->Name, Actor->Begin, Actor->Count, ParticlePositions.Num(), FrameId);

		if (ParticleHues.Contains(Actor->Begin))
		{
			Actor->LerpVertexHues = ParticleHues[Actor->Begin];
		}
	}

	for (int i = 0; i < GranularFluidActors.Num(); ++i)
	{
		ANewtonGranularFluidActor* Actor = GranularFluidActors[i];
		if (Actor->Begin >= 0 && Actor->Count > 0 && Actor->Begin + Actor->Count <= ParticlePositions.Num())
		{
			auto View = TArrayView<const FVector3f>(ParticlePositions).Slice(Actor->Begin, Actor->Count);
			Actor->LerpParticlePositions = TArray<FVector3f>(View);
			Actor->LerpTime = State.DeltaTime;
		}
		else UE_LOG(LogNewtonClips, Error, TEXT("Invalid %s begin %d count %d num %d frame %d"),
		            *Actor->Name, Actor->Begin, Actor->Count, ParticlePositions.Num(), FrameId);

		if (ParticleHues.Contains(Actor->Begin))
		{
			Actor->LerpParticleHues = ParticleHues[Actor->Begin];
		}
	}

	LerpTime = State.DeltaTime;

	// FString Path = FPaths::Combine(Directory, TEXT("screenshots"), FString::Printf(TEXT("FrameId_%d"), FrameId));
	// FScreenshotRequest::RequestScreenshot(Path, false, false, false);
}
