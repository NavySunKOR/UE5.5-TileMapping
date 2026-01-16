// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CreateMappingTextureActor.generated.h"

#define WEST_BIT 3
#define NORTH_BIT 0
#define EAST_BIT 1
#define SOUTH_BIT 2

USTRUCT(BlueprintType)
struct FWangTileData
{
	GENERATED_BODY()
public:
	FWangTileData()
	{
		West = 0;
		North = 0;
		East = 0;
		South = 0;
		TileIndice = 0;
	}
	FWangTileData(uint8 West, uint8 North, uint8 East, uint8 South)
	{
		this->West = West;
		this->North = North;
		this->East = East;
		this->South = South;

		uint8 NETile = (North << NORTH_BIT) | (East << EAST_BIT);
		uint8 SWTile = (South << SOUTH_BIT) | (West << WEST_BIT);

		TileIndice = NETile | SWTile;
	}

	FWangTileData(uint8 Indice)
	{
		TileIndice = Indice;
		West = ((Indice & WEST_BIT) > 0) ? 1 : 0;
		South = ((Indice & SOUTH_BIT) > 0) ? 1 : 0;
		East = ((Indice & EAST_BIT) > 0) ? 1 : 0;
		North = ((Indice & NORTH_BIT) > 0)? 1 : 0;
	}

	void CalcTileIndice()
	{
		uint8 NETile = (North << NORTH_BIT) | (East << EAST_BIT);
		uint8 SWTile = (South << SOUTH_BIT) | (West << WEST_BIT);

		TileIndice = NETile | SWTile;
	}

	UPROPERTY(EditAnywhere)
	int8 West;
	UPROPERTY(EditAnywhere)
	int8 North;
	UPROPERTY(EditAnywhere)
	int8 East;
	UPROPERTY(EditAnywhere)
	int8 South;

	int8 TileIndice;
};

UCLASS()
class ACreateMappingTextureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACreateMappingTextureActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "RandomTile")
	void CIE_CreateRandomTileIndexTexture();

	UFUNCTION(CallInEditor, Category = "WangTile")
	void CIE_CreateWangTileIndexTexture();
#endif

	uint8 SearchWangTileIndex(uint8 SearchIdx);
	uint8 GetWangTileIndex(uint8* InPixelArray, int32 CurrentPixelIdx);

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere , Category="RandomTile")
	int32 TileAtlasTextureResolution;

	UPROPERTY(EditAnywhere, Category = "RandomTile")
	int32 PerTileResolution;

	UPROPERTY(EditAnywhere, Category = "RandomTile")
	int32 IndexTextureResolution;

	UPROPERTY(EditAnywhere, Category = "RandomTile")
	FString SaveDir;

	UPROPERTY(EditAnywhere, Category = "RandomTile")
	FString SaveName;
#endif

	UPROPERTY(EditAnywhere)
	bool bUseManualMapping = false;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseManualMapping",EditConditionHides))
	TArray<FWangTileData> Tiles;


};
