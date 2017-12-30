// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "BeatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventBeat, int, mode, int, value);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), meta = (DisplayName = "BeatComponent"))
class MIDI_PROJECT_API UBeatComponent : public UActorComponent//, public BeatEventListener
{
	GENERATED_BODY()


public:
	// Sets default values for this component's properties
	UBeatComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION(BlueprintCallable, Category = "Beat")
	void callBeatEvent(int mode, int value);

//private:
//	// Beat Event Listener
//	void onEventBeat(int mode, int value) {};
		
protected:
	UPROPERTY(BlueprintAssignable, Category = "Beat")
	FEventBeat OnEventBeat;
};
