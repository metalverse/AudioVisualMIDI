// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SoundComponent.h"


// Sets default values for this component's properties
USoundComponent::USoundComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USoundComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void USoundComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USoundComponent::callSoundEvent(int mode, int value, int pitchId, float volume) {
	OnEventSoundDetected.Broadcast(mode, value, pitchId, volume);
}

void USoundComponent::callMidiEvent(int channel, int noteId, int timeDifference, int type) {
	OnEventMidiToPlay.Broadcast(channel, noteId, timeDifference, type);
}

void USoundComponent::callPainFloorEvent(bool isActive) {
	OnEventPaintFloor.Broadcast(isActive);
}

void USoundComponent::callWallIneraction(bool isActive, int mode, int noteId) {
	OnEventWallInteraction.Broadcast(isActive, mode, noteId);
}