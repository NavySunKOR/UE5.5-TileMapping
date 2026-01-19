// Fill out your copyright notice in the Description page of Project Settings.


#include "CreateMappingTextureActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ImageUtils.h"

void ACreateMappingTextureActor::BeginPlay()
{
	Super::BeginPlay();
	IndexTexturePixelCounts = IndexTextureResolution * IndexTextureResolution * COLOR_CHANNEL;
}

#if WITH_EDITOR
void ACreateMappingTextureActor::CIE_CreateRandomTileIndexTexture()
{
	FString saveLoc = (SaveDir + "/" + SaveName);
	UPackage* TexturePackage = CreatePackage(*saveLoc);
	TexturePackage->FullyLoad();

	UTexture2D* MappingTexture = CreateTexture2D(EPixelFormat::PF_B8G8R8A8, TexturePackage);

	FByteBulkData& BulkData = MappingTexture->GetPlatformData()->Mips[0].BulkData;
	uint8* ArrayData = static_cast<uint8*>(BulkData.Lock(LOCK_READ_WRITE));

	const int32 TileCountsPerAxis = (int32)(TileAtlasTextureResolution / PerTileResolution);
	int32 Max = (bUsing1DTile) ? TileCountsPerAxis * TileCountsPerAxis - 1 : TileCountsPerAxis - 1;

	for (int32 i = 0; i < IndexTexturePixelCounts; i += COLOR_CHANNEL)
	{
		ArrayData[i] = 1; //B
		ArrayData[i + 3] = 1; //A

		if (bUsing1DTile)
		{
			const int32 rand = FMath::RandRange(0, Max);
			const float Idx = (float)rand / (float)Max;
			const uint8 RChannel = (uint8)(Idx * 255);

			ArrayData[i + 1] = 1; // G
			ArrayData[i + 2] = RChannel; //R
		}
		else
		{
			const int32 rand1 = FMath::RandRange(0, Max);
			const int32 rand2 = FMath::RandRange(0, Max);

			const float Idx1 = (float)rand1 / (float)Max;
			const float Idx2 = (float)rand2 / (float)Max;

			const uint8 RChannel = (uint8)(Idx1 * 255);
			const uint8 GChannel = (uint8)(Idx2 * 255);

			ArrayData[i + 1] = GChannel; //G
			ArrayData[i + 2] = RChannel; //R
		}
	}
	MappingTexture->Source.Init(IndexTextureResolution, IndexTextureResolution, 1, 1, ETextureSourceFormat::TSF_BGRA8, ArrayData);
	BulkData.Unlock();
	MappingTexture->UpdateResource();

	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MappingTexture);

	FString PackageFilePath = FPackageName::LongPackageNameToFilename(SaveDir, FPackageName::GetAssetPackageExtension());

	if (!UPackage::SavePackage(
		TexturePackage,
		MappingTexture,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
		*PackageFilePath
	))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Asset: [%s]\n"), *PackageFilePath);
	}

	SyncContentBrowser(MappingTexture);
}
#endif

#if WITH_EDITOR
//Wang tile fixes 16 tile in total.
void ACreateMappingTextureActor::CIE_CreateWangTileIndexTexture()
{
	if (bUseManualMapping == false)
	{
		Tiles.Empty();
		for (int32 i = 0; i <= 15; ++i)
		{
			Tiles.Add(FWangTileData(i));
		}
	}
	else
	{
		for (FWangTileData& Tile : Tiles)
		{
			Tile.CalcTileIndice();
		}
	}

	FString saveLoc = (SaveDir + "/" + SaveName);
	UPackage* TexturePackage = CreatePackage(*saveLoc);
	TexturePackage->FullyLoad();

	UTexture2D* MappingTexture = CreateTexture2D(EPixelFormat::PF_B8G8R8A8, TexturePackage);

	FByteBulkData& BulkData = MappingTexture->GetPlatformData()->Mips[0].BulkData;
	uint8* ArrayData = static_cast<uint8*>(BulkData.Lock(LOCK_READ_WRITE));

	const int32 TileCountsPerAxis = (int32)(TileAtlasTextureResolution / PerTileResolution);
	const int32 Max = (bUsing1DTile) ? TileCountsPerAxis * TileCountsPerAxis - 1 : TileCountsPerAxis - 1;

	for (int32 i = 0; i < IndexTexturePixelCounts; i += COLOR_CHANNEL)
	{
		ArrayData[i + 3] = 1; //A

		const uint8 SelectedTiles1D = GetWangTileIndex(ArrayData, i);
		ArrayData[i] = SelectedTiles1D; //B

		if (bUsing1DTile)
		{
			const uint8 Idx = SelectedTiles1D % TileCountsPerAxis;
			const float Rate = (float)Idx / TileCountsPerAxis;

			const uint8 RChannel = (uint8)(Rate * 255);
			ArrayData[i + 2] = RChannel; //R
		}
		else
		{
			const uint8 XIdx = SelectedTiles1D % TileCountsPerAxis;
			const uint8 YIdx = SelectedTiles1D / TileCountsPerAxis;

			const float RateX = (float)XIdx / TileCountsPerAxis;
			const float RateY = (float)YIdx / TileCountsPerAxis;

			const uint8 RChannel = (uint8)(RateX * 255);
			const uint8 GChannel = (uint8)(RateY * 255);
			ArrayData[i + 1] = GChannel; //G
			ArrayData[i + 2] = RChannel; //R
		}
	}
	MappingTexture->Source.Init(IndexTextureResolution, IndexTextureResolution, 1, 1, ETextureSourceFormat::TSF_BGRA8, ArrayData);
	BulkData.Unlock();
	MappingTexture->UpdateResource();

	TexturePackage->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MappingTexture);

	FString PackageFilePath = FPackageName::LongPackageNameToFilename(SaveDir, FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(
		TexturePackage,
		MappingTexture,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
		*PackageFilePath
	))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save Asset: [%s]\n"), *PackageFilePath);
	}

	SyncContentBrowser(MappingTexture);
}

