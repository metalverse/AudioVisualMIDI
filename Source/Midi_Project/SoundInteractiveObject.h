// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "SoundInteractiveObject.generated.h"

UCLASS(BlueprintType)
class MIDI_PROJECT_API ASoundInteractiveObject : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASoundInteractiveObject();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/*UFUNCTION(BlueprintImplentableEvent, category = "Sound Interaction")
	void NewSoundInteraction();*/
	
};
