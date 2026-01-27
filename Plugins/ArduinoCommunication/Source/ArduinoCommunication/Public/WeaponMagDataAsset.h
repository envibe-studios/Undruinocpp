// Arduino Communication Plugin - Weapon Magazine Data Asset
// UDataAsset wrapper for FWeaponMag to allow saving as a .uasset file

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponMag.h"
#include "WeaponMagDataAsset.generated.h"

/**
 * Data Asset for storing Weapon Magazine configurations
 * This allows FWeaponMag data to be saved as a .uasset file in Content/Blueprints/
 * and edited directly in the Unreal Editor
 */
UCLASS(BlueprintType)
class ARDUINOCOMMUNICATION_API UWeaponMagDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The weapon magazine configuration data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag")
	FWeaponMag WeaponMagData;

	/** Get the weapon mag configuration */
	UFUNCTION(BlueprintPure, Category = "Weapon Mag")
	FWeaponMag GetWeaponMag() const { return WeaponMagData; }

	/** Set the weapon mag configuration */
	UFUNCTION(BlueprintCallable, Category = "Weapon Mag")
	void SetWeaponMag(const FWeaponMag& NewWeaponMag) { WeaponMagData = NewWeaponMag; }
};

/**
 * Data Asset for storing multiple Weapon Magazine configurations
 * Useful for defining a collection of weapon mags in a single asset
 */
UCLASS(BlueprintType)
class ARDUINOCOMMUNICATION_API UWeaponMagCollectionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Array of weapon magazine configurations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Mag Collection")
	TArray<FWeaponMag> WeaponMags;

	/** Get all weapon mags */
	UFUNCTION(BlueprintPure, Category = "Weapon Mag Collection")
	TArray<FWeaponMag> GetAllWeaponMags() const { return WeaponMags; }

	/** Find a weapon mag by tag ID */
	UFUNCTION(BlueprintPure, Category = "Weapon Mag Collection")
	bool FindWeaponMagByTagId(int64 TagId, FWeaponMag& OutWeaponMag) const
	{
		for (const FWeaponMag& Mag : WeaponMags)
		{
			if (Mag.TagId == TagId && Mag.bActive)
			{
				OutWeaponMag = Mag;
				return true;
			}
		}
		return false;
	}

	/** Find a weapon mag by name */
	UFUNCTION(BlueprintPure, Category = "Weapon Mag Collection")
	bool FindWeaponMagByName(FName WeaponName, FWeaponMag& OutWeaponMag) const
	{
		for (const FWeaponMag& Mag : WeaponMags)
		{
			if (Mag.WeaponName == WeaponName && Mag.bActive)
			{
				OutWeaponMag = Mag;
				return true;
			}
		}
		return false;
	}
};
