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
			float Rate = ((float)SelectedTiles1D / 15);
			const uint8 To255 = (uint8)(Rate * 255);
			ArrayData[i + 2] = To255; //R
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
	MappingTexture->CompressionSettings = TC_VectorDisplacementmap;
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
//0 - whatever, 1 - On only , 2 - Off only
uint8 ACreateMappingTextureActor::SearchWangTileIndex(const uint8 West, const uint8 North, const uint8 East, const uint8 South)
{
	TArray<FWangTileData> OutputTile;
	for (const FWangTileData& Tile : Tiles)
	{
		if ((East == 1 && (Tile.East == 0)) || 
			(East == 2 && (Tile.East == 1)))
		{
			continue;
		}

		if ((West == 1 && (Tile.West == 0)) ||
			(West == 2 && (Tile.West == 1)))
		{
			continue;
		}

		if ((South == 1 && (Tile.South == 0)) ||
			(South == 2 && (Tile.South == 1)))
		{
			continue;
		}

		if ((North == 1 && (Tile.North == 0)) ||
			(North == 2 && (Tile.North == 1)))
		{
			continue;
		}

		OutputTile.Add(Tile);
	}

	const FWangTileData& Result = OutputTile[FMath::RandRange(0, OutputTile.Num() - 1)];

	UE_LOG(LogTemp, Warning, TEXT("Selected -> East : %d , West : %d , South : %d , North : %d"), Result.East, Result.West, Result.South, Result.North)
	return Result.TileIndice;
}

uint8 ACreateMappingTextureActor::GetWangTileIndex(uint8* InPixelArray, int32 CurrentPixelIdx)
{
	const int32 ActualPixelIdx = CurrentPixelIdx / 4;
	int32 X = ActualPixelIdx % IndexTextureResolution;
	int32 Y = ActualPixelIdx / IndexTextureResolution;

	//Corner check
	const bool bIsXCornor = (X == 0 || X == (IndexTextureResolution - 1));
	const bool bIsYCornor = (Y == 0 || Y == (IndexTextureResolution - 1));

	//This state needs 3 type of state : whatever,on only, off only
	uint8 West = 0;
	uint8 North = 0;
	uint8 East = 0;
	uint8 South = 0;

	const int32 WestIdx = CurrentPixelIdx - 4;
	const int32 NorthIdx = (Y > 0) ? (CurrentPixelIdx - (IndexTextureResolution * 4)) : 0;
	UE_LOG(LogTemp, Warning, TEXT("CurrentPixelIdx : %d  / Calc : %d"), CurrentPixelIdx, IndexTextureResolution * 4)
	UE_LOG(LogTemp, Warning, TEXT("WestIdx : %d / NorthIdx : %d"), WestIdx, NorthIdx)
	UE_LOG(LogTemp, Warning, TEXT("X : %d / Y : %d"), X, Y)

	//UE_LOG(LogTemp, Warning, TEXT("CurrentPixelIdx : %d"), CurrentPixelIdx)
	//
	//UE_LOG(LogTemp, Warning, TEXT("bIsXCornor : %d / bIsYCornor : %d"), bIsXCornor, bIsYCornor)

	if (bIsXCornor || bIsYCornor)
	{
		//Left top pixel could be any tiles

		//only top pixel should consider left pixels
		if (X != 0 && Y == 0)
		{
			West = ((InPixelArray[WestIdx] &  (1 << EAST_BIT)) != 0) ? 1 : 2; // Get West Side's East Pixel(To find out connection)
			UE_LOG(LogTemp, Warning, TEXT("InPixelArray[WestIdx] : %d"), InPixelArray[WestIdx])
			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d"), West)
		}
		//only left pixel should consider left pixels
		else if (X == 0 && Y != 0)
		{
			North = ((InPixelArray[NorthIdx] & (1<<SOUTH_BIT)) != 0) ? 1 : 2; // Get North Side's South Pixel(To find out connection)
			UE_LOG(LogTemp, Warning, TEXT("InPixelArray[NorthIdx] : %d"), InPixelArray[NorthIdx])
			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> North %d "), North)
		}
		//if pixel points are in edge of east or south
		else if (X != 0 && Y != 0)
		{
			West = ((InPixelArray[WestIdx] & (1 << EAST_BIT)) != 0) ? 1 : 2; // Get West Side's East Pixel(To find out connection)
			North = ((InPixelArray[NorthIdx] & (1 << SOUTH_BIT)) != 0) ? 1 : 2; // Get North Side's South Pixel(To find out connection)
			UE_LOG(LogTemp, Warning, TEXT("InPixelArray[WestIdx] : %d"), InPixelArray[WestIdx])
			UE_LOG(LogTemp, Warning, TEXT("InPixelArray[NorthIdx] : %d"), InPixelArray[NorthIdx])
			UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d , North %d"), West, North)
		}
	}
	else //If it's not cornor, then search four neighbour pixels and matches
	{
		West = ((InPixelArray[WestIdx] & (1 << EAST_BIT)) != 0) ? 1 : 2; // Get West Side's East Pixel(To find out connection)
		North = ((InPixelArray[NorthIdx] & (1 << SOUTH_BIT)) != 0) ? 1 : 2; // Get North Side's South Pixel(To find out connection)
		UE_LOG(LogTemp, Warning, TEXT("InPixelArray[WestIdx] : %d"), InPixelArray[WestIdx])
		UE_LOG(LogTemp, Warning, TEXT("InPixelArray[NorthIdx] : %d"), InPixelArray[NorthIdx])
		UE_LOG(LogTemp, Warning, TEXT("Neighbour pixels -> West : %d , North %d , East %d , South %d"), West, North, East, South)
	}

	UE_LOG(LogTemp, Warning, TEXT("SearchTile -> West : %d , North %d , East %d , South %d"), West, North, East, South)
	const uint8 WangTileIndex = SearchWangTileIndex(West,North,East,South);

	UE_LOG(LogTemp, Warning, TEXT("WangTileIndex %d"), WangTileIndex)
	UE_LOG(LogTemp, Warning, TEXT(""))

	return WangTileIndex;
}