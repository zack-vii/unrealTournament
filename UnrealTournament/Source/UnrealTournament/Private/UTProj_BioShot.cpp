// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProj_BioShot.h"
#include "UnrealNetwork.h"
#include "UTImpactEffect.h"
#include "UTLift.h"
#include "UTProjectileMovementComponent.h"

static const float GOO_TIMER_TICK = 0.5f;

AUTProj_BioShot::AUTProj_BioShot(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ProjectileMovement->InitialSpeed = 4000.0f;
	ProjectileMovement->MaxSpeed = 6000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;
	ProjectileMovement->ProjectileGravityScale = 0.9f;

	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.1f;
	ProjectileMovement->Friction = 0.008f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 140.f;

	DamageParams.BaseDamage = 21.0f;

	Momentum = 40000.0f;

	SurfaceNormal = FVector(0.0f, 0.0f, 1.0f);
	SurfaceType = EHitType::HIT_None;
	SurfaceWallThreshold = 0.3f;

	RestTime = 10.f;
	bAlwaysShootable = true;

	GlobStrength = 1.f;
	MaxRestingGlobStrength = 6;
	DamageRadiusGainFactor = 1.f;
	InitialLifeSpan = 0.0f;
	ExtraRestTimePerStrength = 0.5f;

	SplashSpread = 0.8f;
	bSpawningGloblings = false;
	bLanded = false;
	bHasMerged = false;
	bCanTrack = false;

	LandedOverlapRadius = 16.f;
	LandedOverlapScaling = 7.f;
	MaxSlideSpeed = 1500.f;
	TrackingRange = 1000.f;
	MaxTrackingSpeed = 400.f;
	ProjectileMovement->HomingAccelerationMagnitude = 600.f;
	UUTProjectileMovementComponent* UTProjMovement = Cast<UUTProjectileMovementComponent>(ProjectileMovement);
	if (UTProjMovement)
	{
		UTProjMovement->bPreventZHoming = true;
	}
	GlobRadiusScaling = 4.f;
}

void AUTProj_BioShot::BeginPlay()
{
	Super::BeginPlay();
	if (!IsPendingKillPending())
	{
		SetGlobStrength(GlobStrength);

		// failsafe if never land
		GetWorld()->GetTimerManager().SetTimer(this, &AUTProj_BioShot::BioStabilityTimer, 10.f, false);
	}
}

void AUTProj_BioShot::OnBounce(const struct FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	Super::OnBounce(ImpactResult, ImpactVelocity);
	ProjectileMovement->MaxSpeed = MaxSlideSpeed;
	bCanTrack = true;

	// only one bounce sound
	BounceSound = NULL;
}

float AUTProj_BioShot::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		if (bLanded && !bExploded)
		{
			Explode(GetActorLocation(), SurfaceNormal);
		}
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AUTProj_BioShot::BioStabilityTimer()
{
	if (!bExploded)
	{
		Explode(GetActorLocation(), SurfaceNormal);
	}
}

void AUTProj_BioShot::Landed(UPrimitiveComponent* HitComp, const FVector& HitLocation)
{
	if (!bLanded)
	{
		bCanTrack = true;
		bLanded = true;
		bCanHitInstigator = true;
		bReplicateUTMovement = true;

		//Change the collision so that weapons make it explode
		CollisionComp->SetCollisionProfileName("ProjectileShootable");

		//Rotate away from the floor
		FRotator NormalRotation = (SurfaceNormal).Rotation();
		NormalRotation.Roll = FMath::FRand() * 360.0f;	//Random spin
		SetActorRotation(NormalRotation);

		//Spawn Effects
		OnLanded();
		if ((LandedEffects != NULL) && (GetNetMode() != NM_DedicatedServer) && !MyFakeProjectile)
		{
			LandedEffects.GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(NormalRotation, HitLocation), HitComp, this, InstigatorController);
		}

		//Start the explosion timer
		if (GlobStrength < 1.f)
		{
			BioStabilityTimer();
		}
		if (!bPendingKillPending && (Role == ROLE_Authority))
		{
			GetWorld()->GetTimerManager().SetTimer(this, &AUTProj_BioShot::BioStabilityTimer, RestTime, false);
			SetGlobStrength(GlobStrength);
		}
	}
}

void AUTProj_BioShot::OnLanded_Implementation()
{
}

bool AUTProj_BioShot::CanInteractWithBio()
{
	return (!bExploded && !bHasMerged && !bSpawningGloblings && !bFakeClientProjectile && (Role == ROLE_Authority));
}

