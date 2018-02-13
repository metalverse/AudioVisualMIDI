// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "SoundComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FEventSound, int, mode, int, value, int, pitchId, float, volume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FEventPlayMidi, int, channel, int, noteId, int, timeDifference, int, type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventPaintFloor, bool, isActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventWallInteraction, bool, isActive, int, noteId);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), meta = (DisplayName = "SoundComponent"))
class MIDI_PROJECT_API USoundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoundComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Sound")
		void callSoundEvent(int mode, int value, int pitchId, float volume);
	UFUNCTION(BlueprintCallable, Category = "Sound")
		void callMidiEvent(int channel, int noteId, int timeDifference, int type);
	UFUNCTION(BlueprintCallable, Category = "Sound")
		void callPainFloorEvent(bool isActive);
	UFUNCTION(BlueprintCallable, Category = "Sound")
		void callWallIneraction(bool isActive, int noteId);

protected:
	UPROPERTY(BlueprintAssignable, Category = "Sound")
		FEventSound OnEventSoundDetected;
	UPROPERTY(BlueprintAssignable, Category = "Sound")
		FEventPlayMidi OnEventMidiToPlay;
	UPROPERTY(BlueprintAssignable, Category = "Sound")
		FEventPaintFloor OnEventPaintFloor;
	UPROPERTY(BlueprintAssignable, Category = "Sound")
		FEventWallInteraction OnEventWallInteraction;
};