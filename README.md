# JsonAsAsset

[![Discord](https://img.shields.io/badge/Join%20Discord-Collector?color=7289DA&label=JsonAsAsset&logo=discord&logoColor=7289DA&style=for-the-badge)](https://discord.gg/h9s6qpBnUT)
[![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/JsonAsAsset/JsonAsAsset/latest/total?style=for-the-badge&label=DOWNLOADS&color=blue)](https://github.com/JsonAsAsset/JsonAsAsset/releases)
[![GitHub Repo stars](https://img.shields.io/github/stars/JsonAsAsset/JsonAsAsset?style=for-the-badge&logo=&color=blue)](https://github.com/JsonAsAsset/JsonAsAsset/stargazers)

JsonAsAsset is a user-friendly [Unreal Engine](https://www.unrealengine.com/en-US) plugin for importing assets from packaged games using [JSON](https://www.json.org/json-en.html) files.

**Editor Only Data**:
<br> In Unreal Engine 5.2 and above, editor only data has been introduced to Unreal Engine. Allowing us to import materials and material functions into our own project from the pak files of a game.

⭐ [Contributors](#contribute)

-----------------

**Table of Contents**:
<br> 

> 1. [Introduction to JsonAsAsset](#intro)
> 1. [Installing JsonAsAsset](#install)  
>    2.1 [Setting Up JAA Settings](#setup-jaa)  
>    2.2 [Export Directory](#export-directory)  
> 3. [Local Fetch](#setup-local-fetch)

-----------------

<a name="intro"></a>
## 1. Introduction to JsonAsAsset
> [!NOTE]
> Please note that this plugin is intended solely for **personal and educational use**. Do not use it to create or distribute **commercial products** without obtaining the necessary **licenses and permissions**. It is important to respect **intellectual property rights** and only use assets that you are **authorized to use**. We do not assume any responsibility for the way the created content is used.

JsonAsAsset is a user-friendly Unreal Engine plugin for importing assets from packaged games using JSON files. With a sleek interface, it simplifies mapping JSON data to Unreal Engine structures, supporting both manual and automated workflows.

<details>
  <summary>Supported Asset Types</summary>

##### *All Sound Asset Types

###### Materials
 - Material
 - MaterialFunction
 - MaterialParameterCollection
 - PhysicalMaterial
 - SubsurfaceProfile
     
###### Curve Asset Types
 - CurveFloat
 - CurveTable
 - CurveVector
 - CurveLinearColor
 - CurveLinearColorAtlas

###### Skeleton/Animation Asset Types
 - SkeletalMeshLODSettings
 - Animation (curves, sync markers)
 - Blendspace

###### Data Asset Types
- DataAsset
- DataTable

</details>

<a name="install"></a>
## 2. Installing JsonAsAsset
> [!WARNING]
> JsonAsAsset may not work for every Unreal Engine 5 version, please check Releases to see compatibility. Unreal Engine 4 is not maintained at the moment, and is not planned to be supported.
> *`(See branches for the current available Unreal Engine 4 versions)`*

1. Go to the [Releases page](/../../releases) for the plugin.
2. **Download the release** that matches your version of Unreal Engine. If there **isn't a release that matches your version**, you will need to [**compile the plugin yourself**](https://dev.epicgames.com/community/learning/tutorials/qz93/unreal-engine-building-plugins).
3. Extract the downloaded files to your project's Plugins folder. If there isn't a Plugins folder, create one in the root directory of your project.
4. Open your Unreal Engine project.
5. Click on Edit -> Plugins.
6. In the Plugins window, search for `JsonAsAsset` and enable it.
7. Restart the editor for the changes to take effect.

<a name="setup-jaa"></a>
#### 2.1 Setting Up JAA Settings
If you haven't already, install FModel and setup it up correctly.
<img align="left" width="150" height="150" src="./Resources/FModelLogo.png?raw=true">
The JSON format/file has to be from a program that fits the format of FModel's JSON export files:
<br><br>

- [FModel](https://fmodel.app) *(Software for exploring Unreal Engine games)*

-------------------

Now that you've installed FModel and setup it up correctly, we can continue to setting up JsonAsAsset for our own Unreal Engine project.

<a name="export-directory"></a>
##### 2.2 Setting up Export Directory
<img align="right" width="300" height="180" src=https://github.com/JsonAsAsset/JsonAsAsset/assets/73559984/aad4e86a-6f0b-4e66-aef1-13d30d8215de)>

> [!NOTE]
> Upon launching your unreal engine project, you should of seen a notification asking you to change your export directory in the plugin settings, if you've already done that, skip this part.

We need to change the export directory to allow the plugin to differentiate between what's your directory, and what's the game path it should put it in.

First, open up to the JsonAsAsset plugin settings (basically do what's on the picture on the right) and make sure you are looking at the property "Export Directory".

Now open up FModel, and go to your settings. `(Settings -> General)` There will be a setting called `Output Directory`, copy that and go back to Unreal Engine. Now you need to click on the three dots and jump to the folder you copied and go to the folder named `Exports`, then press `Select Folder`.

-------------------

That’s the basic setup! To bulk import assets and **what they reference**, you'll need to set up [`Local Fetch`](#setup-local-fetch)!

<a name="setup-local-fetch"></a>
## 3. Setting Up *Local Fetch*
> If you want to contribute to the Local Fetch API, check out the source code at [/JsonAsAsset/LocalFetch](https://github.com/JsonAsAsset/LocalFetch) repository

Running the API requires ASP.NET 8.0 to be installed, please install this [here](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-aspnetcore-8.0.1-windows-x64-installer).

<img align="right" width="461.5" height="164" src=https://github.com/JsonAsAsset/JsonAsAsset/assets/73559984/cddf0ea7-2499-4b39-a7af-e6f27ec5148e>

> [!TIP]
> Please make sure you have the plugin in your project's directory and not in the Engine.

Before we can launch Local Fetch and begin working on automated references, you need to input all the information about your game first.

Many of these settings are similar to FModel, but make sure to manually select a file or directory using UE's file selector.

###### Launching Local Fetch
<img align="right" width="461.5" height="250" src=https://github.com/JsonAsAsset/JsonAsAsset/assets/73559984/4688482d-0854-4a62-83cf-fc055d657284>

> [!IMPORTANT]
> You must launch Local Fetch through UE, and not by the executable file. The reason being is that the local host port is different when you launch it through UE, so it's important you do so.

After fully setting up the Local Fetch settings, you can launch the API without any issues.

-------------------

Go ahead and click on the JsonAsAsset logo (<img width="25" height="25" src=https://github.com/JsonAsAsset/JsonAsAsset/assets/73559984/b90ab71f-d9ac-4349-96eb-620aadf7812f>) and hover over the list `"Command-line Application"` and press `"Execute Local Fetch API (.EXE)"`.

A window should pop-up, and once the console says `[CORE] Running API`, Local Fetch has been successfully started!

<a name="contribute"></a>
## ⭐ Contributors
[Tector](https://github.com/Tectors), [GMatrixGames](https://github.com/GMatrixGames), [Tajgames](https://github.com/), [Zylox](https://github.com/0xZylox), and massive thanks to the people who contributed to [UEAssetToolkit](https://github.com/Buckminsterfullerene02/UEAssetToolkit-Fixes)!

- *JsonAsAsset Logo*: *[@Tevtongermany](https://github.com/Tevtongermany)*

### `Key information for contributors:` 🎓

Local Fetch's API is located at [JsonAsAsset/LocalFetch](https://github.com/JsonAsAsset/LocalFetch)
It uses CUE4Parse to read game files, and it interacts with the config files inside of the project.

##### `Adding Asset Types`
> *Asset types without manual code will use **basic** importing, meaning it will only take the properties of the base object and import them.*
- Normal Asset types are found in [`JsonAsAsset/Private/Importers/Constructor/Importer.cpp`](https://github.com/JsonAsAsset/JsonAsAsset/blob/main/Source/JsonAsAsset/Private/Importers/Constructor/Importer.cpp#L51)
- Adding support for Local Fetch of asset types [`JsonAsAsset/Private/Importers/Utilities/AssetUtilities.cpp`](https://github.com/JsonAsAsset/JsonAsAsset/blob/main/Source/JsonAsAsset/Private/Utilities/AssetUtilities.cpp#L28)

##### `Custom Logic for Asset Types`

Adding **manual** asset type imports is done in the [`JsonAsAsset/Public/Importers/Types`](https://github.com/JsonAsAsset/JsonAsAsset/tree/main/Source/JsonAsAsset/Public/Importers/Types) folder.

##### `Settings`

JsonAsAsset's settings are in [`Private/Settings/JsonAsAssetSettings.h`](https://github.com/JsonAsAsset/JsonAsAsset/tree/main/Source/JsonAsAsset/Private/Settings/JsonAsAssetSettings.h)
