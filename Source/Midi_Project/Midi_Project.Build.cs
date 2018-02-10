// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;
using System;

public class Midi_Project : ModuleRules
{


	public Midi_Project(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore","UMG", "Kiss_FFT" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "Voice" });

        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "vamp-plugins", "Includes"));

        //PublicDelayLoadDLLs.Add("vamp-example-plugins.dll");

        //PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "vamp-plugins", "VampExamplePlugins.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "vamp-plugins", "VampHostSDK.lib")); 
        //LoadPortAudio(Target);
        //LoadVampPlugins(Target);
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }


    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    public bool LoadVampPlugins(TargetInfo Target)
    {
        bool isLibrarySupported = false;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;

            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
            string LibrariesPath = Path.Combine(ThirdPartyPath, "vamp-plugins", "Libraries");


            //test your path with:
            //using System; // Console.WriteLine("");
            Console.WriteLine("... LibrariesPath -> " + LibrariesPath);


            //PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "vamp-example-plugins_" + PlatformString + ".lib"));
        }

        if (isLibrarySupported)
        {
            Console.WriteLine("INCLUDE ");
            // Include path
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "vamp-plugins", "Includes"));
        }

        Definitions.Add(string.Format("WITH_VAMP_PLUGINS_BINDING={0}", isLibrarySupported ? 1 : 0));

        return isLibrarySupported;
    }


    public bool LoadPortAudio(TargetInfo Target)
    {
        bool isLibrarySupported = false;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;

            string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
            string LibrariesPath = Path.Combine(ThirdPartyPath, "portaudio", "Libraries");

            
            //test your path with:
            //using System; // Console.WriteLine("");
            Console.WriteLine("... LibrariesPath -> " + LibrariesPath);
            

            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "portaudio_" + PlatformString + ".lib"));
        }

        if (isLibrarySupported)
        {
            // Include path
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "portaudio", "Includes"));
        }

        Definitions.Add(string.Format("WITH_PORT_AUDIO_BINDING={0}", isLibrarySupported ? 1 : 0));

        return isLibrarySupported;
    }
}
