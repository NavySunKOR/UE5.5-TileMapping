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

	UTexture2D* MappingTexture = NewObject<UTexture2D>(TexturePackage, UTexture2D::StaticClass(),*SaveName, RF_Public | RF_Standalone);
	MappingTexture->MipGenSettings = TMGS_NoMipmaps;
	MappingTexture->CompressionSettings = TC_Default;
	MappingTexture->SRGB = false;  
	MappingTexture->SetPlatformData(new FTexturePlatformData());
	FTexturePlatformData* Data = MappingTexture->GetPlatformData();
	Data->SizeX = MappingTileCounts;
	Data->SizeY = MappingTileCounts;
	Data->SetNumSlices(1);
	Data->PixelFormat = PF_R8G8;

	int32 NumBlocksX = MappingTileCounts / GPixelFormats[PF_R8G8].BlockSizeX;
	int32 NumBlocksY = MappingTileCounts / GPixelFormats[PF_R8G8].BlockSizeY;
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	Mip->SizeX = MappingTileCounts;
	Mip->SizeY = MappingTileCounts;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	Mip->BulkData.Realloc((int64)NumBlocksX * NumBlocksY * GPixelFormats[PF_R8G8].BlockBytes);
	Mip->BulkData.Unlock();

	MappingTexture->GetPlatformData()->Mips.Add(Mip);
	
	FByteBulkData& BulkData = MappingTexture->GetPlatformData()->Mips[0].BulkData;
	uint8* ArrayData = static_cast<uint8*>(BulkData.Lock(LOCK_READ_WRITE));
	const int32 Max = MappingTileCounts - 1;
	for (int32 i = 0; i < MappingTileCounts * MappingTileCounts * 2; i += 2)
	{
		const float Idx1 = (float)FMath::RandRange(0, Max) / (float)Max;
		const float Idx2 = (float)FMath::RandRange(0, Max) / (float)Max;

		const uint8 RChannel = (uint8)(Idx1 * 255);
		const uint8 GChannel = (uint8)(Idx2 * 255);
		UE_LOG(LogTemp,Warning,TEXT("Pixel : %d / %d"), RChannel,GChannel)
		ArrayData[i] = RChannel;
		ArrayData[i + 1] = GChannel;
	}
	BulkData.Unlock();


	MappingTexture->UpdateResource();

	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MappingTexture);


	FString PackageFilePath = FPackageName::LongPackageNameToFilename(SaveDir,FPackageName::GetAssetPackageExtension());

	UPackage::SavePackage(
		TexturePackage,
		MappingTexture,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
		*PackageFilePath
	);

	TArray<UObject*> ObjectsToSync;
	ObjectsToSync.Add(MappingTexture);
	GEditor->SyncBrowserToObjects(ObjectsToSync);
}