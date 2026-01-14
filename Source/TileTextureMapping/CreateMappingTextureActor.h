// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CreateMappingTextureActor.generated.h"

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
	UFUNCTION(CallInEditor)
	void CIE_CreateTexture();

	UPROPERTY(EditAnywhere)
	int32 OriginalTileCounts;

	UPROPERTY(EditAnywhere)
	int32 MappingTileCounts;

	UPROPERTY(EditAnywhere)
	FString SaveDir;

	UPROPERTY(EditAnywhere)
	FString SaveName;
};
