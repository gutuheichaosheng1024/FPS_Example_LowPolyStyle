#include "Weapon/PickUpComponent.h"
#include "Character/FPS_CharacterBase.h"
#include "Weapon/WeaponActor.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

UPickUpComponent::UPickUpComponent()
{
    SphereRadius = 32.f;

    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    SetGenerateOverlapEvents(true);
    SetHiddenInGame(true);
    SetCanEverAffectNavigation(false);

    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

// 初始化组件：缓存武器引用并绑定重叠事件
// 流程：调用 CacheWeaponFromOwner → 若未找到武器则禁用 Tick 和碰撞 → 绑定 OnComponentBeginOverlap → 设置 Tick 间隔 0.1s
void UPickUpComponent::BeginPlay()
{
    Super::BeginPlay();

    CacheWeaponFromOwner();

    if (!CachedWeapon)
    {
        UE_LOG(LogTemp, Warning, TEXT("PickUpComponent: No owning weapon found on %s"), *GetNameSafe(GetOwner()));
        SetComponentTickEnabled(false);
        SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return;
    }

    OnComponentBeginOverlap.AddDynamic(this, &UPickUpComponent::OnSphereBeginOverlap);
    PrimaryComponentTick.TickInterval = 0.1f;
}

// 组件注册时缓存武器引用
// 流程：调用 CacheWeaponFromOwner
void UPickUpComponent::OnRegister()
{
    Super::OnRegister();
    CacheWeaponFromOwner();
}

#if WITH_EDITOR
// 编辑器属性变更时刷新武器缓存
// 流程：调用父类 PostEditChangeProperty → 调用 CacheWeaponFromOwner
void UPickUpComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    CacheWeaponFromOwner();
}
#endif

// 从 Owner 或 AttachParent 的 Owner 中查找并缓存 AWeaponActor 引用
// 流程：跳过 CDO → 尝试从 Owner 转换 → 若失败则从 AttachParent 的 Owner 转换 → 存入 CachedWeapon
void UPickUpComponent::CacheWeaponFromOwner()
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        CachedWeapon = nullptr;
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        CachedWeapon = nullptr;
        return;
    }

    if (AWeaponActor* Weapon = Cast<AWeaponActor>(OwnerActor))
    {
        CachedWeapon = Weapon;
        return;
    }

    if (USceneComponent* ParentComp = GetAttachParent())
    {
        if (AWeaponActor* WeaponFromAttach = Cast<AWeaponActor>(ParentComp->GetOwner()))
        {
            CachedWeapon = WeaponFromAttach;
            return;
        }
    }

    CachedWeapon = nullptr;
}

// 重叠开始时处理拾取逻辑
// 流程：转换 OtherActor 为 AFPS_CharacterBase → 服务器直接拾取 → 客户端通过 Server RPC 请求拾取
void UPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (AFPS_CharacterBase* Character = Cast<AFPS_CharacterBase>(OtherActor))
    {
        if (Character->HasAuthority())
        {
            TryPickup(Character);
        }
        else
        {
            if (CachedWeapon && Character->CanEquipWeaponInSlot(CachedWeapon->GetTargetSlot(), CachedWeapon))
            {
                Character->Server_RequestPickup(CachedWeapon);
            }
        }
    }
}

// 重叠结束时禁用组件 Tick
// 流程：转换 OtherActor → 若组件 Tick 启用则禁用它
void UPickUpComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (AFPS_CharacterBase* Character = Cast<AFPS_CharacterBase>(OtherActor))
    {
        if (IsComponentTickEnabled())
        {
            SetComponentTickEnabled(false);
        }
    }
}

// 每帧 Tick：处理当前重叠的 Actor 拾取
// 流程：调用 ProcessCurrentOverlaps
void UPickUpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ProcessCurrentOverlaps();
}

// 尝试拾取武器
// 流程：检查角色和武器有效性 → 检查角色武器槽是否可用 → 不可用则按需恢复 Tick → 可用则调用 EquipWeaponActor 并禁用 Tick
void UPickUpComponent::TryPickup(AFPS_CharacterBase* Character)
{
    if (!Character || !CachedWeapon)
    {
        return;
    }

    if (!Character->CanEquipWeaponInSlot(CachedWeapon->GetTargetSlot(), CachedWeapon))
    {
        SetComponentTickEnabled(Character->GetWeaponCount() < Character->MaxWeaponCount);
        return;
    }

    Character->EquipWeaponActor(CachedWeapon);
    SetComponentTickEnabled(false);
}

// 处理当前所有重叠 Actor 的拾取尝试
// 流程：获取所有重叠的 AFPS_CharacterBase → 遍历并调用 TryPickup
void UPickUpComponent::ProcessCurrentOverlaps()
{
    if (!CachedWeapon)
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    GetOverlappingActors(OverlappingActors, AFPS_CharacterBase::StaticClass());

    for (AActor* Actor : OverlappingActors)
    {
        if (AFPS_CharacterBase* Character = Cast<AFPS_CharacterBase>(Actor))
        {
            TryPickup(Character);
        }
    }
}
