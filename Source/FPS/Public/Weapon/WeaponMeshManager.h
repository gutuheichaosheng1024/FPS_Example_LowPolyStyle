#pragma once

#include "CoreMinimal.h"

class AFPS_CharacterBase;
class USkeletalMeshComponent;
class USceneComponent;
class UPrimitiveComponent;

/**
 * FWeaponMeshManager — 武器网格与可见性管理器
 *
 * 职责：管理武器 FP/TP 双 Mesh 的挂载卸载、可见性切换（含静态子组件）、弹夹插槽缓存及三种弹夹可见性状态（正常/换弹/全部隐藏），以及瞄准插槽缓存
 * 使用：AFPS_CharacterBase（持有者角色）、USkeletalMeshComponent（武器骨骼网格）、USceneComponent（插槽组件）、UStaticMeshComponent（静态子组件）
 */
struct FWeaponMeshManager
{
public:
    FWeaponMeshManager();

    void CacheMagazineSlots(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);
    void CacheAimSocket(USkeletalMeshComponent* WeaponMesh);

    void SetMagazineNormalState();
    void SetMagazineReloadState();
    void HideAllMagazines();
    void SetMagazineSlotVisibility(USceneComponent* Slot, bool bVisible);

    void ShowWeaponMeshes(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    void SetStaticMeshComponentsOwnerVisibility(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);
    void SetStaticMeshComponentsVisibility(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    void AttachToCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic, FName AttachSocketName);
    void DetachFromCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    USceneComponent* GetAimSocketComponent() const { return AimSocketComponent; }

private:
    static void SetComponentAndChildrenCollision(UPrimitiveComponent* RootComp, bool bEnableCollision);

    USceneComponent* MagazineSlot = nullptr;
    USceneComponent* MagazineReverseSlot = nullptr;
    USceneComponent* MagazineSlotStatic = nullptr;
    USceneComponent* MagazineReverseSlotStatic = nullptr;
    USceneComponent* AimSocketComponent = nullptr;
};
