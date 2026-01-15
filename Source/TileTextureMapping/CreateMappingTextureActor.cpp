// Fill out your copyright notice in the Description page of Project Settings.


#include "CreateMappingTextureActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ImageUtils.h"

// Sets default values
ACreateMappingTextureActor::ACreateMappingTextureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACreateMappingTextureActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACreateMappingTextureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


void ACreateMappingTextureActor::CIE_CreateTexture()
{
	FString saveLoc = (SaveDir + "/" + SaveName);
	UPackage* TexturePackage = CreatePackage(*saveLoc);
	TexturePackage->FullyLoad();

	const EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8;
	const int32 TileCountsPerAxis = (int32)(TileAtlasTextureResolution / PerTileResolution);

	UTexture2D* MappingTexture = NewObject<UTexture2D>(TexturePackage, UTexture2D::StaticClass(),*SaveName, RF_Public | RF_Standalone);
	MappingTexture->MipGenSettings = TMGS_NoMipmaps;
	MappingTexture->CompressionSettings = TC_Default;
	MappingTexture->SRGB = false;  
	MappingTexture->SetPlatformData(new FTexturePlatformData());
	FTexturePlatformData* Data = MappingTexture->GetPlatformData();
	Data->SizeX = IndexTextureResolution;
	Data->SizeY = IndexTextureResolution;
	Data->SetNumSlices(1);
	Data->PixelFormat = PixelFormat;

	int32 NumBlocksX = IndexTextureResolution / GPixelFormats[PixelFormat].BlockSizeX;
	int32 NumBlocksY = IndexTextureResolution / GPixelFormats[PixelFormat].BlockSizeY;
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	Mip->SizeX = IndexTextureResolution;
	Mip->SizeY = IndexTextureResolution;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	Mip->BulkData.Realloc((int64)NumBlocksX * NumBlocksY * GPixelFormats[PixelFormat].BlockBytes);
	Mip->BulkData.Unlock();

	MappingTexture->GetPlatformData()->Mips.Add(Mip);

	
	FByteBulkData& BulkData = MappingTexture->GetPlatformData()->Mips[0].BulkData;
	uint8* ArrayData = static_cast<uint8*>(BulkData.Lock(LOCK_READ_WRITE));
	const int32 Max = TileCountsPerAxis - 1;
	for (int32 i = 0; i < IndexTextureResolution * IndexTextureResolution * 4; i += 4)
	{
		const int32 rand1 = FMath::RandRange(0, Max);
		const int32 rand2 = FMath::RandRange(0, Max);

		const float Idx1 = (float)rand1 / (float)Max;
		const float Idx2 = (float)rand2 / (float)Max;

		const uint8 RChannel = (uint8)(Idx1 * 255);
		const uint8 GChannel = (uint8)(Idx2 * 255);
		ArrayData[i] = 1; //B
		ArrayData[i + 1] = GChannel; //G
		ArrayData[i + 2] = RChannel; //R
		ArrayData[i + 3] = 1; //A
	}
	MappingTexture->Source.Init(IndexTextureResolution, IndexTextureResolution, 1, 1, ETextureSourceFormat::TSF_BGRA8, ArrayData);
	BulkData.Unlock();
	MappingTexture->UpdateResource();

	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MappingTexture);


	FString PackageFilePath = FPackageName::LongPackageNameToFilename(SaveDir,FPackageName::GetAssetPackageExtension());

	if (!UPackage::SavePackage(
			TexturePackage,
			MappingTexture,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
			*PackageFilePath
		))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Asset: [%s]\n"), *PackageFilePath);
	}
	TArray<UObject*> ObjectsToSync;
	ObjectsToSync.Add(MappingTexture);
	GEditor->SyncBrowserToObjects(ObjectsToSync);
}