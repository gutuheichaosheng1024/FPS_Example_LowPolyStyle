#pragma once

#include "CoreMinimal.h"

class AFPS_CharacterBase;
class USkeletalMeshComponent;
class USceneComponent;
class UPrimitiveComponent;

/**
 * 武器网格/可见性/弹夹管理器
 * 管理双 Mesh（FP/TP）的挂载、可见性、弹夹插槽缓存
 * 由 WeaponActor 持有
 */
struct FWeaponMeshManager
{
public:
    FWeaponMeshManager();

    // ======================================================================
    // 缓存
    // ======================================================================

    /** 搜索并缓存弹夹插槽（FP + TP） */
    void CacheMagazineSlots(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    /** 搜索并缓存瞄准插槽（仅 FP） */
    void CacheAimSocket(USkeletalMeshComponent* WeaponMesh);

    // ======================================================================
    // 弹夹可见性 — 3 种状态
    // ======================================================================

    /** 正常状态：Reverse 隐藏, Normal 显示 */
    void SetMagazineNormalState();

    /** 换弹状态：全部显示 */
    void SetMagazineReloadState();

    /** 全部隐藏 */
    void HideAllMagazines();

    /** 设置单个插槽可见性（含子组件传播） */
    void SetMagazineSlotVisibility(USceneComponent* Slot, bool bVisible);

    // ======================================================================
    // 武器 Mesh 可见性
    // ======================================================================

    /** 显示/隐藏武器 Mesh（含静态子组件，不含弹夹状态——由调用方管理） */
    void ShowWeaponMeshes(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    /** 设置 FP/TP 静态子组件的 OwnerSee/OwnerNoSee */
    void SetStaticMeshComponentsOwnerVisibility(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    /** 设置 FP/TP 静态子组件可见性（跳过弹夹子组件） */
    void SetStaticMeshComponentsVisibility(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    // ======================================================================
    // 挂载 / 卸载
    // ======================================================================

    /** 挂载武器 Mesh 到角色（禁用碰撞 + AttachToComponent） */
    void AttachToCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic, FName AttachSocketName);

    /** 从角色卸载武器 Mesh */
    void DetachFromCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic);

    // ======================================================================
    // 访问器
    // ======================================================================

    USceneComponent* GetAimSocketComponent() const { return AimSocketComponent; }

private:
    /** 递归禁用/启用组件及所有子组件的碰撞 */
    static void SetComponentAndChildrenCollision(UPrimitiveComponent* RootComp, bool bEnableCollision);

    // 缓存的插槽指针
    USceneComponent* MagazineSlot = nullptr;
    USceneComponent* MagazineReverseSlot = nullptr;
    USceneComponent* MagazineSlotStatic = nullptr;
    USceneComponent* MagazineReverseSlotStatic = nullptr;
    USceneComponent* AimSocketComponent = nullptr;
};