void AUTProj_BioShot::MergeWithGlob(AUTProj_BioShot* OtherBio)
{
	if (!OtherBio || !CanInteractWithBio() || !OtherBio->CanInteractWithBio() || (GlobStrength < 1.f))
	{
		//Let the globlings pass through so they dont explode the glob, ignore exploded bio
		return;
	}
	// let the landed glob grow
	if (!bLanded && OtherBio->bLanded)
	{
		return;
	}
	OtherBio->bHasMerged = true;

	if (OtherBio->GlobStrength > GlobStrength)
	{
		InstigatorController = OtherBio->InstigatorController;
		Instigator = OtherBio->Instigator;
	}

	SetGlobStrength(GlobStrength + OtherBio->GlobStrength);
	OtherBio->Destroy();
}

void AUTProj_BioShot::Track(AUTCharacter* NewTrackedPawn)
{
	// track closest
	if (!TrackedPawn || !ProjectileMovement->HomingTargetComponent.IsValid() || ((ProjectileMovement->HomingTargetComponent->GetComponentLocation() - GetActorLocation()).SizeSquared() > (NewTrackedPawn->CapsuleComponent->GetComponentLocation() - GetActorLocation()).SizeSquared()))
	{
		TrackedPawn = NewTrackedPawn;
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->SetUpdatedComponent(CollisionComp);
		ProjectileMovement->MaxSpeed = MaxTrackingSpeed / FMath::Sqrt(GlobStrength);
		ProjectileMovement->HomingTargetComponent = TrackedPawn->CapsuleComponent.Get();
		ProjectileMovement->bShouldBounce = true;
		bLanded = false;
		
		if (ProjectileMovement->Velocity.Size() < 1.5f*ProjectileMovement->BounceVelocityStopSimulatingThreshold)
		{
			ProjectileMovement->Velocity = 1.5f*ProjectileMovement->BounceVelocityStopSimulatingThreshold * (ProjectileMovement->HomingTargetComponent->GetComponentLocation() - GetActorLocation()).SafeNormal();
		}
	}
}

void AUTProj_BioShot::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (ProjectileMovement->bIsHomingProjectile)
	{
		if (!ProjectileMovement->HomingTargetComponent.IsValid() || ((ProjectileMovement->HomingTargetComponent->GetComponentLocation() - GetActorLocation()).SizeSquared() > FMath::Square(1.5f * TrackingRange)))
		{
			ProjectileMovement->HomingTargetComponent = NULL;
			ProjectileMovement->bIsHomingProjectile = false;
			TrackedPawn = NULL;
		}
	}
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void AUTProj_BioShot::ProcessHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	if (bHasMerged)
	{
		ShutDown(); 
		return;
	}

	AUTProj_BioShot* OtherBio = Cast<AUTProj_BioShot>(OtherActor);
	if (OtherBio)
	{
		MergeWithGlob(OtherBio);
	}
	else if (Cast<AUTCharacter>(OtherActor) != NULL || Cast<AUTProjectile>(OtherActor) != NULL)
	{
		AUTCharacter* TargetCharacter = Cast<AUTCharacter>(OtherActor);
		if (TargetCharacter && !bFakeClientProjectile && (Role == ROLE_Authority) && (TargetCharacter != Instigator))
		{
			// tell nearby bio that is on ground @TODO FIXMESTEVE OPTIMIZE
			for (TActorIterator<AUTProj_BioShot> It(GetWorld()); It; ++It)
			{
				AUTProj_BioShot* Glob = *It;
				if ((Glob != this) && !Glob->IsPendingKillPending() && Glob->bCanTrack && ((TargetCharacter->GetActorLocation() - Glob->GetActorLocation()).SizeSquared() < FMath::Square(TrackingRange)))
				{
					Glob->Track(TargetCharacter);
				}
			}
		}
		// set different damagetype for charged shots
		MyDamageType = (GlobStrength > 1.f) ? ChargedDamageType : GetClass()->GetDefaultObject<AUTProjectile>()->MyDamageType;
		float GlobScalingSqrt = FMath::Sqrt(GlobStrength);
		DamageParams = GetClass()->GetDefaultObject<AUTProjectile>()->DamageParams;
		DamageParams.BaseDamage *= GlobStrength;
		DamageParams.OuterRadius *= DamageRadiusGainFactor * GlobScalingSqrt;
		Momentum = GetClass()->GetDefaultObject<AUTProjectile>()->Momentum * GlobScalingSqrt;
		Super::ProcessHit_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
	}
	else if (!bLanded)
	{
		//Determine if we hit a Wall/Floor/Ceiling
		SurfaceNormal = HitNormal;
		if (FMath::Abs(SurfaceNormal.Z) > SurfaceWallThreshold) // A wall will have a low Z in the HitNormal since it's a unit vector
		{
			SurfaceType = (SurfaceNormal.Z >= 0) ? HIT_Floor : HIT_Ceiling;
		}
		else
		{
			SurfaceType = HIT_Wall;
		}

		SetActorLocation(HitLocation);
		Landed(OtherComp, HitLocation);

		AUTLift* Lift = Cast<AUTLift>(OtherActor);
		if (Lift && Lift->GetEncroachComponent())
		{
			AttachRootComponentTo(Lift->GetEncroachComponent(), NAME_None, EAttachLocation::KeepWorldPosition);
		}
	}
}
void AUTProj_BioShot::OnRep_GlobStrength()
{
	if (Cast<AUTProj_BioShot>(MyFakeProjectile))
	{
		Cast<AUTProj_BioShot>(MyFakeProjectile)->SetGlobStrength(GlobStrength);
	}
	else
	{
		SetGlobStrength(GlobStrength);
	}
}

