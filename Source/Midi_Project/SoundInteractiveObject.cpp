// Fill out your copyright notice in the Description page of Project Settings.

#include "Midi_Project.h"
#include "SoundInteractiveObject.h"


// Sets default values
ASoundInteractiveObject::ASoundInteractiveObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASoundInteractiveObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASoundInteractiveObject::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

//void ASoundInteractiveObject::NewSoundInteraction() {}