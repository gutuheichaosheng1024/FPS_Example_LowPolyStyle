#include "Weapon/WeaponMeshManager.h"
#include "Character/FPS_CharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

FWeaponMeshManager::FWeaponMeshManager()
{
}

// ======================================================================
// 缓存
// ======================================================================

void FWeaponMeshManager::CacheMagazineSlots(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    // 第一人称弹夹插槽
    if (WeaponMesh)
    {
        TArray<USceneComponent*> ChildComponents;
        WeaponMesh->GetChildrenComponents(true, ChildComponents);
        for (USceneComponent* Child : ChildComponents)
        {
            if (!MagazineSlot && Child->GetFName() == TEXT("Socket_Magazine"))
                MagazineSlot = Child;
            if (!MagazineReverseSlot && Child->GetFName() == TEXT("Socket_Magazine_Reverse"))
                MagazineReverseSlot = Child;
        }
    }

    // 第三人称弹夹插槽 (后缀 _Static)
    if (WeaponMeshStatic)
    {
        TArray<USceneComponent*> ChildComponents;
        WeaponMeshStatic->GetChildrenComponents(true, ChildComponents);
        for (USceneComponent* Child : ChildComponents)
        {
            if (!MagazineSlotStatic && Child->GetFName() == TEXT("Socket_Magazine_Static"))
                MagazineSlotStatic = Child;
            if (!MagazineReverseSlotStatic && Child->GetFName() == TEXT("Socket_Magazine_Reverse_Static"))
                MagazineReverseSlotStatic = Child;
        }
    }
}

void FWeaponMeshManager::CacheAimSocket(USkeletalMeshComponent* WeaponMesh)
{
    if (AimSocketComponent || !WeaponMesh) return;
    TArray<USceneComponent*> ChildComponents;
    WeaponMesh->GetChildrenComponents(true, ChildComponents);
    for (USceneComponent* Child : ChildComponents)
    {
        if (Child->GetFName() == TEXT("AimSocket"))
        {
            AimSocketComponent = Child;
            break;
        }
    }
}

// ======================================================================
// 弹夹可见性
// ======================================================================

void FWeaponMeshManager::SetMagazineNormalState()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, false);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, true);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, false);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, true);
}

void FWeaponMeshManager::SetMagazineReloadState()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, true);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, true);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, true);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, true);
}

void FWeaponMeshManager::HideAllMagazines()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, false);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, false);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, false);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, false);
}

void FWeaponMeshManager::SetMagazineSlotVisibility(USceneComponent* Slot, bool bVisible)
{
    if (!Slot) return;

    Slot->SetVisibility(bVisible, true);

    TArray<USceneComponent*> SlotChildren = Slot->GetAttachChildren();
    for (USceneComponent* Child : SlotChildren)
    {
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Child))
        {
            Prim->SetVisibility(bVisible, true);
        }
    }
}

// ======================================================================
// 武器 Mesh 可见性
// ======================================================================

void FWeaponMeshManager::ShowWeaponMeshes(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (WeaponMesh) WeaponMesh->SetVisibility(bShow);
    if (WeaponMeshStatic) WeaponMeshStatic->SetVisibility(bShow);
    SetStaticMeshComponentsVisibility(bShow, WeaponMesh, WeaponMeshStatic);
    // 弹夹状态由 WeaponActor 根据 Reloading 状态独立管理
}

void FWeaponMeshManager::SetStaticMeshComponentsOwnerVisibility(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (!WeaponMesh || !WeaponMeshStatic) return;
    TArray<USceneComponent*> FPChildren;
    WeaponMesh->GetChildrenComponents(true, FPChildren);
    for (USceneComponent* Child : FPChildren)
    {
        if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(Child))
            SM->SetOnlyOwnerSee(true);
    }
    TArray<USceneComponent*> TPChildren;
    WeaponMeshStatic->GetChildrenComponents(true, TPChildren);
    for (USceneComponent* Child : TPChildren)
    {
        if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(Child))
            SM->SetOwnerNoSee(true);
    }
}

void FWeaponMeshManager::SetStaticMeshComponentsVisibility(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (!WeaponMesh || !WeaponMeshStatic) return;
    TArray<USceneComponent*> FPChildren;
    WeaponMesh->GetChildrenComponents(true, FPChildren);
    for (USceneComponent* Child : FPChildren)
    {
        if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(Child))
        {
            USceneComponent* Parent = SM->GetAttachParent();
            if (Parent == MagazineSlot || Parent == MagazineReverseSlot)
                continue;
            SM->SetVisibility(bShow);
        }
    }
    TArray<USceneComponent*> TPChildren;
    WeaponMeshStatic->GetChildrenComponents(true, TPChildren);
    for (USceneComponent* Child : TPChildren)
    {
        if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(Child))
        {
            USceneComponent* Parent = SM->GetAttachParent();
            if (Parent == MagazineSlotStatic || Parent == MagazineReverseSlotStatic)
                continue;
            SM->SetVisibility(bShow);
        }
    }
}

// ======================================================================
// 挂载 / 卸载
// ======================================================================

void FWeaponMeshManager::AttachToCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic, FName AttachSocketName)
{
    if (!Character) return;
    SetComponentAndChildrenCollision(WeaponMeshStatic, false);
    SetComponentAndChildrenCollision(WeaponMesh, false);
    USkeletalMeshComponent* Mesh1P = Character->GetFPAnimMesh();
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
    if (Mesh1P && WeaponMesh)
    {
        WeaponMesh->AttachToComponent(Mesh1P, Rules, AttachSocketName);
        WeaponMesh->SetOnlyOwnerSee(true);
    }
    if (Mesh && WeaponMeshStatic)
    {
        WeaponMeshStatic->AttachToComponent(Mesh, Rules, AttachSocketName);
        WeaponMeshStatic->SetOwnerNoSee(true);
    }
}

void FWeaponMeshManager::DetachFromCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (!Character) return;
    if (WeaponMesh && WeaponMesh->GetAttachParent() == Character->GetFPAnimMesh())
        WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    if (WeaponMeshStatic && WeaponMeshStatic->GetAttachParent() == Character->GetMesh())
        WeaponMeshStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

// ======================================================================
// 内部工具
// ======================================================================

void FWeaponMeshManager::SetComponentAndChildrenCollision(UPrimitiveComponent* RootComp, bool bEnableCollision)
{
    if (!RootComp) return;
    if (bEnableCollision)
    {
        RootComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    else
    {
        RootComp->SetCollisionProfileName(TEXT("NoCollision"));
        RootComp->SetSimulatePhysics(false);
    }
    TArray<USceneComponent*> Children;
    RootComp->GetChildrenComponents(true, Children);
    for (USceneComponent* Child : Children)
    {
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Child))
        {
            if (bEnableCollision)
            {
                Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            }
            else
            {
                Prim->SetCollisionProfileName(TEXT("NoCollision"));
                Prim->SetSimulatePhysics(false);
            }
        }
    }
}