void AUTProj_BioShot::SetGlobStrength(float NewStrength)
{
	if (bHasMerged)
	{
		return;
	}
	float OldStrength = GlobStrength;
	GlobStrength = NewStrength;
	ProjectileMovement->bShouldBounce = (GlobStrength <= 1.f);

	if (!bLanded)
	{
		//Set the projectile speed here so the client can mirror the strength speed
		ProjectileMovement->InitialSpeed = ProjectileMovement->InitialSpeed * (0.4f + GlobStrength) / (1.35f * GlobStrength);
		ProjectileMovement->MaxSpeed = ProjectileMovement->InitialSpeed;
	}
	// don't reduce remaining time for strength lost (i.e. SplashGloblings())
	else if (GlobStrength > OldStrength)
	{
		if (Role == ROLE_Authority)
		{
			float RemainingRestTime = GetWorld()->GetTimerManager().GetTimerRemaining(this, &AUTProj_BioShot::BioStabilityTimer) + (GlobStrength - OldStrength) * ExtraRestTimePerStrength;
			GetWorld()->GetTimerManager().SetTimer(this, &AUTProj_BioShot::BioStabilityTimer, RemainingRestTime, false);

		}
		// Glob merge effects
		UUTGameplayStatics::UTPlaySound(GetWorld(), MergeSound, this, ESoundReplicationType::SRT_IfSourceNotReplicated);
		if (GetNetMode() != NM_DedicatedServer)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MergeEffect, GetActorLocation() - CollisionComp->GetUnscaledSphereRadius(), SurfaceNormal.Rotation(), true);
		}
	}

	//Increase The collision of the flying Glob if over a certain strength
	float GlobScalingSqrt = FMath::Sqrt(GlobStrength);
	if (GlobStrength > 3.f)
	{
		CollisionComp->SetSphereRadius(GlobRadiusScaling * GlobScalingSqrt);
	}

	if (bLanded && (GlobStrength > MaxRestingGlobStrength))
	{
		if (Role == ROLE_Authority)
		{
			FActorSpawnParameters Params;
			Params.Instigator = Instigator;
			Params.Owner = Instigator;

			//Adjust a bit so globlings don't spawn in the floor
			FVector FloorOffset = GetActorLocation() + (SurfaceNormal * 10.0f);

			//Spawn globlings for as many Glob's above MaxRestingGlobStrength
			bSpawningGloblings = true;
			int32 NumGloblings = int32(GlobStrength) - MaxRestingGlobStrength;
			for (int32 i = 0; i<NumGloblings; i++)
			{
				FVector Dir = SurfaceNormal + FMath::VRand() * SplashSpread;
				AUTProjectile* Globling = GetWorld()->SpawnActor<AUTProjectile>(GetClass(), FloorOffset, Dir.Rotation(), Params);
				if (Globling)
				{
					Globling->ProjectileMovement->InitialSpeed *= 0.2f;
					Globling->ProjectileMovement->Velocity *= 0.2f;
				}
			}
			bSpawningGloblings = false;
		}
		GlobStrength = MaxRestingGlobStrength;
	}

	//Update any effects
	OnSetGlobStrength();

	if (bLanded)
	{
		PawnOverlapSphere->SetSphereRadius(LandedOverlapRadius + LandedOverlapScaling*GlobScalingSqrt, false);
		//PawnOverlapSphere->bHiddenInGame = false;
		//PawnOverlapSphere->bVisible = true;
	}
	GetRootComponent()->SetRelativeScale3D(FVector(GlobScalingSqrt));
}

void AUTProj_BioShot::OnSetGlobStrength_Implementation()
{
}

void AUTProj_BioShot::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AUTProj_BioShot, GlobStrength, COND_None);
}