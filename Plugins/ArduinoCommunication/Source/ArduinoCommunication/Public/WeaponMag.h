// Arduino Communication Plugin - Weapon Magazine Configuration Struct
// Separate header file for FWeaponMag struct to allow independent use

#pragma once

#include "CoreMinimal.h"
#include "FiringComponent.h"
#include "WeaponMag.generated.h"

// ============================================================================
// Weapon Magazine Configuration Struct
// ============================================================================

/**
 * Weapon Magazine configuration that maps an RFID tag to firing parameters
 * When a weapon tag is inserted, the corresponding WeaponMag configuration
 * is applied to the FiringComponent
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FWeaponMag
{
	GENERATED_BODY()

	/** Whether this weapon mag is currently active/enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag")
	bool bActive = true;

	/** RFID/NFC tag UID that identifies this weapon magazine */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag")
	int64 TagId = 0;

	/** Display name for this weapon type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag")
	FName WeaponName = NAME_None;

	/** Firing mode to activate when this mag is inserted. Shown as a dropdown in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag")
	EFiringModeType FiringMode = EFiringModeType::Bullet;

	/** Damage per bullet (only applies to Bullet mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	/** Rounds fired per second (only applies to Bullet mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "0.1"))
	float RateOfFire = 10.0f;

	/** Bullet spread angle in degrees (only applies to Bullet mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadAngle = 1.0f;

	/** Number of bullets per shot (only applies to Bullet mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "1"))
	int32 BulletsPerShot = 1;

	/** Maximum ammo capacity for this mag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "1"))
	int32 MaxAmmo = 100;

	/** Current ammo count in this mag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Bullet", meta = (ClampMin = "0"))
	int32 CurrentAmmo = 100;

	/** Firing range in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag", meta = (ClampMin = "0.0"))
	float Range = 5000.0f;

	/** Scan duration for scanner mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag|Scanner", meta = (ClampMin = "0.1"))
	float ScanDuration = 2.0f;

	FWeaponMag() = default;
};
