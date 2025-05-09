/* Copyright JsonAsAsset Contributors 2024-2025 */

#include "Importers/Types/Materials/MaterialImporter.h"

/* Include Material.h (depends on UE Version) */
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3) || ENGINE_UE4
#include "Materials/Material.h"
#else
#include "MaterialDomain.h"
#endif

#include "Factories/MaterialFactoryNew.h"
#include "Settings/JsonAsAssetSettings.h"
#include <MaterialCachedData.h>
#include <Materials/MaterialExpressionScalarParameter.h>
#include <Materials/MaterialExpressionVectorParameter.h>
#include <Materials/MaterialExpressionTextureSampleParameter2D.h>
#include <Materials/MaterialExpressionStaticSwitchParameter.h>
#include <Materials/MaterialExpressionAdd.h>
#include <Materials/MaterialExpressionClamp.h>
#include <Materials/MaterialExpressionConstant.h>
#include <Materials/MaterialExpressionSetMaterialAttributes.h>
#include <Materials/MaterialAttributeDefinitionMap.h>

bool IMaterialImporter::Import() {
	/* Create Material Factory (factory automatically creates the Material) */
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), OutermostPkg, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

	/* Clear any default expressions the engine adds */
#if ENGINE_UE5
	Material->GetExpressionCollection().Empty();
#else
	Material->Expressions.Empty();
#endif

	/* Define material data from the JSON */
	FUObjectExportContainer ExpressionContainer;
	TSharedPtr<FJsonObject> Props = FindMaterialData(Material, JsonObject->GetStringField(TEXT("Type")), Material->GetName(), ExpressionContainer);

	/* Map out each expression for easier access */
	ConstructExpressions(ExpressionContainer);
	
	const UJsonAsAssetSettings* Settings = GetDefault<UJsonAsAssetSettings>();
	/* If Missing Material Data */
	if (ExpressionContainer.Num() == 0) {
		SpawnMaterialDataMissingNotification();
#if ENGINE_UE5
		const TSharedPtr<FJsonObject>* ShadingModelsPtr;
		if (AssetData->TryGetObjectField(TEXT("ShadingModels"), ShadingModelsPtr)) {
			int ShadingModelField;

			if (ShadingModelsPtr->Get()->TryGetNumberField(TEXT("ShadingModelField"), ShadingModelField)) {
				Material->GetShadingModels().SetShadingModelField(ShadingModelField);
			}
		}

		if (!Settings->AssetSettings.MaterialImportSettings.bSkipResultNodeConnection) {
			int32 x = 0, y = 0;

			FExpressionInput* matInput;
			if (Material->bUseMaterialAttributes) {
				// for mats that use attribtues
				x -= 16*8*2;
				FExpressionInput* AttributeInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_MaterialAttributes);
				UMaterialExpressionSetMaterialAttributes* setAttrNode = NewObject<UMaterialExpressionSetMaterialAttributes>(Material);
				Material->GetExpressionCollection().AddExpression(setAttrNode);
				setAttrNode->MaterialExpressionEditorX = x;
				setAttrNode->MaterialExpressionEditorY = 0;
				AttributeInput->Expression = setAttrNode;

				// determine input type based on shading model.
				switch (Material->MaterialDomain) {
					case EMaterialDomain::MD_Surface:
						if (Material->GetShadingModels().HasShadingModel(EMaterialShadingModel::MSM_Unlit)) {
							setAttrNode->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_EmissiveColor));
						} else {
							setAttrNode->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_BaseColor));
						}
						break;
					case EMaterialDomain::MD_LightFunction:
					case EMaterialDomain::MD_PostProcess:
					case EMaterialDomain::MD_UI:
						setAttrNode->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_EmissiveColor));
						break;
					case EMaterialDomain::MD_DeferredDecal:
						setAttrNode->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_WorldPositionOffset));
						break;
					case EMaterialDomain::MD_Volume:
					default:
						setAttrNode->AttributeSetTypes.Add(FMaterialAttributeDefinitionMap::GetID(EMaterialProperty::MP_BaseColor));
						break;
				}
				setAttrNode->Inputs.Add(FExpressionInput());
				setAttrNode->Inputs.Last().InputName = FName(*FMaterialAttributeDefinitionMap::GetDisplayNameForMaterial(setAttrNode->AttributeSetTypes.Last(), Material).ToString());
				matInput = &setAttrNode->Inputs.Last();
			} else {
				// for mats that don't use attributes
				switch (Material->MaterialDomain) {
					case EMaterialDomain::MD_Surface:
						if (Material->GetShadingModels().HasShadingModel(EMaterialShadingModel::MSM_Unlit)) {
							matInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_EmissiveColor);
						} else {
							matInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_BaseColor);
						}
						break;
					case EMaterialDomain::MD_LightFunction:
					case EMaterialDomain::MD_PostProcess:
					case EMaterialDomain::MD_UI:
						matInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_EmissiveColor);
						break;
					case EMaterialDomain::MD_DeferredDecal:
						matInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_WorldPositionOffset);
						break;
					case EMaterialDomain::MD_Volume:
					default:
						matInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_BaseColor);
						break;
				}
			}

			x -= 16*8*2;
			UMaterialExpressionClamp* clamp = NewObject<UMaterialExpressionClamp>(Material);
			clamp->MaterialExpressionEditorX = x + 16*4;
			clamp->MaterialExpressionEditorY = 0;
			clamp->MinDefault = 0.0f;
			clamp->MaxDefault = 1.0f;
			Material->GetExpressionCollection().AddExpression(clamp);
			matInput->Expression = clamp;

			FUObjectExportContainer ParamContainer;
			TSharedPtr<FJsonObject> CachedExpressionData = FindMaterialParameters(Material, JsonObject->GetStringField(TEXT("Type")), Material->GetName(), ParamContainer);
			UMaterialExpressionAdd* lastAdd = NULL;

