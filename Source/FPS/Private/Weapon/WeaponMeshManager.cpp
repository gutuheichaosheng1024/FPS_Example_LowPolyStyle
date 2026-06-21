#include "Weapon/WeaponMeshManager.h"
#include "Character/FPS_CharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

FWeaponMeshManager::FWeaponMeshManager()
{
}

// 搜索并缓存 FP 和 TP 武器的弹夹插槽组件
// 流程：遍历 WeaponMesh 子组件查找 Socket_Magazine/Socket_Magazine_Reverse → 遍历 WeaponMeshStatic 子组件查找 Socket_Magazine_Static/Socket_Magazine_Reverse_Static → 存入成员变量
void FWeaponMeshManager::CacheMagazineSlots(USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
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

// 搜索并缓存 FP 武器的瞄准插槽组件
// 流程：遍历 WeaponMesh 子组件 → 查找名为 AimSocket 的组件 → 存入 AimSocketComponent
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

// 设置弹夹为正常状态（Reverse 隐藏，Normal 显示）
// 流程：FP/TP Reverse 插槽隐藏 → FP/TP Normal 插槽显示
void FWeaponMeshManager::SetMagazineNormalState()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, false);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, true);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, false);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, true);
}

// 设置弹夹为换弹状态（全部可见）
// 流程：FP/TP 所有弹夹插槽全部显示
void FWeaponMeshManager::SetMagazineReloadState()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, true);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, true);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, true);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, true);
}

// 隐藏所有弹夹插槽
// 流程：FP/TP 所有弹夹插槽全部隐藏
void FWeaponMeshManager::HideAllMagazines()
{
    if (MagazineReverseSlot) SetMagazineSlotVisibility(MagazineReverseSlot, false);
    if (MagazineSlot) SetMagazineSlotVisibility(MagazineSlot, false);
    if (MagazineReverseSlotStatic) SetMagazineSlotVisibility(MagazineReverseSlotStatic, false);
    if (MagazineSlotStatic) SetMagazineSlotVisibility(MagazineSlotStatic, false);
}

// 设置单个插槽及子组件的可见性
// 流程：设置插槽自身可见性 → 遍历子组件 → 对 UPrimitiveComponent 子组件递归设置可见性
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

// 显示/隐藏武器 Mesh（含静态子组件，弹夹状态由调用方独立管理）
// 流程：设置 WeaponMesh/WeaponMeshStatic 可见性 → 设置静态子组件可见性
void FWeaponMeshManager::ShowWeaponMeshes(bool bShow, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (WeaponMesh) WeaponMesh->SetVisibility(bShow);
    if (WeaponMeshStatic) WeaponMeshStatic->SetVisibility(bShow);
    SetStaticMeshComponentsVisibility(bShow, WeaponMesh, WeaponMeshStatic);
}

// 设置 FP/TP 静态子组件的 OwnerSee/OwnerNoSee
// 流程：遍历 WeaponMesh 子组件 → 对所有 UStaticMeshComponent 设置 OnlyOwnerSee → 遍历 WeaponMeshStatic 子组件 → 设置 OwnerNoSee
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

// 设置 FP/TP 静态子组件的可见性（跳过弹夹子组件）
// 流程：遍历 WeaponMesh 子组件 → 跳过弹夹插槽的子组件 → 设置其余 UStaticMeshComponent 可见性 → 对 WeaponMeshStatic 同理
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

// 将武器 FP/TP Mesh 挂载到角色对应骨骼插槽
// 流程：禁用 WeaponMeshStatic/WeaponMesh 碰撞 → 获取角色 FP/TP Mesh → FP Mesh 挂载到角色 FP AnimMesh 的 AttachSocketName → 设置 OnlyOwnerSee → TP Mesh 挂载到角色 TP Mesh → 设置 OwnerNoSee
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

// 从角色卸载武器 FP/TP Mesh
// 流程：检查 WeaponMesh 是否挂载在角色 FP AnimMesh 上 → 是则 Detach → 检查 WeaponMeshStatic 是否挂载在角色 TP Mesh 上 → 是则 Detach
void FWeaponMeshManager::DetachFromCharacter(AFPS_CharacterBase* Character, USkeletalMeshComponent* WeaponMesh, USkeletalMeshComponent* WeaponMeshStatic)
{
    if (!Character) return;
    if (WeaponMesh && WeaponMesh->GetAttachParent() == Character->GetFPAnimMesh())
        WeaponMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    if (WeaponMeshStatic && WeaponMeshStatic->GetAttachParent() == Character->GetMesh())
        WeaponMeshStatic->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

// 递归设置组件及其所有子组件的碰撞状态
// 流程：根据 bEnableCollision 启用或禁用 RootComp 碰撞 → 遍历所有子组件 → 对每个 UPrimitiveComponent 同样启用或禁用碰撞
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
