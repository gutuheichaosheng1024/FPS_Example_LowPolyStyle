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

void UPickUpComponent::OnRegister()
{
    Super::OnRegister();
    CacheWeaponFromOwner();
}

#if WITH_EDITOR
void UPickUpComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    CacheWeaponFromOwner();
}
#endif

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
            // 服务器本地（AI）→ 直接拾取
            TryPickup(Character);
        }
        else
        {
            // 客户端 → 通过 Server RPC 请求拾取
            if (CachedWeapon && Character->CanEquipWeaponInSlot(CachedWeapon->GetTargetSlot(), CachedWeapon))
            {
                Character->Server_RequestPickup(CachedWeapon);
            }
        }
    }
}

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

void UPickUpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ProcessCurrentOverlaps();
}

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