#pragma region param recreation

			// map known scalar params
			if (
				CachedExpressionData->HasTypedField<EJson::Object>(TEXT("RuntimeEntries"))
				&& CachedExpressionData->GetObjectField(TEXT("RuntimeEntries"))->HasTypedField<EJson::Array>(TEXT("ParameterInfoSet"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("ScalarPrimitiveDataIndexValues"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("ScalarValues"))
				) {
				const TArray<TSharedPtr<FJsonValue>>
					paramsPtr = CachedExpressionData->GetObjectField(TEXT("RuntimeEntries"))->GetArrayField("ParameterInfoSet"),
					paramValueIndexesPtr = CachedExpressionData->GetArrayField(TEXT("ScalarPrimitiveDataIndexValues")),
					paramValuesPtr = CachedExpressionData->GetArrayField(TEXT("ScalarValues"));
				if (paramsPtr.Num() == paramValueIndexesPtr.Num() && paramsPtr.Num() == paramValuesPtr.Num()) {
					int32 i = 0;
					y -= 0;
					x -= 16*8*3;
					for (const TSharedPtr<FJsonValue> paramVal : paramsPtr) {
						const FJsonObject* paramObj = paramVal->AsObject().Get();
						FString paramName;
						int32 index;
						float value;
						if (
							paramObj->TryGetStringField(TEXT("Name"), paramName)
							&& paramValueIndexesPtr[i]->TryGetNumber(index)
							&& paramValuesPtr.IsValidIndex(index == -1 ? i : index)
							&& paramValuesPtr[index == -1 ? i : index]->TryGetNumber(value)
							) {
							UMaterialExpressionScalarParameter* param = NewObject<UMaterialExpressionScalarParameter>(Material);
							Material->GetExpressionCollection().AddExpression(param);
							param->ParameterName = FName(paramName);
							param->MaterialExpressionEditorX = x;
							param->MaterialExpressionEditorY = y;
							param->DefaultValue = value;

							UMaterialExpressionAdd* newAdd = NewObject<UMaterialExpressionAdd>(Material);
							Material->GetExpressionCollection().AddExpression(newAdd);
							newAdd->MaterialExpressionEditorX = x + 16 * 8 * 2;
							newAdd->MaterialExpressionEditorY = y;
							newAdd->A.Connect(0, param);
							if (lastAdd == NULL) {
								clamp->Input.Connect(0, newAdd);
							} else {
								lastAdd->B.Connect(0, newAdd);
							}
							lastAdd = newAdd;
						}
						y += 16 * 6;
						i++;
					}
				}
			}


			// map known vector params
			if (
				CachedExpressionData->HasTypedField<EJson::Object>(TEXT("RuntimeEntries[1]"))
				&& CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[1]"))->HasTypedField<EJson::Array>(TEXT("ParameterInfoSet"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("VectorPrimitiveDataIndexValues"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("VectorValues"))
				) {
				const TArray<TSharedPtr<FJsonValue>>
					paramsPtr = CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[1]"))->GetArrayField("ParameterInfoSet"),
					paramValueIndexesPtr = CachedExpressionData->GetArrayField(TEXT("VectorPrimitiveDataIndexValues")),
					paramValuesPtr = CachedExpressionData->GetArrayField(TEXT("VectorValues"));
				if (paramsPtr.Num() == paramValueIndexesPtr.Num() && paramsPtr.Num() == paramValuesPtr.Num()) {
					int32 i = 0;
					x -= 16*8*4;
					y = 0;
					for (const TSharedPtr<FJsonValue> paramVal : paramsPtr) {
						const FJsonObject* paramObj = paramVal->AsObject().Get();
						FString paramName;
						int32 index;
						TSharedPtr<FJsonObject>* value;
						float r, g, b, a;
						if (
							paramObj->TryGetStringField(TEXT("Name"), paramName)
							&& paramValueIndexesPtr[i]->TryGetNumber(index)
							&& paramValuesPtr.IsValidIndex(index == -1 ? i : index)
							&& paramValuesPtr[index == -1 ? i : index]->TryGetObject(value)
							&& value->Get()->TryGetNumberField(TEXT("R"), r)
							&& value->Get()->TryGetNumberField(TEXT("G"), g)
							&& value->Get()->TryGetNumberField(TEXT("B"), b)
							&& value->Get()->TryGetNumberField(TEXT("A"), a)
							) {
							UMaterialExpressionVectorParameter* param = NewObject<UMaterialExpressionVectorParameter>(Material);
							Material->GetExpressionCollection().AddExpression(param);
							param->ParameterName = FName(paramName);
							param->MaterialExpressionEditorX = x;
							param->MaterialExpressionEditorY = y;
							param->DefaultValue = FLinearColor(r, g, b, a);

							UMaterialExpressionAdd* newAdd = NewObject<UMaterialExpressionAdd>(Material);
							Material->GetExpressionCollection().AddExpression(newAdd);
							newAdd->MaterialExpressionEditorX = x + 16 * 8 * 2;
							newAdd->MaterialExpressionEditorY = y;
							newAdd->A.Connect(0, param);
							if (lastAdd == NULL) {
								clamp->Input.Connect(0, newAdd);
							} else {
								lastAdd->B.Connect(0, newAdd);
							}
							lastAdd = newAdd;
						}
						y += 16 * 13;
						i++;
					}
				}
			}


			// map known texture params
			if (
				CachedExpressionData->HasTypedField<EJson::Object>(TEXT("RuntimeEntries[3]"))
				&& CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[3]"))->HasTypedField<EJson::Array>(TEXT("ParameterInfoSet"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("TextureValues"))
				) {
				const TArray<TSharedPtr<FJsonValue>>
					paramsPtr = CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[3]"))->GetArrayField(TEXT("ParameterInfoSet")),
					paramValuesPtr = CachedExpressionData->GetArrayField(TEXT("TextureValues"));
				if (paramsPtr.Num() == paramValuesPtr.Num()) {
					int32 i = 0;
					x -= 16*8*4;
					y = 0;
					// import all known referenced textures;
					if (CachedExpressionData->HasTypedField<EJson::Array>(TEXT("ReferencedTextures"))) {
						for (const TSharedPtr<FJsonValue> refedTexture : CachedExpressionData->GetArrayField(TEXT("ReferencedTextures"))) {
							const TSharedPtr<FJsonObject> paramObj = refedTexture->AsObject();
							TObjectPtr<UObject> idk;
							IImporter::LoadObject(&paramObj, idk);
						}
					}
					for (const TSharedPtr<FJsonValue> paramVal : paramsPtr) {
						const FJsonObject* paramObj = paramVal->AsObject().Get();
						FString paramName, textureAssetPath, textureSubPath;
						TSharedPtr<FJsonObject>* textureValueObj;
						if (
							paramObj->TryGetStringField(TEXT("Name"), paramName)
							&& paramValuesPtr.IsValidIndex(i)
							&& paramValuesPtr[i]->TryGetObject(textureValueObj)
							&& textureValueObj->Get()->TryGetStringField("AssetPathName", textureAssetPath)
							&& textureValueObj->Get()->TryGetStringField("SubPathString", textureSubPath)
							) {
							UMaterialExpressionTextureSampleParameter2D* param = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
							Material->GetExpressionCollection().AddExpression(param);
							param->ParameterName = FName(paramName);
							param->MaterialExpressionEditorX = x;
							param->MaterialExpressionEditorY = y;
							TObjectPtr<UTexture> tex = TSoftObjectPtr<UTexture>(FSoftObjectPath(FName(textureAssetPath), textureSubPath)).LoadSynchronous();
							param->Texture = tex;
							param->SamplerType = param->GetSamplerTypeForTexture(tex.Get());

							UMaterialExpressionAdd* newAdd = NewObject<UMaterialExpressionAdd>(Material);
							Material->GetExpressionCollection().AddExpression(newAdd);
							newAdd->MaterialExpressionEditorX = x + 16 * 8 * 2;
							newAdd->MaterialExpressionEditorY = y;
							newAdd->A.Connect(0, param);
							if (lastAdd == NULL) {
								clamp->Input.Connect(0, newAdd);
							} else {
								lastAdd->B.Connect(0, newAdd);
							}
							lastAdd = newAdd;
						}
						y += 16*8*2;
						i++;
					}
				}
			}


			// map known switch params
			if (
				CachedExpressionData->HasTypedField<EJson::Object>(TEXT("RuntimeEntries[7]"))
				&& CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[7]"))->HasTypedField<EJson::Array>(TEXT("ParameterInfoSet"))
				&& CachedExpressionData->HasTypedField<EJson::Array>(TEXT("StaticSwitchValues"))
				) {
				const TArray<TSharedPtr<FJsonValue>>
					paramsPtr = CachedExpressionData->GetObjectField(TEXT("RuntimeEntries[7]"))->GetArrayField("ParameterInfoSet"),
					paramValuesPtr = CachedExpressionData->GetArrayField(TEXT("StaticSwitchValues"));
				if (paramsPtr.Num() == paramValuesPtr.Num()) {
					int32 i = 0;
					x -= 16*8*4;
					y = 0;
					for (const TSharedPtr<FJsonValue> paramVal : paramsPtr) {
						const FJsonObject* paramObj = paramVal->AsObject().Get();
						FString paramName;
						bool value;
						if (
							paramObj->TryGetStringField(TEXT("Name"), paramName)
							&& paramValuesPtr.IsValidIndex(i)
							&& paramValuesPtr[i]->TryGetBool(value)
							) {

							UMaterialExpressionConstant* constVal = NewObject<UMaterialExpressionConstant>(Material);
							Material->GetExpressionCollection().AddExpression(constVal);
							constVal->MaterialExpressionEditorX = x;
							constVal->MaterialExpressionEditorY = y;
							constVal->R = 1.0f;

							UMaterialExpressionStaticSwitchParameter* param = NewObject<UMaterialExpressionStaticSwitchParameter>(Material);
							Material->GetExpressionCollection().AddExpression(param);
							param->A.Connect(0, constVal);
							param->B.Connect(0, constVal);
							param->ParameterName = FName(paramName);
							param->MaterialExpressionEditorX = x;
							param->MaterialExpressionEditorY = y;
							param->DefaultValue = value;

							UMaterialExpressionAdd* newAdd = NewObject<UMaterialExpressionAdd>(Material);
							Material->GetExpressionCollection().AddExpression(newAdd);
							newAdd->MaterialExpressionEditorX = x + 16 * 8 * 2;
							newAdd->MaterialExpressionEditorY = y;
							newAdd->A.Connect(0, param);
							if (lastAdd == NULL) {
								clamp->Input.Connect(0, newAdd);
							} else {
								lastAdd->B.Connect(0, newAdd);
							}
							lastAdd = newAdd;
						}
						y += 16*9;
						i++;
					}
				}
			}

#pragma endregion param recreation
		}


		if (!OnAssetCreation(Material)) return false;

		GetObjectSerializer()->DeserializeObjectProperties(AssetData, Material);
		Material->UpdateCachedExpressionData();

		FMaterialUpdateContext MaterialUpdateContext;
		MaterialUpdateContext.AddMaterial(Material);

		Material->ForceRecompileForRendering();

		Material->PostEditChange();
		Material->MarkPackageDirty();
		Material->PreEditChange(nullptr);

		SavePackage();

		return true;
#else
		return false;
#endif
	}

	/* Iterate through all the expressions, and set properties */
	PropagateExpressions(ExpressionContainer);