void ACreateMappingTextureActor::SyncContentBrowser(UTexture2D* InObjectToSync)
{
	TArray<UObject*> ObjectsToSync;
	ObjectsToSync.Add(InObjectToSync);
	GEditor->SyncBrowserToObjects(ObjectsToSync);
}
#endif

UTexture2D* ACreateMappingTextureActor::CreateTexture2D(EPixelFormat InPixelFormat, UPackage* TexturePackage)
{
	UTexture2D* MappingTexture = NewObject<UTexture2D>(TexturePackage, UTexture2D::StaticClass(), *SaveName, RF_Public | RF_Standalone);
	MappingTexture->MipGenSettings = TMGS_NoMipmaps;
	MappingTexture->CompressionSettings = TC_Default;
	MappingTexture->SRGB = false;
	MappingTexture->SetPlatformData(new FTexturePlatformData());
	FTexturePlatformData* Data = MappingTexture->GetPlatformData();
	Data->SizeX = IndexTextureResolution;
	Data->SizeY = IndexTextureResolution;
	Data->SetNumSlices(1);
	Data->PixelFormat = InPixelFormat;

	const FPixelFormatInfo& PixelFormatInfo = GPixelFormats[InPixelFormat];
	const int32 NumBlocksX = IndexTextureResolution / PixelFormatInfo.BlockSizeX;
	const int32 NumBlocksY = IndexTextureResolution / PixelFormatInfo.BlockSizeY;

	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	Mip->SizeX = IndexTextureResolution;
	Mip->SizeY = IndexTextureResolution;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	Mip->BulkData.Realloc((int64)NumBlocksX * NumBlocksY * PixelFormatInfo.BlockBytes);
	Mip->BulkData.Unlock();

	MappingTexture->GetPlatformData()->Mips.Add(Mip);

	return MappingTexture;
}

uint8 ACreateMappingTextureActor::SearchWangTileIndex(uint8 SearchIdx)
{
	TArray<FWangTileData> OutputTile;
	for (const FWangTileData& Tile : Tiles)
	{
		//Place where
		if (SearchIdx == 0)
		{
			OutputTile.Add(Tile);
		}
		else if ((Tile.TileIndice & SearchIdx) != 0)
		{
			OutputTile.Add(Tile);
		}
	}

	const FWangTileData& Result = OutputTile[FMath::RandRange(0, OutputTile.Num() - 1)];

	return Result.TileIndice;
}

uint8 ACreateMappingTextureActor::GetWangTileIndex(uint8* InPixelArray, int32 CurrentPixelIdx)
{
	int32 X = CurrentPixelIdx % IndexTextureResolution;
	int32 Y = CurrentPixelIdx / IndexTextureResolution;

	//Corner check
	const bool bIsXCornor = (X == 0 || X == (IndexTextureResolution - 1));
	const bool bIsYCornor = (Y == 0 || Y == (IndexTextureResolution - 1));

	uint8 West = 0;
	uint8 North = 0;
	uint8 East = 0;
	uint8 South = 0;

	if (bIsXCornor || bIsYCornor)
	{
		if (bIsXCornor)
		{
			West = (X == 0) ? 0 : 1;
			East = (X == (IndexTextureResolution - 1)) ? 0 : 1;
		}

		if (bIsYCornor)
		{
			North = (Y == 0) ? 0 : 1;
			South = (Y == (IndexTextureResolution - 1)) ? 0 : 1;
		}
	}
	else //If it's not cornor, then search four neighbour pixels and matches
	{
		int32 WestIdx = CurrentPixelIdx - 4;
		West = ((InPixelArray[WestIdx] & WEST_BIT) != 0) ? 1 : 0;

		int32 NorthIdx = (CurrentPixelIdx - (Y * IndexTextureResolution));
		North = ((InPixelArray[NorthIdx] & NORTH_BIT) != 0) ? 1 : 0;

		int32 EastIdx = CurrentPixelIdx + 4;
		East = ((InPixelArray[EastIdx] & EAST_BIT) != 0) ? 1 : 0;

		int32 SouthIdx = (CurrentPixelIdx + (Y * IndexTextureResolution));
		South = ((InPixelArray[SouthIdx] & SOUTH_BIT) != 0) ? 1 : 0;
	}

	FWangTileData SearchTile = FWangTileData(West, North, East, South);
	return SearchWangTileIndex(SearchTile.TileIndice);
}