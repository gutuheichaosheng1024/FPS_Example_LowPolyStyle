#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "PickUpComponent.generated.h"


class AFPS_CharacterBase;
class AWeaponActor;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUp, AFPS_CharacterBase*, PickUpCharacter);

/**
 * UPickUpComponent — 武器拾取碰撞组件
 *
 * 职责：作为 SphereComponent 检测角色重叠，在服务器端或通过 RPC 触发武器拾取逻辑，管理拾取后的组件状态（启停 Tick/碰撞）
 * 使用：AFPS_CharacterBase（拾取角色）、AWeaponActor（所属武器）
 */
UCLASS(Blueprintable, BlueprintType)
class FPS_API UPickUpComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UPickUpComponent();

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