#if ENGINE_UE5
	UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
#else
	UMaterial* EditorOnlyData = Material;
#endif
	
	if (!Settings->AssetSettings.MaterialImportSettings.bSkipResultNodeConnection) {
		TArray<FString> IgnoredProperties = TArray<FString> {
			"ParameterGroupData",
			"ExpressionCollection",
			"CustomizedUVs"
		};

		const TSharedPtr<FJsonObject> RawConnectionData = TSharedPtr<FJsonObject>(Props);
		for (FString Property : IgnoredProperties) {
			if (RawConnectionData->HasField(Property))
				RawConnectionData->RemoveField(Property);
		}
		
		/* Connect all pins using deserializer */
		GetObjectSerializer()->DeserializeObjectProperties(RawConnectionData, EditorOnlyData);

		/* CustomizedUVs defined here */
		const TArray<TSharedPtr<FJsonValue>>* InputsPtr;
		
		if (Props->TryGetArrayField(TEXT("CustomizedUVs"), InputsPtr)) {
			int i = 0;
			for (const TSharedPtr<FJsonValue> InputValue : *InputsPtr) {
				FJsonObject* InputObject = InputValue->AsObject().Get();
				FName InputExpressionName = GetExpressionName(InputObject);

				if (ExpressionContainer.Contains(InputExpressionName)) {
					FExpressionInput Input = PopulateExpressionInput(InputObject, ExpressionContainer.Find<UMaterialExpression>(InputExpressionName));
					EditorOnlyData->CustomizedUVs[i] = *reinterpret_cast<FVector2MaterialInput*>(&Input);
				}
				i++;
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* StringParameterGroupData;
	if (Props->TryGetArrayField(TEXT("ParameterGroupData"), StringParameterGroupData)) {
		TArray<FParameterGroupData> ParameterGroupData;

		for (const TSharedPtr<FJsonValue> ParameterGroupDataObject : *StringParameterGroupData) {
			if (ParameterGroupDataObject->IsNull()) continue;
			FParameterGroupData GroupData;

			FString GroupName;
			if (ParameterGroupDataObject->AsObject()->TryGetStringField(TEXT("GroupName"), GroupName)) GroupData.GroupName = GroupName;
			int GroupSortPriority;
			if (ParameterGroupDataObject->AsObject()->TryGetNumberField(TEXT("GroupSortPriority"), GroupSortPriority)) GroupData.GroupSortPriority = GroupSortPriority;

			ParameterGroupData.Add(GroupData);
		}

		EditorOnlyData->ParameterGroupData = ParameterGroupData;
	}

	/* Handle edit changes, and add it to the content browser */
	if (!OnAssetCreation(Material)) return false;

	const TSharedPtr<FJsonObject>* ShadingModelsPtr;
	
	if (AssetData->TryGetObjectField(TEXT("ShadingModels"), ShadingModelsPtr)) {
		int ShadingModelField;
		
		if (ShadingModelsPtr->Get()->TryGetNumberField(TEXT("ShadingModelField"), ShadingModelField)) {
#if ENGINE_UE5
			Material->GetShadingModels().SetShadingModelField(ShadingModelField);
#else
			/* Not to sure what to do in UE4, no function exists to override it. */
#endif
		}
	}

	/* Deserialize any properties */
	GetObjectSerializer()->DeserializeObjectProperties(AssetData, Material);

	Material->UpdateCachedExpressionData();
	
	FMaterialUpdateContext MaterialUpdateContext;
	MaterialUpdateContext.AddMaterial(Material);
	
	Material->ForceRecompileForRendering();

	Material->PostEditChange();
	Material->MarkPackageDirty();
	Material->PreEditChange(nullptr);

	SavePackage();

	return true;
}