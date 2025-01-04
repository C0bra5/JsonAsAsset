// Copyright JAA Contributors 2024-2025

#include "Importers/Types/DataAssetImporter.h"
#include "Engine/DataAsset.h"

bool IDataAssetImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		GetObjectSerializer()->SetPackageForDeserialization(Package);

		UDataAsset* DataAsset = NewObject<UDataAsset>(Package, DataAssetClass, FName(FileName), RF_Public | RF_Standalone);
		GetObjectSerializer()->DeserializeObjectProperties(Properties, DataAsset);

		return OnAssetCreation(DataAsset);
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
	}

	return false;
}
