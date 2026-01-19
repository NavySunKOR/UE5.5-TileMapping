// Fill out your copyright notice in the Description page of Project Settings.


#include "CreateMappingTextureActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ImageUtils.h"

void ACreateMappingTextureActor::BeginPlay()
{
	Super::BeginPlay();
}

#if WITH_EDITOR
void ACreateMappingTextureActor::CIE_CreateRandomTileIndexTexture()
{
	const int32 IndexTexturePixelCounts = IndexTextureResolution * IndexTextureResolution * COLOR_CHANNEL;

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

	const int32 IndexTexturePixelCounts = IndexTextureResolution * IndexTextureResolution * COLOR_CHANNEL;

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

//Search Match tile 1 is essential match, 0 is optional.
uint8 ACreateMappingTextureActor::SearchWangTileIndex(const FWangTileData& InWangTileData)
{
	TArray<FWangTileData> OutputTile;

	//If 0, then it's optional.
	//If 1, then it's essential.
	for (const FWangTileData& Tile : Tiles)
	{
		if (InWangTileData.East == 1 && (Tile.East == 0))
		{
			continue;
		}

		if (InWangTileData.West == 1 && (Tile.West == 0))
		{
			continue;
		}

		if (InWangTileData.South == 1 && (Tile.South == 0))
		{
			continue;
		}

		if (InWangTileData.North == 1 && (Tile.North == 0))
		{
			continue;
		}

		OutputTile.Add(Tile);
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

	const int32 WestIdx = CurrentPixelIdx - 4;
	const int32 NorthIdx = (CurrentPixelIdx - (Y * IndexTextureResolution));
	const int32 EastIdx = CurrentPixelIdx + 4;
	const int32 SouthIdx = (CurrentPixelIdx + (Y * IndexTextureResolution));

	if (bIsXCornor || bIsYCornor)
	{
		//Left top pixel could be any tiles

		if (X == 0 && Y == 0)
		{
			West = FMath::RandRange(0, 1);
			East = FMath::RandRange(0, 1);
			North = FMath::RandRange(0, 1);
			South = FMath::RandRange(0, 1);
		}
		//only top pixel should consider left pixels
		else if (X != 0 && Y == 0)
		{
			West = ((InPixelArray[WestIdx] & EAST_BIT) != 0) ? 1 : 0; // Get West Side's East Pixel(To find out connection)
			East = FMath::RandRange(0, 1);
			North = FMath::RandRange(0, 1);
			South = FMath::RandRange(0, 1);

			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d"), West)
		}
		//only left pixel should consider left pixels
		else if (X == 0 && Y != 0)
		{
			West = FMath::RandRange(0, 1);
			East = FMath::RandRange(0, 1);
			South = FMath::RandRange(0, 1);
			North = ((InPixelArray[NorthIdx] & SOUTH_BIT) != 0) ? 1 : 0; // Get North Side's South Pixel(To find out connection)

			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> North %d "), North)
		}
		//right and none top pixels should consider north and west pixels.
		else if (X != 0 && Y != 0)
		{
			East = FMath::RandRange(0, 1);
			South = FMath::RandRange(0, 1);
			West = ((InPixelArray[WestIdx] & EAST_BIT) != 0) ? 1 : 0; // Get West Side's East Pixel(To find out connection)
			North = ((InPixelArray[NorthIdx] & SOUTH_BIT) != 0) ? 1 : 0; // Get North Side's South Pixel(To find out connection)

			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d , North %d"), West, North)
		}
	}
	else //If it's not cornor, then search four neighbour pixels and matches
	{
		West = ((InPixelArray[WestIdx] & EAST_BIT) != 0) ? 1 : 0; // Get West Side's East Pixel(To find out connection)
		North = ((InPixelArray[NorthIdx] & SOUTH_BIT) != 0) ? 1 : 0; // Get North Side's South Pixel(To find out connection)
		East = ((InPixelArray[EastIdx] & WEST_BIT) != 0) ? 1 : 0;  // Get East Side's West Pixel(To find out connection)
		South = ((InPixelArray[SouthIdx] & NORTH_BIT) != 0) ? 1 : 0; // Get South Side's North Pixel(To find out connection)

		UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d , North %d , East %d , South %d"), West, North, East, South)
	}

	FWangTileData SearchTile = FWangTileData(West, North, East, South);
	const uint8 WangTileIndex = SearchWangTileIndex(SearchTile);

	UE_LOG(LogTemp,Warning,TEXT("West : %d , North %d , East %d , South %d"), SearchTile.West, SearchTile.North, SearchTile.East, SearchTile.South)
	UE_LOG(LogTemp, Warning, TEXT("WangTileIndex %d"), WangTileIndex)
	UE_LOG(LogTemp, Warning, TEXT(""))

	return WangTileIndex;
}