// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "PickUpComponent.generated.h"


class AFPS_CharacterBase;
class AWeaponActor;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUp, AFPS_CharacterBase*, PickUpCharacter);

UCLASS(Blueprintable, BlueprintType)
class FPS_API UPickUpComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UPickUpComponent();

    /** 玩家成功拾取武器时回调 */
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnPickUp OnPickUp;

    void TryPickup(AFPS_CharacterBase* Character);

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;
    virtual void OnRegister() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

private:
    UPROPERTY(Transient)
    AWeaponActor* CachedWeapon = nullptr;

    void CacheWeaponFromOwner();
    void ProcessCurrentOverlaps();
};
